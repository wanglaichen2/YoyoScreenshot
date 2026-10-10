#include "Lang.h"
#include <shlobj.h>
#include <stdio.h>
#include <map>
#include <algorithm>

namespace spy
{
namespace
{
std::map<std::wstring, std::wstring> g_strings;
std::wstring g_pref;      // empty = system
std::wstring g_code;      // active pack code
std::wstring g_name;      // active pack display name
std::wstring g_langDir;

struct Builtin
{
	const wchar_t* key;
	const wchar_t* en;
};

// English builtins — always available if .lng missing a key
const Builtin kBuiltin[] = {
	{ L"AppTitle", L"YouYou Screenshot" },
	{ L"Tip", L"Tip" },
	{ L"AlreadyRunning", L"YouYou Screenshot is already running." },
	{ L"TrayBalloonTitle", L"YouYou Screenshot" },
	{ L"TrayBalloonText", L"Running in the system tray." },
	{ L"BtnStartShot", L"Capture" },
	{ L"BtnFullShot", L"Full screen" },
	{ L"BtnOpenFolder", L"Open folder" },
	{ L"StatusHotkeys", L"Hotkeys: Ctrl+Alt+I region / Ctrl+Alt+Shift+I full screen" },
	{ L"MenuEdit", L"Edit" },
	{ L"MenuTools", L"Tools" },
	{ L"MenuHelp", L"Help" },
	{ L"MenuExit", L"Exit" },
	{ L"MenuOpenShots", L"Open shots folder" },
	{ L"MenuStartShot", L"Capture" },
	{ L"MenuFullShot", L"Full screen" },
	{ L"MenuSettings", L"Options" },
	{ L"MenuUsage", L"Help" },
	{ L"MenuAbout", L"About" },
	{ L"UsageTitle", L"Help" },
	{ L"UsageBody", L"YouYou Screenshot\n\n"
		L"- Region capture with pen / rectangle / arrow annotations\n"
		L"- Ctrl+Alt+I region capture\n"
		L"- Ctrl+Alt+Shift+I full screen\n"
		L"- Closing the window minimizes to tray\n"
		L"- History: click to select, double-click to edit, right-click for more\n"
		L"- Files are saved to: " },
	{ L"AboutTitle", L"About" },
	{ L"AboutBody", L"YouYou Screenshot\nVersion: 1.0.6\n" },
	{ L"OpenFolderFail", L"Failed to open shots folder" },
	{ L"OpenFolderOk", L"Opened shots folder: " },
	{ L"StatusHistoryHint", L"Hotkeys Ctrl+Alt+I / Ctrl+Alt+Shift+I · History: click · double-click · right-click" },
	{ L"SettingsTitle", L"Options" },
	{ L"SettingsLang", L"Language:" },
	{ L"SettingsLangSystem", L"System default" },
	{ L"SettingsHide", L"Hide main window to tray while capturing" },
	{ L"SettingsHotkeys", L"Hotkeys (fixed):\nCapture: Ctrl+Alt+I\nFull screen: Ctrl+Alt+Shift+I" },
	{ L"SettingsOk", L"OK" },
	{ L"SettingsCancel", L"Cancel" },
	{ L"SettingsSavedHideOn", L"Set: hide main window while capturing" },
	{ L"SettingsSavedHideOff", L"Set: keep main window while capturing" },
	{ L"SettingsLangRestart", L"Language saved. Restart the app to apply menus fully." },
	{ L"SettingsLangApplied", L"Language applied" },
	{ L"TrayShow", L"Open window" },
	{ L"TrayStartShot", L"Capture" },
	{ L"TrayFullShot", L"Full screen" },
	{ L"TraySettings", L"Options" },
	{ L"TrayExit", L"Exit" },
	{ L"SavedClipboard", L"Saved and copied to clipboard: " },
	{ L"SavedNoClipboard", L"Saved (clipboard copy failed): " },
	{ L"CaptureCancelled", L"Capture cancelled" },
	{ L"FullSavedClipboard", L"Full screen saved and copied: " },
	{ L"FullSavedNoClipboard", L"Full screen saved (clipboard failed): " },
	{ L"FullFailed", L"Full screen capture failed" },
	{ L"ShotTip", L"Hotkeys: Ctrl+Alt+I region | Ctrl+Alt+Shift+I full screen | Enter confirm / Esc cancel\n"
		L"History: click (red) | double-click edit | right-click preview/copy/edit/pin/delete | auto-save + clipboard" },
	{ L"Recent", L"Recent" },
	{ L"DeleteConfirm", L"Delete this screenshot?" },
	{ L"DeleteTitle", L"Delete" },
	{ L"DeleteFail", L"Delete failed (error %lu, file may be in use)" },
	{ L"Deleted", L"Screenshot deleted" },
	{ L"EditSaved", L"Edit saved and copied to clipboard" },
	{ L"EditCancelled", L"Edit cancelled" },
	{ L"PreviewOpened", L"Preview opened" },
	{ L"Pinned", L"Pinned to desktop (drag to move, Esc/right-click to close)" },
	{ L"Selected", L"Selected (double-click edit, right-click for more)" },
	{ L"CtxPreview", L"Preview" },
	{ L"CtxCopy", L"Copy" },
	{ L"CtxEdit", L"Edit" },
	{ L"CtxPin", L"Pin" },
	{ L"CtxDelete", L"Delete" },
	{ L"Copied", L"Copied to clipboard" },
	{ L"CopyFail", L"Copy failed" },
	{ L"OverlayComingSoon", L"This feature will be available in a later version" },
	{ L"OverlayEditTitle", L"Edit capture" },
	{ L"PixelCopyHint", L"Press Q to copy color" },
	{ L"PixelShiftHint", L"Press Shift for RGB/HEX" },
	{ L"PixelCoord", L"Pos: %d, %d" },
};

const wchar_t* BuiltinEn(const wchar_t* key)
{
	for (size_t i = 0; i < sizeof(kBuiltin) / sizeof(kBuiltin[0]); ++i)
	{
		if (wcscmp(kBuiltin[i].key, key) == 0)
		{
			return kBuiltin[i].en;
		}
	}
	return nullptr;
}

std::wstring Trim(const std::wstring& s)
{
	size_t a = 0;
	while (a < s.size() && (s[a] == L' ' || s[a] == L'\t' || s[a] == L'\r')) ++a;
	size_t b = s.size();
	while (b > a && (s[b - 1] == L' ' || s[b - 1] == L'\t' || s[b - 1] == L'\r')) --b;
	return s.substr(a, b - a);
}

std::wstring Utf8ToWide(const std::string& u8)
{
	if (u8.empty()) return std::wstring();
	// 强制按 UTF-8 解码（语言包约定编码）；拒绝系统 ANSI 代码页误解码
	int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, u8.data(), (int)u8.size(), NULL, 0);
	DWORD flags = MB_ERR_INVALID_CHARS;
	if (n <= 0)
	{
		// 兼容偶发非法序列：宽松 UTF-8
		flags = 0;
		n = MultiByteToWideChar(CP_UTF8, 0, u8.data(), (int)u8.size(), NULL, 0);
	}
	if (n <= 0) return std::wstring();
	std::wstring w((size_t)n, 0);
	MultiByteToWideChar(CP_UTF8, flags, u8.data(), (int)u8.size(), &w[0], n);
	return w;
}

std::wstring Unescape(const std::wstring& s)
{
	std::wstring out;
	out.reserve(s.size());
	for (size_t i = 0; i < s.size(); ++i)
	{
		if (s[i] == L'\\' && i + 1 < s.size())
		{
			const wchar_t n = s[++i];
			if (n == L'n') out.push_back(L'\n');
			else if (n == L't') out.push_back(L'\t');
			else if (n == L'\\') out.push_back(L'\\');
			else { out.push_back(L'\\'); out.push_back(n); }
		}
		else out.push_back(s[i]);
	}
	return out;
}

bool ParseLngFile(const std::wstring& path, std::map<std::wstring, std::wstring>& out,
	std::wstring* name, std::wstring* code)
{
	FILE* fp = NULL;
	if (_wfopen_s(&fp, path.c_str(), L"rb") != 0 || !fp)
	{
		return false;
	}
	fseek(fp, 0, SEEK_END);
	const long sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if (sz <= 0)
	{
		fclose(fp);
		return false;
	}
	std::string raw((size_t)sz, '\0');
	fread(&raw[0], 1, (size_t)sz, fp);
	fclose(fp);

	// UTF-8 BOM
	size_t off = 0;
	if (raw.size() >= 3 && (unsigned char)raw[0] == 0xEF && (unsigned char)raw[1] == 0xBB && (unsigned char)raw[2] == 0xBF)
	{
		off = 3;
	}
	// UTF-16 LE BOM
	std::wstring text;
	if (raw.size() >= 2 && (unsigned char)raw[0] == 0xFF && (unsigned char)raw[1] == 0xFE)
	{
		const wchar_t* p = (const wchar_t*)(raw.data() + 2);
		const size_t n = (raw.size() - 2) / sizeof(wchar_t);
		text.assign(p, n);
	}
	else
	{
		text = Utf8ToWide(raw.substr(off));
	}

	out.clear();
	size_t lineStart = 0;
	while (lineStart <= text.size())
	{
		size_t lineEnd = text.find(L'\n', lineStart);
		if (lineEnd == std::wstring::npos) lineEnd = text.size();
		std::wstring line = Trim(text.substr(lineStart, lineEnd - lineStart));
		lineStart = lineEnd + 1;
		if (line.empty() || line[0] == L';' || line[0] == L'#')
		{
			continue;
		}
		const size_t eq = line.find(L'=');
		if (eq == std::wstring::npos) continue;
		std::wstring key = Trim(line.substr(0, eq));
		std::wstring val = Unescape(Trim(line.substr(eq + 1)));
		if (key == L"@Name" || key == L"Name")
		{
			if (name) *name = val;
			continue;
		}
		if (key == L"@Code" || key == L"Code")
		{
			if (code) *code = val;
			continue;
		}
		out[key] = val;
	}
	return true;
}

std::wstring ExeDir()
{
	wchar_t dir[MAX_PATH] = {0};
	GetModuleFileNameW(NULL, dir, MAX_PATH);
	wchar_t* slash = wcsrchr(dir, L'\\');
	if (slash) *slash = 0;
	return dir;
}

std::wstring IniPath()
{
	return GetAppDataDir() + L"\\settings.ini";
}

std::wstring ReadIni(const wchar_t* key, const wchar_t* def)
{
	wchar_t buf[512] = {0};
	GetPrivateProfileStringW(L"General", key, def, buf, _countof(buf), IniPath().c_str());
	return buf;
}

void WriteIni(const wchar_t* key, const wchar_t* val)
{
	CreateDirectoryW(GetAppDataDir().c_str(), NULL);
	WritePrivateProfileStringW(L"General", key, val, IniPath().c_str());
}

std::wstring SystemLangCode()
{
	wchar_t locale[16] = {0};
	if (GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SNAME, locale, _countof(locale)) > 0)
	{
		return locale; // e.g. zh-CN
	}
	const LANGID id = GetUserDefaultUILanguage();
	const WORD primary = PRIMARYLANGID(id);
	if (primary == LANG_CHINESE)
	{
		return (SUBLANGID(id) == SUBLANG_CHINESE_TRADITIONAL) ? L"zh-TW" : L"zh-CN";
	}
	if (primary == LANG_JAPANESE) return L"ja-JP";
	return L"en-US";
}

