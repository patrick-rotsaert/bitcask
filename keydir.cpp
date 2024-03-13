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
		return &it->second; // FIXME? probably not safe
	}
}

bool keydir::put(const std::string_view& key, keydir::info&& info)
{
	if (info.version > this->version_)
	{
		this->version_ = info.version;
	}
	return this->map_.insert_or_assign(std::string{ key }, std::move(info)).second;
}

bool keydir::del(const std::string_view& key)
{
	const auto it = this->map_.find(key);
	if (it == this->map_.end())
	{
		return false;
	}
	else
	{
		this->map_.erase(it);
		return true;
	}
}

void keydir::clear()
{
	this->map_.clear();
}

bool keydir::traverse(std::function<bool(const std::string_view& key, const info& info)> callback)
{
	for (const auto& pair : this->map_)
	{
		if (!callback(pair.first, pair.second))
		{
			return false;
		}
	}
	return true;
}
