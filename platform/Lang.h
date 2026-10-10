#pragma once

#include <windows.h>
#include <string>
#include <vector>

namespace spy
{

struct LangPackInfo
{
	std::wstring code; // e.g. zh-CN / en-US
	std::wstring name; // display name in combo
	std::wstring path;
};

/** Load preference + language pack. Call once at startup (after chdir to exe). */
bool LangInit();

/** Translate by key. Falls back to English builtin, then key itself. */
const wchar_t* Tr(const wchar_t* key);

/** Non-const pointer for XCGUI APIs that take wchar_t* (same string as Tr). */
inline wchar_t* TrMut(const wchar_t* key)
{
	return const_cast<wchar_t*>(Tr(key));
}

/** preference: empty = system default; otherwise pack code like L"en-US" */
std::wstring LangGetPreference();
bool LangSetPreference(const std::wstring& codeOrEmpty);

std::wstring LangCurrentCode();
std::wstring LangCurrentName();

/** Enumerate Lang\*.lng next to exe (Everything-style). */
std::vector<LangPackInfo> LangEnumerate();

/** App settings dir (LocalAppData\YouYouJieTu) — not localized. */
std::wstring GetAppDataDir();

bool LoadAppSettings(bool& hideOnCapture);
bool SaveAppSettings(bool hideOnCapture);

} // namespace spy
