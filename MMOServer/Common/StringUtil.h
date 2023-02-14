#ifndef __STRING_UTIL__H_
#define __STRING_UTIL__H_
#include <string>

namespace Jay
{
	inline void MultiByteToUnicode() = delete;
	inline void UnicodeToMultiByte(const wchar_t* unicode, char* multibyte)
	{
		int len = wcslen(unicode) + 1;
		int size = WideCharToMultiByte(CP_UTF8, 0, unicode, len, NULL, 0, NULL, NULL);
		WideCharToMultiByte(CP_UTF8, 0, unicode, len, multibyte, size, NULL, NULL);
	}
	inline void MultiByteToWString(const char* multibyte, std::wstring& wstr)
	{
		int len = strlen(multibyte) + 1;
		int size = MultiByteToWideChar(CP_ACP, 0, multibyte, len, NULL, 0);
		wstr.resize(size - 1);
		MultiByteToWideChar(CP_ACP, 0, multibyte, len, &wstr[0], size);
	}
	inline void UnicodeToString(const wchar_t* unicode, std::string& str)
	{
		int len = wcslen(unicode) + 1;
		int size = WideCharToMultiByte(CP_UTF8, 0, unicode, len, NULL, 0, NULL, NULL);
		str.resize(size - 1);
		WideCharToMultiByte(CP_UTF8, 0, unicode, len, &str[0], size, NULL, NULL);
	}
}

#endif