bool LoadPackByCode(const std::wstring& wantCode)
{
	const auto packs = LangEnumerate();
	const LangPackInfo* hit = nullptr;
	for (size_t i = 0; i < packs.size(); ++i)
	{
		if (_wcsicmp(packs[i].code.c_str(), wantCode.c_str()) == 0)
		{
			hit = &packs[i];
			break;
		}
	}
	// loose match: zh-CN ~ zh
	if (!hit && wantCode.size() >= 2)
	{
		const std::wstring prim = wantCode.substr(0, 2);
		for (size_t i = 0; i < packs.size(); ++i)
		{
			if (_wcsnicmp(packs[i].code.c_str(), prim.c_str(), 2) == 0)
			{
				hit = &packs[i];
				break;
			}
		}
	}
	g_strings.clear();
	g_code = L"en-US";
	g_name = L"English (US)";
	if (!hit)
	{
		return false;
	}
	std::wstring name, code;
	if (!ParseLngFile(hit->path, g_strings, &name, &code))
	{
		return false;
	}
	g_code = code.empty() ? hit->code : code;
	g_name = name.empty() ? hit->name : name;
	return true;
}
} // namespace

std::wstring GetAppDataDir()
{
	PWSTR local = nullptr;
	if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, NULL, &local)) && local)
	{
		std::wstring dir = std::wstring(local) + L"\\YouYouJieTu";
		CoTaskMemFree(local);
		CreateDirectoryW(dir.c_str(), NULL);
		return dir;
	}
	return ExeDir();
}

