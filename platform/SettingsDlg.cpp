#include "SettingsDlg.h"
#include "Lang.h"
#include <commctrl.h>
#include <vector>

#pragma comment(lib, "comctl32.lib")

namespace spy
{
namespace
{
enum
{
	IDC_LANG_LABEL = 1001,
	IDC_LANG_COMBO = 1002,
	IDC_HIDE_CHECK = 1003,
	IDC_HOTKEYS = 1004,
	IDC_OK = 1005,
	IDC_CANCEL = 1006
};

struct DlgState
{
	bool hideOnCapture;
	std::wstring langPref;
	std::vector<LangPackInfo> packs;
	bool accepted;
	bool closed;
	HFONT font;
};

BOOL CALLBACK ApplyFontProc(HWND child, LPARAM font)
{
	SendMessageW(child, WM_SETFONT, (WPARAM)font, TRUE);
	return TRUE;
}

HFONT CreateUiFont()
{
	// Segoe UI + DEFAULT_CHARSET：靠系统字体链接显示泰/缅/韩/格鲁吉亚等脚本
	NONCLIENTMETRICSW ncm = {};
	ncm.cbSize = sizeof(ncm);
	if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
	{
		ncm.lfMessageFont.lfCharSet = DEFAULT_CHARSET;
		ncm.lfMessageFont.lfQuality = CLEARTYPE_QUALITY;
		// 尽量用 Segoe UI（覆盖面比 MS Shell Dlg / 系统点阵字体广）
		wcsncpy_s(ncm.lfMessageFont.lfFaceName, L"Segoe UI", _TRUNCATE);
		HFONT f = CreateFontIndirectW(&ncm.lfMessageFont);
		if (f) return f;
	}
	return CreateFontW(
		-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
}

std::wstring LangComboLabel(const LangPackInfo& info)
{
	// 本地名 + 语言代码，缺字形时仍可识别
	if (info.name.empty()) return info.code;
	if (info.name == info.code) return info.name;
	return info.name + L"  (" + info.code + L")";
}

void LayoutControls(HWND hwnd)
{
	CreateWindowW(L"STATIC", Tr(L"SettingsLang"),
		WS_CHILD | WS_VISIBLE | SS_LEFT, 16, 18, 90, 22, hwnd, (HMENU)IDC_LANG_LABEL, NULL, NULL);
	CreateWindowExW(0, L"COMBOBOX", L"",
		WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | CBS_HASSTRINGS,
		110, 14, 300, 360, hwnd, (HMENU)IDC_LANG_COMBO, NULL, NULL);
	CreateWindowW(L"BUTTON", Tr(L"SettingsHide"),
		WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
		16, 56, 400, 24, hwnd, (HMENU)IDC_HIDE_CHECK, NULL, NULL);
	CreateWindowW(L"STATIC", Tr(L"SettingsHotkeys"),
		WS_CHILD | WS_VISIBLE | SS_LEFT, 16, 92, 400, 72, hwnd, (HMENU)IDC_HOTKEYS, NULL, NULL);
	CreateWindowW(L"BUTTON", Tr(L"SettingsOk"),
		WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 220, 190, 96, 30, hwnd, (HMENU)IDC_OK, NULL, NULL);
	CreateWindowW(L"BUTTON", Tr(L"SettingsCancel"),
		WS_CHILD | WS_VISIBLE, 326, 190, 96, 30, hwnd, (HMENU)IDC_CANCEL, NULL, NULL);
}

LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	DlgState* st = reinterpret_cast<DlgState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
	switch (msg)
	{
	case WM_CREATE:
	{
		CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
		st = reinterpret_cast<DlgState*>(cs->lpCreateParams);
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)st);
		st->font = CreateUiFont();
		LayoutControls(hwnd);
		if (st->font)
		{
			SendMessageW(hwnd, WM_SETFONT, (WPARAM)st->font, TRUE);
			EnumChildWindows(hwnd, ApplyFontProc, (LPARAM)st->font);
		}
		HWND combo = GetDlgItem(hwnd, IDC_LANG_COMBO);
		const std::wstring sysLabel = std::wstring(Tr(L"SettingsLangSystem")) + L"  (system)";
		const int sysIdx = (int)SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)sysLabel.c_str());
		SendMessageW(combo, CB_SETITEMDATA, sysIdx, (LPARAM)-1);
		int sel = 0;
		for (size_t i = 0; i < st->packs.size(); ++i)
		{
			const std::wstring label = LangComboLabel(st->packs[i]);
			const int idx = (int)SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)label.c_str());
			SendMessageW(combo, CB_SETITEMDATA, idx, (LPARAM)i);
			if (!st->langPref.empty() && _wcsicmp(st->langPref.c_str(), st->packs[i].code.c_str()) == 0)
			{
				sel = idx;
			}
		}
		if (st->langPref.empty()) sel = 0;
		SendMessageW(combo, CB_SETCURSEL, sel, 0);
		CheckDlgButton(hwnd, IDC_HIDE_CHECK, st->hideOnCapture ? BST_CHECKED : BST_UNCHECKED);
		return 0;
	}
	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_OK)
		{
			HWND combo = GetDlgItem(hwnd, IDC_LANG_COMBO);
			const int sel = (int)SendMessageW(combo, CB_GETCURSEL, 0, 0);
			const LPARAM data = SendMessageW(combo, CB_GETITEMDATA, sel, 0);
			if (data == (LPARAM)-1 || sel <= 0)
			{
				st->langPref.clear();
			}
			else if (data >= 0 && (size_t)data < st->packs.size())
			{
				st->langPref = st->packs[(size_t)data].code;
			}
			st->hideOnCapture = (IsDlgButtonChecked(hwnd, IDC_HIDE_CHECK) == BST_CHECKED);
			st->accepted = true;
			DestroyWindow(hwnd);
			return 0;
		}
		if (LOWORD(wParam) == IDC_CANCEL || LOWORD(wParam) == IDCANCEL)
		{
			DestroyWindow(hwnd);
			return 0;
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		return 0;
	case WM_DESTROY:
		if (st)
		{
			if (st->font)
			{
				DeleteObject(st->font);
				st->font = NULL;
			}
			st->closed = true;
		}
		return 0;
	default:
		break;
	}
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}
}

