// ============================================================================
//  util/util.h — 与业务无关的小工具（字符串格式化 / 编码转换 / 定长拷贝）
// ============================================================================
#pragma once

#include <cstddef>
#include <string>

// 宽字符 printf 包装，返回 std::wstring。
std::wstring Fmt(const wchar_t* fmt, ...);

// ANSI（系统代码页）→ 宽字符。磁盘型号、SDK 返回的字符串都是 ANSI。
std::wstring AnsiToWide(const char* s);

// 去掉首尾空格 / CR / LF / 结尾 NUL（设备描述符里的字段常常带尾随空格）
std::wstring TrimW(std::wstring s);

// 安全拷进定长缓冲区（NOTIFYICONDATA 的 szTip / szInfo 等）
void CopyTip(wchar_t* dst, size_t n, const wchar_t* src);