bool LangInit()
{
	g_langDir = ExeDir() + L"\\Lang";
	g_pref = ReadIni(L"Language", L"");
	if (g_pref == L"system" || g_pref == L"System")
	{
		g_pref.clear();
	}
	const std::wstring want = g_pref.empty() ? SystemLangCode() : g_pref;
	LoadPackByCode(want);
	return true;
}

const wchar_t* Tr(const wchar_t* key)
{
	if (!key) return L"";
	const auto it = g_strings.find(key);
	if (it != g_strings.end() && !it->second.empty())
	{
		return it->second.c_str();
	}
	const wchar_t* en = BuiltinEn(key);
	return en ? en : key;
}

std::wstring LangGetPreference()
{
	return g_pref;
}

bool LangSetPreference(const std::wstring& codeOrEmpty)
{
	g_pref = codeOrEmpty;
	WriteIni(L"Language", g_pref.empty() ? L"" : g_pref.c_str());
	const std::wstring want = g_pref.empty() ? SystemLangCode() : g_pref;
	LoadPackByCode(want);
	return true;
}

std::wstring LangCurrentCode()
{
	return g_code;
}

std::wstring LangCurrentName()
{
	return g_name;
}

std::vector<LangPackInfo> LangEnumerate()
{
	std::vector<LangPackInfo> list;
	const std::wstring dir = g_langDir.empty() ? (ExeDir() + L"\\Lang") : g_langDir;
	const std::wstring pattern = dir + L"\\*.lng";
	WIN32_FIND_DATAW fd = {};
	HANDLE h = FindFirstFileW(pattern.c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE)
	{
		return list;
	}
	do
	{
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
		LangPackInfo info;
		info.path = dir + L"\\" + fd.cFileName;
		std::map<std::wstring, std::wstring> tmp;
		std::wstring name, code;
		ParseLngFile(info.path, tmp, &name, &code);
		info.code = code;
		if (info.code.empty())
		{
			// filename without .lng
			std::wstring fn = fd.cFileName;
			const size_t dot = fn.rfind(L'.');
			if (dot != std::wstring::npos) fn = fn.substr(0, dot);
			info.code = fn;
		}
		info.name = name.empty() ? info.code : name;
		list.push_back(info);
	} while (FindNextFileW(h, &fd));
	FindClose(h);
	std::sort(list.begin(), list.end(), [](const LangPackInfo& a, const LangPackInfo& b) {
		return _wcsicmp(a.name.c_str(), b.name.c_str()) < 0;
	});
	return list;
}

bool LoadAppSettings(bool& hideOnCapture)
{
	const std::wstring v = ReadIni(L"HideOnCapture", L"1");
	hideOnCapture = (v != L"0");
	return true;
}

bool SaveAppSettings(bool hideOnCapture)
{
	WriteIni(L"HideOnCapture", hideOnCapture ? L"1" : L"0");
	WriteIni(L"Language", g_pref.empty() ? L"" : g_pref.c_str());
	return true;
}

} // namespace spy
