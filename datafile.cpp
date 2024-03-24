#include "datafile.h"
#include "file.h"
#include "keydir.h"
#include "basictypes.h"
#include "hton.h"
#include "crc32.h"

#include <fmt/format.h>

#include <string_view>
#include <optional>
#include <functional>
#include <charconv>
#include <stdexcept>
#include <cstring>
#include <limits>

#include <fcntl.h>

namespace {

constexpr auto max_ksz          = std::numeric_limits<ksz_type>::max();
constexpr auto deleted_value_sz = std::numeric_limits<value_sz_type>::max();
constexpr auto max_value_sz     = deleted_value_sz - 1u;

struct record_header
{
	crc_type      crc;
	version_type  version;
	ksz_type      ksz;
	value_sz_type value_sz;

	static constexpr auto size = sizeof(crc_type) + sizeof(version_type) + sizeof(ksz_type) + sizeof(value_sz_type);

	char buffer[size];

	bool read(const lock_type& lock, file& f, crc_type& crc)
	{
		if (f.locked_read(lock, this->buffer, size, file::read_mode::zero_or_count))
		{
			auto src = this->buffer;

			std::memcpy(&this->crc, src, sizeof(this->crc));
			src += sizeof(this->crc);

			crc = crc32_fast(src, size - sizeof(this->crc));

			std::memcpy(&this->version, src, sizeof(this->version));
			src += sizeof(this->version);

			std::memcpy(&this->ksz, src, sizeof(this->ksz));
			src += sizeof(this->ksz);

			std::memcpy(&this->value_sz, src, sizeof(this->value_sz));

			this->crc      = ntoh(this->crc);
			this->version  = ntoh(this->version);
			this->ksz      = ntoh(this->ksz);
			this->value_sz = ntoh(this->value_sz);

			return true;
		}
		else
		{
			return false;
		}
	}

	void init_crc()
	{
		const auto n_version  = hton(this->version);
		const auto n_ksz      = hton(this->ksz);
		const auto n_value_sz = hton(this->value_sz);

		auto begin = this->buffer + sizeof(this->crc);
		auto dst   = begin;

		std::memcpy(dst, &n_version, sizeof(n_version));
		dst += sizeof(n_version);

		std::memcpy(dst, &n_ksz, sizeof(n_ksz));
		dst += sizeof(n_ksz);

		std::memcpy(dst, &n_value_sz, sizeof(n_value_sz));

		this->crc = crc32_fast(begin, size - sizeof(this->crc));
	}

	void write(const lock_type& lock, file& f)
	{
		const auto n_crc      = hton(this->crc);
		const auto n_version  = hton(this->version);
		const auto n_ksz      = hton(this->ksz);
		const auto n_value_sz = hton(this->value_sz);

		auto dst = this->buffer;

		std::memcpy(dst, &n_crc, sizeof(n_crc));
		dst += sizeof(n_crc);

		std::memcpy(dst, &n_version, sizeof(n_version));
		dst += sizeof(n_version);

		std::memcpy(dst, &n_ksz, sizeof(n_ksz));
		dst += sizeof(n_ksz);

		std::memcpy(dst, &n_value_sz, sizeof(n_value_sz));

		f.locked_write(lock, this->buffer, size);
	}
};

file_id_type get_id_from_file_name(std::string_view name)
{
	if (name.starts_with("bitcask-") && name.ends_with(".data") && name.length() == 8u + file_id_nibbles + 5u)
	{
		const auto first = name.data() + 8u;
		const auto last  = first + file_id_nibbles;
		auto       id    = file_id_type{};
		auto       res   = std::from_chars(first, last, id, 16);
		if (res.ec == std::errc{} && res.ptr == last)
		{
			return id;
		}
	}
	throw std::invalid_argument{ fmt::format("'{}' is not a valid data file name", name) };
}

} // namespace

std::regex datafile::name_regex{ fmt::format(R"~(^bitcask-[0-9a-f]{{{}}}\.data)~", file_id_nibbles) };