bool ShowSettingsDialog(HWND owner, bool& hideOnCapture, std::wstring& langPref)
{
	static bool registered = false;
	if (!registered)
	{
		WNDCLASSW wc = {};
		wc.lpfnWndProc = SettingsWndProc;
		wc.hInstance = GetModuleHandleW(NULL);
		wc.lpszClassName = L"YoyoSettingsDlg";
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		RegisterClassW(&wc);
		registered = true;
	}

	DlgState st;
	st.hideOnCapture = hideOnCapture;
	st.langPref = langPref;
	st.packs = LangEnumerate();
	st.accepted = false;
	st.closed = false;
	st.font = NULL;

	HWND hwnd = CreateWindowExW(
		WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE,
		L"YoyoSettingsDlg",
		Tr(L"SettingsTitle"),
		WS_CAPTION | WS_SYSMENU | WS_POPUP | WS_VISIBLE,
		0, 0, 450, 280,
		owner, NULL, GetModuleHandleW(NULL), &st);
	if (!hwnd)
	{
		return false;
	}

	RECT rc = {};
	GetWindowRect(hwnd, &rc);
	const int w = rc.right - rc.left;
	const int h = rc.bottom - rc.top;
	const int sx = GetSystemMetrics(SM_CXSCREEN);
	const int sy = GetSystemMetrics(SM_CYSCREEN);
	SetWindowPos(hwnd, NULL, (sx - w) / 2, (sy - h) / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	if (owner) EnableWindow(owner, FALSE);
	MSG msg;
	while (!st.closed && GetMessageW(&msg, NULL, 0, 0) > 0)
	{
		if (!IsWindow(hwnd) || !IsDialogMessageW(hwnd, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}
	if (owner)
	{
		EnableWindow(owner, TRUE);
		SetForegroundWindow(owner);
	}

	if (st.accepted)
	{
		hideOnCapture = st.hideOnCapture;
		langPref = st.langPref;
	}
	return st.accepted;
}

} // namespace spy
