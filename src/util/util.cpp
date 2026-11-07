#include "util.h"

#include <windows.h>

#include <cstdarg>
#include <cwchar>

std::wstring Fmt(const wchar_t* fmt, ...) {
    wchar_t buf[512];
    va_list ap;
    va_start(ap, fmt);
    _vsnwprintf(buf, 511, fmt, ap);
    va_end(ap);
    buf[511] = 0;
    return std::wstring(buf);
}

std::wstring AnsiToWide(const char* s) {
    if (!s || !*s) return L"";
    int n = MultiByteToWideChar(CP_ACP, 0, s, -1, nullptr, 0);
    if (n <= 0) return L"";
    std::wstring w((size_t)n - 1, L'\0');
    MultiByteToWideChar(CP_ACP, 0, s, -1, &w[0], n);
    return w;
}

std::wstring TrimW(std::wstring s) {
    while (!s.empty() &&
           (s.back() == L' ' || s.back() == L'\r' || s.back() == L'\n' || s.back() == L'\0')) {
        s.pop_back();
    }
    size_t i = 0;
    while (i < s.size() && (s[i] == L' ' || s[i] == L'\r' || s[i] == L'\n')) ++i;
    return s.substr(i);
}

void CopyTip(wchar_t* dst, size_t n, const wchar_t* src) {
    size_t i = 0;
    for (; i + 1 < n && src && src[i]; ++i) dst[i] = src[i];
    dst[i] = 0;
}