std::string datafile::make_filename(file_id_type id)
{
	return fmt::format("bitcask-{0:0{1}x}.data", id, file_id_nibbles);
}

class datafile::impl final
{
	std::unique_ptr<file> file_;
	file_id_type          id_;

public:
	explicit impl(std::unique_ptr<file>&& f)
	    : file_{ std::move(f) }
	    , id_{ get_id_from_file_name(this->file_->path().filename().string()) }
	{
	}

	file_id_type id() const
	{
		return this->id_;
	}

	std::filesystem::path path() const
	{
		return this->file_->path();
	}

	std::filesystem::path hint_path() const
	{
		return hint_path(this->path());
	}

	static std::filesystem::path hint_path(const std::filesystem::path& path)
	{
		return path.string() + ".hint";
	}

	bool size_greater_than(off64_t size) const
	{
		return this->file_->size() > size;
	}

	void reopen(int flags, mode_t mode) const
	{
		return this->file_->reopen(flags, mode);
	}

	void build_keydir(keydir& kd) const
	{
		{
			const auto hint_path = this->hint_path();
			if (std::filesystem::exists(hint_path))
			{
				return hintfile{ file::open(hint_path, O_RDONLY, 0664) }.build_keydir(kd, this->id_);
			}
		}

		this->traverse([&](const auto& rec) {
			if (rec.value)
			{
				const auto& v = rec.value.value();
				kd.put(rec.key,
				       keydir::info{ .file_id   = this->id_,
				                     .value_sz  = static_cast<value_sz_type>(v.value.size()),
				                     .value_pos = v.value_pos,
				                     .version   = v.version });
			}
			else
			{
				kd.del(rec.key);
			}
		});
	}

	value_type get(const keydir::info& info) const
	{
		auto value = value_type{};
		if (info.value_sz)
		{
			value.resize(info.value_sz);
			const auto lock = this->file_->lock();
			this->file_->locked_seek(lock, info.value_pos);
			this->file_->locked_read(lock, value.data(), value.size(), file::read_mode::count);
		}
		return value;
	}

	keydir::info put(const std::string_view& key, const std::string_view& value, version_type version) const
	{
		if (key.length() > max_ksz)
		{
			throw std::runtime_error{ fmt::format("Key length exceeds limit of {}", max_ksz) };
		}

		if (value.length() > max_value_sz)
		{
			throw std::runtime_error{ fmt::format("Value length exceeds limit of {}", max_value_sz) };
		}

		const auto lock = this->file_->lock();

		this->file_->locked_seek(lock, 0, SEEK_END);

		auto header = record_header{};

		header.version  = version;
		header.ksz      = key.length();
		header.value_sz = value.length();
		header.init_crc();

		if (!key.empty())
		{
			header.crc = crc32_fast(key.data(), key.length(), header.crc);
		}

		if (!value.empty())
		{
			header.crc = crc32_fast(value.data(), value.length(), header.crc);
		}

		header.write(lock, *this->file_);

		this->file_->locked_write(lock, key.data(), key.length());

		const auto value_pos = this->file_->locked_position(lock);

		this->file_->locked_write(lock, value.data(), value.length());

		return keydir::info{
			.file_id   = this->id_,
			.value_sz  = header.value_sz,
			.value_pos = value_pos,
			.version   = header.version,
		};
	}

	void del(const std::string_view& key, version_type version) const
	{
		if (key.length() > max_ksz)
		{
			throw std::runtime_error{ fmt::format("Key length exceeds limit of {}", max_ksz) };
		}

		const auto lock = this->file_->lock();

		this->file_->locked_seek(lock, 0, SEEK_END);

		auto header = record_header{};

		header.version  = version;
		header.ksz      = key.length();
		header.value_sz = deleted_value_sz;
		header.init_crc();

		if (!key.empty())
		{
			header.crc = crc32_fast(key.data(), key.length(), header.crc);
		}

		header.write(lock, *this->file_);

		this->file_->locked_write(lock, key.data(), key.length());
	}

