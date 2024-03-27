//
// Copyright (C) 2024 Patrick Rotsaert
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "bitcask/hintfile.h"
#include "bitcask/crc32.h"
#include "bitcask/hton.h"

#include <fmt/format.h>

#include <functional>
#include <cstring>

namespace bitcask {

namespace {

struct record_header
{
	crc_type       crc;
	version_type   version;
	ksz_type       ksz;
	value_sz_type  value_sz;
	value_pos_type value_pos;

	static constexpr auto size =
	    sizeof(crc_type) + sizeof(version_type) + sizeof(ksz_type) + sizeof(value_sz_type) + sizeof(value_pos_type);

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
			src += sizeof(this->value_sz);

			std::memcpy(&this->value_pos, src, sizeof(this->value_pos));

			this->crc       = ntoh(this->crc);
			this->version   = ntoh(this->version);
			this->ksz       = ntoh(this->ksz);
			this->value_sz  = ntoh(this->value_sz);
			this->value_pos = ntoh(this->value_pos);

			return true;
		}
		else
		{
			return false;
		}
	}

	void init_crc()
	{
		const auto n_version   = hton(this->version);
		const auto n_ksz       = hton(this->ksz);
		const auto n_value_sz  = hton(this->value_sz);
		const auto n_value_pos = hton(this->value_pos);

		auto begin = this->buffer + sizeof(this->crc);
		auto dst   = begin;

		std::memcpy(dst, &n_version, sizeof(n_version));
		dst += sizeof(n_version);

		std::memcpy(dst, &n_ksz, sizeof(n_ksz));
		dst += sizeof(n_ksz);

		std::memcpy(dst, &n_value_sz, sizeof(n_value_sz));
		dst += sizeof(n_value_sz);

		std::memcpy(dst, &n_value_pos, sizeof(n_value_pos));

		this->crc = crc32_fast(begin, size - sizeof(this->crc));
	}

	void write(const lock_type& lock, file& f)
	{
		const auto n_crc       = hton(this->crc);
		const auto n_version   = hton(this->version);
		const auto n_ksz       = hton(this->ksz);
		const auto n_value_sz  = hton(this->value_sz);
		const auto n_value_pos = hton(this->value_pos);

		auto dst = this->buffer;

		std::memcpy(dst, &n_crc, sizeof(n_crc));
		dst += sizeof(n_crc);

		std::memcpy(dst, &n_version, sizeof(n_version));
		dst += sizeof(n_version);

		std::memcpy(dst, &n_ksz, sizeof(n_ksz));
		dst += sizeof(n_ksz);

		std::memcpy(dst, &n_value_sz, sizeof(n_value_sz));
		dst += sizeof(n_value_sz);

		std::memcpy(dst, &n_value_pos, sizeof(n_value_pos));

		f.locked_write(lock, this->buffer, size);
	}
};

} // namespace

class hintfile::impl
{
	std::unique_ptr<file> file_;

	struct record
	{
		record_header    header;
		std::string_view key;
	};

	void traverse(std::function<void(const record&)> callback) const
	{
		const auto lock = this->file_->lock();

		this->file_->locked_seek(lock, 0);

		auto rec = record{};

		auto key_buffer = std::string{};
		key_buffer.reserve(4096u);

		for (;;)
		{
			const auto position = this->file_->locked_position(lock);

			auto crc = crc_type{};

			if (!rec.header.read(lock, *this->file_, crc))
			{
				break;
			}

			// read the key
			const auto key_buffer_size = std::max(key_buffer.capacity(), static_cast<std::string::size_type>(rec.header.ksz));
			key_buffer.resize(key_buffer_size);

			this->file_->locked_read(lock, key_buffer.data(), rec.header.ksz, file::read_mode::count);

			crc = crc32_fast(key_buffer.data(), rec.header.ksz, crc);

			rec.key = std::string_view{ key_buffer }.substr(0, rec.header.ksz);

			if (crc != rec.header.crc)
			{
				// TODO: do not throw but return a value that tells the caller that the hint file
				// is corrupted. The caller must then delete this hint file and read the keys from the
				// datafile instead.
				throw std::runtime_error{ fmt::format(
					"{}: CRC mismatch in record at position {}", this->file_->path().string(), position) };
			}

			callback(rec);
		}
	}

public:
	explicit impl(std::unique_ptr<file>&& f)
	    : file_{ std::move(f) }
	{
	}

	std::filesystem::path path() const
	{
		return this->file_->path();
	}

	void build_keydir(keydir& kd, file_id_type file_id)
	{
		this->traverse([&](const record& rec) {
			kd.put(rec.key,
			       keydir::info{ .file_id   = file_id,
			                     .value_sz  = rec.header.value_sz,
			                     .value_pos = rec.header.value_pos,
			                     .version   = rec.header.version });
		});
	}

	void put(hintfile::hint&& rec) const
	{
		const auto lock = this->file_->lock();

		this->file_->locked_seek(lock, 0, SEEK_END);

		auto header = record_header{};

		header.version   = rec.version;
		header.ksz       = rec.key.length();
		header.value_sz  = rec.value_sz;
		header.value_pos = rec.value_pos;
		header.init_crc();

		if (!rec.key.empty())
		{
			header.crc = crc32_fast(rec.key.data(), rec.key.length(), header.crc);
		}

		header.write(lock, *this->file_);

		this->file_->locked_write(lock, rec.key.data(), rec.key.length());
	}
};

hintfile::hintfile(std::unique_ptr<file>&& f)
    : pimpl_{ std::make_unique<impl>(std::move(f)) }
{
}

hintfile::~hintfile() noexcept
{
}

std::filesystem::path hintfile::path() const
{
	return this->pimpl_->path();
}

void hintfile::build_keydir(keydir& kd, file_id_type file_id) const
{
	return this->pimpl_->build_keydir(kd, file_id);
}

void hintfile::put(hint&& rec) const
{
	this->pimpl_->put(std::move(rec));
}

} // namespace bitcask
