#include "datadir.h"
#include "datafile.h"
#include "hintfile.h"
#include "keydir.h"
#include "file.h"

#include <fmt/format.h>

#include <system_error>
#include <regex>
#include <set>
#include <vector>
#include <map>
#include <algorithm>
#include <cassert>

#include <fcntl.h>

namespace fs = std::filesystem;

class datadir::impl final
{
	fs::path                               directory_;
	std::vector<std::unique_ptr<datafile>> files_;
	std::map<file_id_type, datafile*>      id_map_;
	off_t                                  max_file_size_ = 1024u * 1024u * 1024u;

	void push_back_file(std::unique_ptr<datafile>&& file)
	{
		this->id_map_[file->id()] = file.get();
		this->files_.push_back(std::move(file));
	}

	void open(const std::set<std::string>& names)
	{
		this->files_.clear();
		this->id_map_.clear();

		// The set is ordered alphabetically, meaning the last file in the set is the most recent file, i.e. the active file.
		for (auto it = names.begin(); it != names.end();)
		{
			const auto& name    = *it;
			const auto  path    = this->directory_ / name;
			const auto  is_last = (++it == names.end());
			this->push_back_file(std::make_unique<datafile>(file::open(path, is_last ? O_RDWR : O_RDONLY, 0664)));
		}

		if (this->files_.empty())
		{
			this->push_back_file(
			    std::make_unique<datafile>(file::open(this->directory_ / datafile::make_filename(0u), O_RDWR | O_CREAT, 0664)));
		}
	}

	datafile& active_file()
	{
		{
			assert(!this->files_.empty());
			auto& active = *this->files_.back();
			if (active.get_file().size() >= this->max_file_size_)
			{
				active.get_file().reopen(O_RDONLY, 0664);
				this->push_back_file(std::make_unique<datafile>(
				    file::open(this->directory_ / datafile::make_filename(active.id() + 1), O_RDWR | O_CREAT, 0664)));
			}
		}

		return *this->files_.back();
	}

public:
	explicit impl(const fs::path& directory)
	    : directory_{ directory }
	    , files_{}
	    , id_map_{}
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

		// TODO: create a lock file

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
		for (auto& file : this->files_)
		{
			file->build_keydir(kd);
		}
	}

	value_type get(const keydir::info& info)
	{
		const auto it = this->id_map_.find(info.file_id);
		if (it == this->id_map_.end())
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
		if (this->files_.size() < 2u)
		{
			return;
		}

		const auto merge_directory = this->directory_ / "merge";

		if (fs::exists(merge_directory))
		{
			fs::remove_all(merge_directory);
		}
		fs::create_directories(merge_directory);

		const auto begin = this->files_.begin();
		const auto end   = this->files_.end() - 1u;
		assert(end > begin);

		auto dd = datadir{ merge_directory };
		auto hf = std::unique_ptr<hintfile>{};

		std::for_each(begin, end, [&](const auto& file) {
			file->traverse([&](const auto& rec) {
				if (rec.value)
				{
					const auto& v        = rec.value.value();
					const auto  key_info = kd.get(rec.key);
					if (key_info && key_info->version == v.version)
					{
						const auto  info = dd.put(rec.key, v.value, v.version);
						const auto& df   = dd.pimpl_->active_file();
						if (!hf || hf->path() != df.hint_path())
						{
							hf = std::make_unique<hintfile>(file::open(df.hint_path(), O_WRONLY | O_CREAT, 0664));
						}
						hf->put(hintfile::hint{
						    .version = info.version, .value_sz = info.value_sz, .value_pos = info.value_pos, .key = rec.key });
					}
				}
			});
		});

		// close the last hint file
		hf.reset();

		const auto last_merged_file_id = dd.pimpl_->files_.back()->id();

		auto my_file_names = std::vector<std::string>{};
		std::transform(this->files_.begin(), this->files_.end(), std::back_inserter(my_file_names), [](const auto& file) {
			return file->path().filename().string();
		});

		auto merged_file_names = std::vector<std::string>{};
		std::transform(dd.pimpl_->files_.begin(), dd.pimpl_->files_.end(), std::back_inserter(merged_file_names), [](const auto& file) {
			return file->path().filename().string();
		});

		auto hint_file_names = std::vector<std::string>{};
		std::transform(dd.pimpl_->files_.begin(), dd.pimpl_->files_.end(), std::back_inserter(hint_file_names), [](const auto& file) {
			return file->hint_path().filename().string();
		});

		// TODO: lock me

		// close all files
		this->files_.clear();
		dd.pimpl_->files_.clear();

		// remove all of my files except the last, i.e. all immutable files
		std::for_each(
		    my_file_names.begin(), my_file_names.end() - 1u, [&](const auto& file_name) { fs::remove(this->directory_ / file_name); });

		// rename my active file to id of last merged file + 1
		const auto new_active_file_name = datafile::make_filename(last_merged_file_id + 1);
		fs::rename(this->directory_ / my_file_names.back(), this->directory_ / new_active_file_name);

		// move all merged files
		std::for_each(merged_file_names.begin(), merged_file_names.end(), [&](const auto& file_name) {
			fs::rename(merge_directory / file_name, this->directory_ / file_name);
		});

		// move all hint files
		std::for_each(hint_file_names.begin(), hint_file_names.end(), [&](const auto& file_name) {
			fs::rename(merge_directory / file_name, this->directory_ / file_name);
		});

		fs::remove(merge_directory);

		auto names = std::set<std::string>{ merged_file_names.begin(), merged_file_names.end() };
		names.insert(new_active_file_name);

		this->open(names);

		kd.clear();
		this->build_keydir(kd);
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
