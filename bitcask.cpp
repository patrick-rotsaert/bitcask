#include "bitcask.h"
#include "datadir.h"
#include "keydir.h"

class bitcask::impl
{
	datadir datadir_;
	keydir  keydir_;

public:
	explicit impl(const std::filesystem::path& directory)
	    : datadir_{ directory }
	    , keydir_{}
	{
		this->datadir_.build_keydir(this->keydir_);
	}

	off64_t max_file_size() const
	{
		return this->datadir_.max_file_size();
	}

	void max_file_size(off64_t size)
	{
		return this->datadir_.max_file_size(size);
	}

	std::optional<value_type> get(const std::string_view& key)
	{
		const auto info = this->keydir_.get(key);
		if (info)
		{
			return this->datadir_.get(*info);
		}
		else
		{
			return std::nullopt;
		}
	}

	void put(const std::string_view& key, const std::string_view& value)
	{
		this->keydir_.put(key, this->datadir_.put(key, value, this->keydir_.next_version()));
	}

	void del(const std::string_view& key)
	{
		this->datadir_.del(key, this->keydir_.next_version());
		this->keydir_.del(key);
	}

	void merge()
	{
		return this->datadir_.merge(this->keydir_);
	}
};

bitcask::bitcask(const std::filesystem::path& directory)
    : pimpl_{ std::make_unique<impl>(directory) }
{
}

off64_t bitcask::max_file_size() const
{
	return this->pimpl_->max_file_size();
}

void bitcask::max_file_size(off64_t size)
{
	return this->pimpl_->max_file_size(size);
}

bitcask::~bitcask() noexcept
{
}

std::optional<value_type> bitcask::get(const std::string_view& key)
{
	return this->pimpl_->get(key);
}

void bitcask::put(const std::string_view& key, const std::string_view& value)
{
	return this->pimpl_->put(key, value);
}

void bitcask::del(const std::string_view& key)
{
	return this->pimpl_->del(key);
}

void bitcask::merge()
{
	return this->pimpl_->merge();
}
