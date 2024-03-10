#pragma once

#include "basic_types.h"

#include <string_view>
#include <string>
#include <unordered_map>

#include <sys/types.h>

struct keydir_info final
{
	file_id_type   file_id;
	value_sz_type  value_sz;
	value_pos_type value_pos;
	version_type   version;
};

class keydir final
{
	struct string_hash
	{
		using is_transparent = void;

		std::size_t operator()(const std::string& v) const
		{
			return std::hash<std::string>{}(v);
		}

		std::size_t operator()(const std::string_view& v) const
		{
			return std::hash<std::string_view>{}(v);
		}

		std::size_t operator()(const char* v) const
		{
			return std::hash<std::string_view>{}(v);
		}
	};

	std::unordered_map<key_type, keydir_info, string_hash, std::equal_to<>> map_;
	version_type                                                            version_;

public:
	using info = keydir_info;

	keydir();

	version_type next_version();

	const info* get(const std::string_view& key) const;
	void        put(const std::string_view& key, info&& info);
	void        del(const std::string_view& key);
};
