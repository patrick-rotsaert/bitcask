#include "keydir.h"
#include <stdexcept>

keydir::keydir()
    : map_{}
    , version_{}
{
}

version_type keydir::next_version()
{
	return ++this->version_;
}

const keydir::info* keydir::get(const std::string_view& key) const
{
	const auto it = this->map_.find(key);
	if (it == this->map_.end())
	{
		return nullptr;
	}
	else
	{
		return &it->second;
	}
}

void keydir::put(const std::string_view& key, keydir::info&& info)
{
	if (info.version > this->version_)
	{
		this->version_ = info.version;
	}
	this->map_.insert_or_assign(std::string{ key }, std::move(info));
}

void keydir::del(const std::string_view& key)
{
	const auto it = this->map_.find(key);
	if (it != this->map_.end())
	{
		this->map_.erase(it);
	}
}

void keydir::clear()
{
	this->map_.clear();
}
