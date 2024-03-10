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

	std::optional<value_type> get(const std::string_view& key)
	{
		// TODO: lock
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
		// TODO: lock
		this->keydir_.put(key, this->datadir_.put(key, value, this->keydir_.next_version()));
	}

	void del(const std::string_view& key)
	{
		// TODO: lock
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
