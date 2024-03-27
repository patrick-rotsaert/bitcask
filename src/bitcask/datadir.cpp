//
// Copyright (C) 2024 Patrick Rotsaert
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "bitcask/datadir.h"
#include "bitcask/datafile.h"
#include "bitcask/hintfile.h"
#include "bitcask/keydir.h"
#include "bitcask/file.h"
#include "bitcask/lockfile.h"
#include "bitcask/locktypes.hpp"

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

namespace bitcask {

namespace fs = std::filesystem;

namespace {

std::set<std::string> scan_data_files(const fs::path& directory)
{
	auto names = std::set<std::string>{};
	for (const auto& entry : fs::directory_iterator(directory))
	{
		if (entry.is_regular_file() && std::regex_match(entry.path().filename().string(), datafile::name_regex))
		{
			names.insert(entry.path().filename().string());
		}
	}
	return names;
}

std::unique_ptr<lockfile> lock_directory(const fs::path& directory)
{
	return std::make_unique<lockfile>(directory / "LOCK");
}

fs::path ensure_directory(const fs::path& directory)
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
	return directory;
}

void remove_if_exists(const fs::path& path)
{
	if (fs::exists(path))
	{
		fs::remove(path);
	}
}

} // namespace

class datadir::impl final
{
	static constexpr auto file_id_increment = static_cast<file_id_type>(1) << (file_id_bits / 2);
	static constexpr auto file_id_mask      = std::numeric_limits<file_id_type>::max() << (file_id_bits / 2);

	fs::path                                          directory_{};
	std::unique_ptr<lockfile>                         lockfile_{};
	std::map<file_id_type, std::unique_ptr<datafile>> file_map_{};
	off_t                                             max_file_size_{ 1024u * 1024u * 1024u };
	mutable shared_locker                             locker_{};
	mutable locker                                    merge_locker_{};

	datafile* add_file(const write_lock_type&, std::unique_ptr<datafile>&& file)
	{
		return this->file_map_.insert_or_assign(file->id(), std::move(file)).first->second.get();
	}

	datafile& active_file(const write_lock_type& lock)
	{
		{
			assert(!this->file_map_.empty());
			auto& active = *this->file_map_.rbegin()->second;
			if (active.size_greater_than(this->max_file_size_))
			{
				active.reopen(O_RDONLY, 0664);
				this->add_file(lock,
				               std::make_unique<datafile>(
				                   file::open(this->directory_ / datafile::make_filename((active.id() + file_id_increment) & file_id_mask),
				                              O_RDWR | O_CREAT,
				                              0664)));
			}
		}

		return *this->file_map_.rbegin()->second;
	}

public:
	explicit impl(const fs::path& directory)
	    : directory_{ ensure_directory(directory) }
	    , lockfile_{ lock_directory(directory) }
	{
		// Scan directory for data files
		auto names = scan_data_files(directory);

		// Open all data files
		// The set is ordered alphabetically, meaning the last file in the set is the most recent file, i.e. the active file.
		const auto lock = this->locker_.write_lock();
		for (auto it = names.begin(); it != names.end();)
		{
			const auto& name    = *it;
			const auto  path    = this->directory_ / name;
			const auto  is_last = (++it == names.end());

			this->add_file(lock, std::make_unique<datafile>(file::open(path, is_last ? O_RDWR : O_RDONLY, 0664)));
		}

		if (this->file_map_.empty())
		{
			this->add_file(lock,
			               std::make_unique<datafile>(file::open(this->directory_ / datafile::make_filename(0u), O_RDWR | O_CREAT, 0664)));
		}
	}

	off64_t max_file_size() const
	{
		const auto lock = this->locker_.read_lock();
		(void)(lock);

		return this->max_file_size_;
	}

	void max_file_size(off64_t size)
	{
		const auto lock = this->locker_.write_lock();
		(void)(lock);

		this->max_file_size_ = size;
	}

	void build_keydir(keydir& kd)
	{
		const auto lock = this->locker_.read_lock();
		(void)(lock);

		for (auto& pair : this->file_map_)
		{
			pair.second->build_keydir(kd);
		}
	}

	value_type get(const keydir::info& info)
	{
		const auto lock = this->locker_.read_lock();
		(void)(lock);

		const auto it = this->file_map_.find(info.file_id);
		if (it == this->file_map_.end())
		{
			throw std::runtime_error{ fmt::format("Unknown file_id {}", info.file_id) };
		}
		return it->second->get(info);
	}

	keydir::info put(const std::string_view& key, const std::string_view& value, version_type version)
	{
		return this->active_file(this->locker_.write_lock()).put(key, value, version);
	}

	void del(const std::string_view& key, version_type version)
	{
		return this->active_file(this->locker_.write_lock()).del(key, version);
	}

	void merge(keydir& kd)
	{
		auto rlock = this->locker_.read_lock();

		if (this->file_map_.size() < 2u)
		{
			return;
		}

		// one merge at a time!
		const auto lock = this->merge_locker_.lock();
		(void)(lock);

		auto immutable_files = std::vector<datafile*>{};
		std::transform(this->file_map_.begin(),
		               std::prev(this->file_map_.end()),
		               std::back_inserter(immutable_files),
		               [](const auto& pair) { return pair.second.get(); });

		rlock.unlock();

		auto last_immutable_file_id = immutable_files.back()->id();

		datafile* merged_file{ nullptr };
		auto      hint_file = std::unique_ptr<hintfile>{};

		std::for_each(immutable_files.begin(), immutable_files.end(), [&](auto file) {
			file->traverse([&](const auto& rec) {
				if (rec.value)
				{
					const auto& v            = rec.value.value();
					const auto  opt_key_info = kd.get_mutable(rec.key);
					if (opt_key_info)
					{
						auto key_info = opt_key_info.value().first;
						if (key_info->version == v.version)
						{
							if (!merged_file)
							{
								merged_file = this->add_file(
								    this->locker_.write_lock(),
								    std::make_unique<datafile>(file::open(
								        this->directory_ / datafile::make_filename(++last_immutable_file_id), O_RDWR | O_CREAT, 0664)));
								hint_file = std::make_unique<hintfile>(file::open(merged_file->hint_path(), O_WRONLY | O_CREAT, 0664));
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
				}
			});

			const auto path      = file->path();
			const auto hint_path = file->hint_path();

			const auto wlock = this->locker_.write_lock();
			(void)(wlock);

			this->file_map_.erase(file->id());

			fs::remove(path);
			remove_if_exists(hint_path);
		});
	}

	static void clear(const std::filesystem::path& directory)
	{
		if (fs::is_directory(directory))
		{
			const auto lockfile = lock_directory(directory);
			for (const auto& name : scan_data_files(directory))
			{
				const auto path = directory / name;
				fs::remove(path);
				remove_if_exists(datafile::hint_path(path));
			}
		}
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

void datadir::clear(const std::filesystem::path& directory)
{
	return impl::clear(directory);
}

} // namespace bitcask