	void traverse(std::function<void(const record&)> callback) const
	{
		const auto lock = this->file_->lock();

		this->file_->locked_seek(lock, 0);

		auto header = record_header{};

		auto key_buffer = std::string{};
		key_buffer.reserve(4096u);

		auto value_buffer = std::string{};
		value_buffer.reserve(4096u);

		for (;;)
		{
			const auto position = this->file_->locked_position(lock);

			auto crc = crc_type{};

			if (!header.read(lock, *this->file_, crc))
			{
				break;
			}

			// read the key
			const auto key_buffer_size = std::max(key_buffer.capacity(), static_cast<std::string::size_type>(header.ksz));
			key_buffer.resize(key_buffer_size);

			this->file_->locked_read(lock, key_buffer.data(), header.ksz, file::read_mode::count);

			crc = crc32_fast(key_buffer.data(), header.ksz, crc);

			auto rec = record{ .key = std::string_view{ key_buffer }.substr(0, header.ksz), .value = std::nullopt };

			// Not using a tombstone value as delete marker (as mentioned in https://riak.com/assets/bitcask-intro.pdf)
			// because any value, no matter how unique, could not be used as a real value.
			// Maybe that's just splitting hairs, but it's just not my idea of good practice.
			// I'm using maximum length as delete marker.
			if (header.value_sz != deleted_value_sz)
			{
				const auto value_pos = this->file_->locked_position(lock);

				const auto value_buffer_size = std::max(value_buffer.capacity(), static_cast<std::string::size_type>(header.value_sz));
				value_buffer.resize(value_buffer_size);

				this->file_->locked_read(lock, value_buffer.data(), header.value_sz, file::read_mode::count);

				crc = crc32_fast(value_buffer.data(), header.value_sz, crc);

				rec.value = record::value_info{ .value_pos = value_pos,
					                            .value     = std::string_view{ value_buffer }.substr(0, header.value_sz),
					                            .version   = header.version };
			}

			if (crc != header.crc)
			{
				// TODO: try to recover from this.
				// Starting at the current position, seek forward 1 byte per iteration and try
				// to read the next record. Continue this iteration until a valid record is found
				// or until EOF is reached.
				// A corrupted file can happen for multiple reasons, this should not be fatal.
				throw std::runtime_error{ fmt::format(
					"{}: CRC mismatch in record at position {}", this->file_->path().string(), position) };
			}

			callback(rec);
		}
	}
};

datafile::datafile(std::unique_ptr<file>&& f)
    : pimpl_{ std::make_unique<impl>(std::move(f)) }
{
}

datafile::~datafile() noexcept
{
}

file_id_type datafile::id() const
{
	return this->pimpl_->id();
}

std::filesystem::path datafile::path() const
{
	return this->pimpl_->path();
}

std::filesystem::path datafile::hint_path() const
{
	return this->pimpl_->hint_path();
}

std::filesystem::path datafile::hint_path(const std::filesystem::path& path)
{
	return impl::hint_path(path);
}

bool datafile::size_greater_than(off64_t size) const
{
	return this->pimpl_->size_greater_than(size);
}

void datafile::reopen(int flags, mode_t mode) const
{
	return this->pimpl_->reopen(flags, mode);
}

void datafile::build_keydir(keydir& kd) const
{
	return this->pimpl_->build_keydir(kd);
}

value_type datafile::get(const keydir::info& info) const
{
	return this->pimpl_->get(info);
}

keydir::info datafile::put(const std::string_view& key, const std::string_view& value, version_type version) const
{
	return this->pimpl_->put(key, value, version);
}

void datafile::del(const std::string_view& key, version_type version) const
{
	return this->pimpl_->del(key, version);
}

void datafile::traverse(std::function<void(const record&)> callback) const
{
	return this->pimpl_->traverse(callback);
}
