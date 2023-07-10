#pragma once

#include <string_view>

template<size_t max_length>
struct FixedString {
	constexpr FixedString() = default;
	constexpr FixedString(const FixedString&) = default;
	constexpr FixedString(FixedString&&) = default;
	
	constexpr FixedString(const char* string, size_t length)
		: len(max_length >= length ? length : max_length)
	{
		for (size_t i = 0; i < len; ++i)
			str[i] = string[i];
		for (size_t i = len; i < max_length; ++i)
			str[i] = 0;
	}
	
	constexpr ~FixedString() = default;
	
	constexpr FixedString& operator=(const FixedString&) = default;
	constexpr FixedString& operator=(FixedString&&) = default;
	
	constexpr operator std::string_view() const {
		return std::string_view(str, len);
	}
	
	char str[max_length];
	size_t len;
};

constexpr FixedString<256> operator""_cstr(const char* str, size_t len) {
	return FixedString<256>(str, len);
}
