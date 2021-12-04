/**
 * Copyright (C) 2016 Jessica James.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Written by Jessica James <jessica.aj@outlook.com>
 */

#if !defined _HTTP_QUERYSTRING_H_HEADER
#define _HTTP_QUERYSTRING_H_HEADER

#include <unordered_map>
#include <charconv>

/**
 * @file HTTP_QueryString.h
 * @brief Provides parsing for HTTP Query Strings.
 */

namespace Jupiter
{
	namespace HTTP
	{
		/**
		* @brief Provides parsing for HTML form responses.
		* Note: This is essentially the same thing as QueryString, except key/value pairs are pared into 'table'.
		*/
		class HTMLFormResponse
		{
		public:
			HTMLFormResponse() = delete;
			inline HTMLFormResponse(std::string_view query_string) : HTMLFormResponse(query_string.data(), query_string.size()) {}
			inline HTMLFormResponse(const char *ptr, size_t str_size);
			using TableType = std::unordered_map<std::string, std::string, jessilib::text_hash, jessilib::text_equal>;
#ifdef __cpp_lib_generic_unordered_lookup
			using InKeyType = std::string_view;
#else // We can't use std::string_view for InKeyType until GCC 11 & clang 12, and I still want to support GCC 9
			using InKeyType = const std::string&;

			template<typename CastT>
			CastT tableGetCast(std::string_view in_key, const CastT &in_value) const {
				return tableGetCast<CastT>(static_cast<std::string>(in_key), in_value);
			}

			std::string_view tableGet(std::string_view in_key, std::string_view in_value) const {
				return tableGet(static_cast<std::string>(in_key), in_value);
			}
#endif // __cpp_lib_generic_unordered_lookup

			template<typename CastT>
			CastT tableGetCast(InKeyType in_key, const CastT &in_value) const {
				CastT result = in_value;
				auto item = table.find(in_key);
				if (item != table.end()) {
					std::from_chars(item->second.data(), item->second.data() + item->second.size(), result);
					return result;
				}

				return result;
			}

			auto tableFind(InKeyType in_key) const {
				return table.find(in_key);
			}

#ifndef __cpp_lib_generic_unordered_lookup
			auto tableFind(std::string_view in_key) const {
				return tableFind(static_cast<std::string>(in_key));
			}
#endif // __cpp_lib_generic_unordered_lookup

			std::string_view tableGet(InKeyType in_key, std::string_view in_value) const {
				auto item = table.find(in_key);
				if (item != table.end()) {
					return item->second;
				}

				return in_value;
			}

			TableType table;
			std::string m_parsed;
		};
	}
}

/** Implementation */

inline Jupiter::HTTP::HTMLFormResponse::HTMLFormResponse(const char *ptr, size_t str_size) {
	std::string_view in_view{ ptr, str_size };
	if (str_size < 3) { // not enough room for "%XX", therefore no escaped characters to parse
		m_parsed = in_view;
		return;
	}
	m_parsed.resize(in_view.size());

	const char *end = ptr + str_size - 2;
	char* buf = m_parsed.data();
	const char *token_start = buf;
	int val;
	std::string key;

	while (ptr != end)
	{
		if (*ptr == '%')
		{
			if ((val = Jupiter_getHex(*++ptr)) != -1)
			{
				*buf = static_cast<uint8_t>(val) << 4;
				if ((val = Jupiter_getHex(*++ptr)) != -1)
					*buf |= val;

				++buf, ++ptr;
				if (ptr > end)
				{
					if (ptr == end + 1) // copy 1 remaining character
					{
						*buf = *ptr;
						++buf;
					}
					m_parsed.erase(buf - m_parsed.data());
					return;
				}
			}
		}
		else if (*ptr == '&') // End of key/value, start of key
		{
			if (!key.empty()) { // A key was already set; end of value
				Jupiter::HTTP::HTMLFormResponse::table[key] = std::string_view(token_start, buf - token_start);
			}

			key = std::string_view{};
			++buf, ++ptr;
			token_start = buf;
		}
		else if (*ptr == '=') // End of key, start of value
		{
			key = std::string_view(token_start, buf - token_start);
			++buf, ++ptr;
			token_start = buf;
		}
		else // Copy character
		{
			*buf = *ptr;
			++buf, ++ptr;
		}
	}

	// copy last 2 characters
	*buf = *ptr;

	if (*buf == '=') // End of key, start of value
	{
		key = std::string_view(token_start, ++buf - token_start);
		*buf = *++ptr;
		Jupiter::HTTP::HTMLFormResponse::table[key] = std::string_view(ptr, 1);
	}
	else
		*++buf = *++ptr;

	if (!key.empty()) // A key was already set; end of value
		Jupiter::HTTP::HTMLFormResponse::table[key] = std::string_view(token_start, buf - token_start + 1);

	m_parsed.erase(buf + 1 - m_parsed.data()); // TODO: double check where this +1 came from? actually just replace this
}

#endif // _HTTP_QUERYSTRING_H_HEADER