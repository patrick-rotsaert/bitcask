#include "datadir.h"
#include "datafile.h"
#include "keydir.h"
#include "file.h"

#include <fmt/format.h>

#include <system_error>
#include <regex>
#include <set>
#include <vector>
#include <map>
#include <cassert>

#include <fcntl.h>

namespace fs = std::filesystem;

class datadir::impl final
{
	fs::path                               directory_;
	fs::path                               merge_directory_;
	std::vector<std::unique_ptr<datafile>> files_;
	std::map<file_id_type, datafile*>      id_map_;
	off_t                                  max_file_size_;

	void push_back_file(std::unique_ptr<datafile>&& file)
	{
		this->id_map_[file->id()] = file.get();
		this->files_.push_back(std::move(file));
	}

	datafile& active_file() const
	{
		assert(!this->files_.empty());
		return *this->files_.back();
	}

	std::vector<datafile*> immutable_files() const
	{
		auto files = std::vector<datafile*>{};
		if (this->files_.size() > 1u)
		{
			for (auto it = this->files_.begin(), end = this->files_.end() - 1; it != end; ++it)
			{
				files.push_back(it->get());
			}
		}
		return files;
	}

	void close_active_file_if_necessary()
	{
		auto& active = this->active_file();
		if (active.get_file().size() >= this->max_file_size_)
		{
			active.get_file().reopen(O_RDONLY, 0664);
			this->push_back_file(std::make_unique<datafile>(
			    file::open(this->directory_ / datafile::make_filename(active.id() + 1), O_RDWR | O_CREAT, 0664)));
		}
	}

public:
	explicit impl(const fs::path& directory)
	    : directory_{ directory }
	    , merge_directory_{ directory / "merge" }
	    , files_{}
	    , id_map_{}
	    , max_file_size_{ 1024 } // TODO: parameterize
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

		if (fs::exists(this->merge_directory_))
		{
			// Remove stale merge directory
			fs::remove_all(this->merge_directory_);
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

		// Iterate over the data files and open all of them.
		// The set is ordered alphabetically, meaning the last file in the set is the most recent file, i.e. the active file.
		for (auto it = names.begin(); it != names.end();)
		{
			const auto& name    = *it;
			const auto  path    = directory / name;
			const auto  is_last = (++it == names.end());
			this->push_back_file(std::make_unique<datafile>(file::open(path, is_last ? O_RDWR : O_RDONLY, 0664)));
		}

		if (this->files_.empty())
		{
			this->push_back_file(std::make_unique<datafile>(file::open(directory / datafile::make_filename(0u), O_RDWR | O_CREAT, 0664)));
		}
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
		// TODO: investigate if this should be called from layer above,
		// so that this isn't done before each and every write.
		// The cost of this operation is a seek to EOF, not sure how big this cost is...
		this->close_active_file_if_necessary();

		return this->active_file().put(key, value, version);
	}

	void del(const std::string_view& key, version_type version)
	{
		// TODO: investigate if this should be called from layer above,
		// so that this isn't done before each and every write.
		// The cost of this operation is a seek to EOF, not sure how big this cost is...
		this->close_active_file_if_necessary();

		return this->active_file().del(key, version);
	}

	void merge(keydir& kd)
	{
		if (this->files_.size() < 2u)
		{
			return;
		}

		fs::create_directories(this->merge_directory_);

		const auto& last_immutable_file = **(this->files_.end() - 2u);

		const auto merged_file_path = this->merge_directory_ / last_immutable_file.path().filename();
		const auto hint_file_path   = this->merge_directory_ / last_immutable_file.hint_path().filename();

		{
			const auto begin = this->files_.begin();
			const auto end   = this->files_.end() - 1u;

			{
				auto merged_file = datafile{ file::open(merged_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0664) };
				auto hf          = hintfile{ file::open(hint_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0664) };

				std::for_each(begin, end, [&](const auto& file) { file->merge(kd, merged_file, hf); });
			}

			// remove all immutable files
			std::for_each(begin, end, [](const auto& file) { file->remove(); });
			this->files_.erase(begin, end);
		}

		// move the merged data file and hint to the data directory
		const auto new_merged_file_path = this->directory_ / merged_file_path.filename();
		const auto new_hint_file_path   = this->directory_ / hint_file_path.filename();

		fs::rename(merged_file_path, new_merged_file_path);
		fs::rename(hint_file_path, new_hint_file_path);

		// push the merged file to front
		assert(this->files_.size() == 1u);
		this->files_.insert(this->files_.begin(), std::make_unique<datafile>(file::open(merged_file_path, O_RDONLY, 0664)));

		hintfile{ file::open(new_hint_file_path, O_RDONLY, 0664) }.build_keydir(kd, this->files_.front()->id());

		this->id_map_.clear();
		std::for_each(this->files_.begin(), this->files_.end(), [&](const auto& file) { this->id_map_[file->id()] = file.get(); });
	}
};

datadir::datadir(const fs::path& directory)
    : pimpl_{ std::make_unique<impl>(directory) }
{
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
