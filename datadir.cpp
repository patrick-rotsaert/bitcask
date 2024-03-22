#include "datadir.h"
#include "datafile.h"
#include "hintfile.h"
#include "keydir.h"
#include "file.h"
#include "lockfile.h"

#include <fmt/format.h>

#include <system_error>
#include <regex>
#include <set>
#include <vector>
#include <map>
#include <algorithm>
#include <limits>
#include <cassert>

#include <fcntl.h>

namespace fs = std::filesystem;

class datadir::impl final
{
	fs::path                                          directory_{};
	std::unique_ptr<lockfile>                         lockfile_{};
	std::map<file_id_type, std::unique_ptr<datafile>> file_map_{};
	off_t                                             max_file_size_{ 1024u * 1024u * 1024u };

	static constexpr auto file_id_increment = static_cast<file_id_type>(1) << (file_id_bits / 2);
	static constexpr auto file_id_mask      = std::numeric_limits<file_id_type>::max() << (file_id_bits / 2);

	datafile* add_file(std::unique_ptr<datafile>&& file)
	{
		return this->file_map_.insert_or_assign(file->id(), std::move(file)).first->second.get();
	}

	void open(const std::set<std::string>& names)
	{
		this->file_map_.clear();

		// The set is ordered alphabetically, meaning the last file in the set is the most recent file, i.e. the active file.
		for (auto it = names.begin(); it != names.end();)
		{
			const auto& name    = *it;
			const auto  path    = this->directory_ / name;
			const auto  is_last = (++it == names.end());
			this->add_file(std::make_unique<datafile>(file::open(path, is_last ? O_RDWR : O_RDONLY, 0664)));
		}

		if (this->file_map_.empty())
		{
			this->add_file(std::make_unique<datafile>(file::open(this->directory_ / datafile::make_filename(0u), O_RDWR | O_CREAT, 0664)));
		}
	}

	datafile& active_file()
	{
		{
			assert(!this->file_map_.empty());
			auto& active = *this->file_map_.rbegin()->second;
			if (active.size_greater_than(this->max_file_size_))
			{
				active.reopen(O_RDONLY, 0664);
				this->add_file(std::make_unique<datafile>(file::open(
				    this->directory_ / datafile::make_filename((active.id() + file_id_increment) & file_id_mask), O_RDWR | O_CREAT, 0664)));
			}
		}

		return *this->file_map_.rbegin()->second;
	}

public:
	explicit impl(const fs::path& directory)
	    : directory_{ directory }
	{
		if (fs::exists(directory))
		{
			if (!fs::is_directory(directory))
			{
				throw std::system_error{ std::error_code{ ENOTDIR, std::system_category() }, directory.string() };
			}
		}
		else
		{
			fs::create_directories(directory);
		}

		// Only one instance may exist
		this->lockfile_ = std::make_unique<lockfile>(directory / "LOCK");

		// Scan directory for data files
		auto names = std::set<std::string>{};
		for (const auto& entry : fs::directory_iterator(directory))
		{
			if (entry.is_regular_file() && std::regex_match(entry.path().filename().string(), datafile::name_regex))
			{
				names.insert(entry.path().filename().string());
			}
		}

		// Open all data files
		this->open(names);
	}

	off64_t max_file_size() const
	{
		return this->max_file_size_;
	}

	void max_file_size(off64_t size)
	{
		this->max_file_size_ = size;
	}

	void build_keydir(keydir& kd)
	{
		for (auto& pair : this->file_map_)
		{
			pair.second->build_keydir(kd);
		}
	}

	value_type get(const keydir::info& info)
	{
		const auto it = this->file_map_.find(info.file_id);
		if (it == this->file_map_.end())
		{
			throw std::runtime_error{ fmt::format("Unknown file_id {}", info.file_id) };
		}
		return it->second->get(info);
	}

	keydir::info put(const std::string_view& key, const std::string_view& value, version_type version)
	{
		return this->active_file().put(key, value, version);
	}

	void del(const std::string_view& key, version_type version)
	{
		return this->active_file().del(key, version);
	}

	void merge(keydir& kd)
	{
		if (this->file_map_.size() < 2u)
		{
			return;
		}

		auto immutable_files = std::vector<datafile*>{};
		std::transform(this->file_map_.begin(),
		               std::prev(this->file_map_.end()),
		               std::back_inserter(immutable_files),
		               [](const auto& pair) { return pair.second.get(); });

		auto merged_file_id = immutable_files.back()->id();

		datafile* merged_file{ nullptr };
		auto      hint_file = std::unique_ptr<hintfile>{};

		std::for_each(immutable_files.begin(), immutable_files.end(), [&](auto file) {
			file->traverse([&](const auto& rec) {
				if (rec.value)
				{
					const auto& v        = rec.value.value();
					auto        key_info = kd.get_mutable(rec.key);
					if (key_info && key_info->version == v.version)
					{
						if (!merged_file)
						{
							merged_file = this->add_file(std::make_unique<datafile>(
							    file::open(this->directory_ / datafile::make_filename(++merged_file_id), O_RDWR | O_CREAT, 0664)));
							hint_file   = std::make_unique<hintfile>(file::open(merged_file->hint_path(), O_WRONLY | O_CREAT, 0664));
						}

						*key_info = merged_file->put(rec.key, v.value, v.version);

						hint_file->put(hintfile::hint{ .version   = key_info->version,
						                               .value_sz  = key_info->value_sz,
						                               .value_pos = key_info->value_pos,
						                               .key       = rec.key });

						if (merged_file->size_greater_than(this->max_file_size_))
						{
							merged_file->reopen(O_RDONLY, 0664);
							merged_file = nullptr;
						}
					}
				}
			});

			const auto path      = file->path();
			const auto hint_path = file->hint_path();

			this->file_map_.erase(file->id());

			fs::remove(path);
			if (fs::exists(hint_path))
			{
				fs::remove(hint_path);
			}
		});
	}

	void clear()
	{
		auto paths = std::vector<fs::path>{};

		std::for_each(this->file_map_.begin(), this->file_map_.end(), [&](const auto& pair) {
			const auto& file = pair.second;
			paths.push_back(file->path());
			paths.push_back(file->hint_path());
		});

		this->file_map_.clear();

		std::for_each(paths.begin(), paths.end(), [](const auto& path) {
			if (fs::exists(path))
			{
				fs::remove(path);
			}
		});

		this->open({});
	}
};

datadir::datadir(const fs::path& directory)
    : pimpl_{ std::make_unique<impl>(directory) }
{
}

off64_t datadir::max_file_size() const
{
	return this->pimpl_->max_file_size();
}

void datadir::max_file_size(off64_t size)
{
	return this->pimpl_->max_file_size(size);
}

datadir::~datadir() noexcept
{
}

void datadir::build_keydir(keydir& kd)
{
	this->pimpl_->build_keydir(kd);
}

value_type datadir::get(const keydir::info& info)
{
	return this->pimpl_->get(info);
}

keydir::info datadir::put(const std::string_view& key, const std::string_view& value, version_type version)
{
	return this->pimpl_->put(key, value, version);
}

void datadir::del(const std::string_view& key, version_type version)
{
	return this->pimpl_->del(key, version);
}

void datadir::merge(keydir& kd)
{
	return this->pimpl_->merge(kd);
}

void datadir::clear()
{
	return this->pimpl_->clear();
}
