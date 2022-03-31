#pragma once

#include <string>

namespace mb
{
	namespace HttpResponseCodes
	{
		constexpr inline int OK = 200;
	}

	namespace common_http_headers
	{
		constexpr static inline std::string_view USER_AGENT = "User-Agent";
		constexpr static inline std::string_view ACCEPT = "Accept";
		constexpr static inline std::string_view CONTENT_TYPE = "Content-Type";
		constexpr static inline std::string_view APPLICATION_JSON = "application/json";
		constexpr static inline std::string_view APPLICATION_URLENCODED = "application/x-www-form-urlencoded";
	}

	enum class http_verb
	{
		GET,
		POST,
		HTTP_DELETE
	};

	constexpr std::string to_string(http_verb verb)
	{
		switch (verb)
		{
		case http_verb::GET:
			return "GET";
		case http_verb::POST:
			return "POST";
		case http_verb::HTTP_DELETE:
			return "DELETE";
		}

		throw std::invalid_argument("HTTP verb not recognized");
	}
}
