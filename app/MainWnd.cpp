#include "stdafx.h"
#include "MainWnd.h"
#include "../core/ScreenshotService.h"
#include "../core/CaptureOverlay.h"
#include "../platform/Lang.h"
#include "../platform/SettingsDlg.h"
#include "../resources/resource.h"
#include <stdio.h>
#include <map>

#define WM_SPY_TRAY (WM_APP + 40)
#define WM_SPY_SHOT_DELETE (WM_APP + 51)

extern CMainWnd g_MainWnd;

namespace
{
int CALLBACK OnShot(HELE, HELE, BOOL*)
{
	g_MainWnd.StartRegionCapture();
	return 0;
}
int CALLBACK OnShotFull(HELE, HELE, BOOL*)
{
	g_MainWnd.StartFullCapture();
	return 0;
}
int CALLBACK OnOpenShots(HELE, HELE, BOOL*)
{
	g_MainWnd.OnMenuSelect(kMenuOpenShots);
	return 0;
}
int CALLBACK OnShotThumb(HELE hEle, HELE, BOOL*)
{
	g_MainWnd.OnShotThumbClick(hEle);
	return 0;
}
int CALLBACK SpyOnShotThumbDbClick(HELE hEle, HELE, POINT*, BOOL*)
{
	g_MainWnd.OnShotThumbDblClick(hEle);
	return 0;
}
int CALLBACK SpyOnShotThumbRButtonUp(HELE hEle, HELE, UINT, POINT* pt, BOOL*)
{
	g_MainWnd.OnShotThumbRButtonUp(hEle, pt);
	return 0;
}
int CALLBACK SpyOnShotMenuSelect(HELE, HELE, int id, BOOL*)
{
	g_MainWnd.OnShotContextMenuSelect(id);
	return 0;
}
int CALLBACK SpyWndProc(HWINDOW, UINT message, WPARAM wParam, LPARAM lParam, BOOL* pBool)
{
	g_MainWnd.HandleWndMessage(message, wParam, lParam, pBool);
	return 0;
}
int CALLBACK SpyOnMenuBarSelect(HELE, HELE, int id, BOOL*)
{
	g_MainWnd.OnMenuSelect(id);
	return 0;
}
}

CMainWnd::CMainWnd()
	: m_hWindow(NULL)
	, m_hMenuBar(NULL)
	, m_hStatus(NULL)
	, m_hBtnShot(NULL)
	, m_hBtnShotFull(NULL)
	, m_hBtnOpenShots(NULL)
	, m_hShotTip(NULL)
	, m_hShotScroll(NULL)
	, m_shotCtxIndex(-1)
	, m_shotSelectedIndex(-1)
	, m_toolY(0)
	, m_toolH(26)
	, m_clientW(0)
	, m_clientH(0)
	, m_exiting(false)
	, m_hiddenToTray(false)
	, m_hideOnCapture(true)
	, m_capturing(false)
{
	spy::LoadAppSettings(m_hideOnCapture);
	for (int i = 0; i < kShotSectionMax; ++i)
	{
		m_hShotSectionLabels[i] = NULL;
	}
	for (int i = 0; i < kShotThumbMax; ++i)
	{
		m_hShotThumbFrames[i] = NULL;
		m_hShotThumbs[i] = NULL;
	}
}

CMainWnd::~CMainWnd()
{
	UnregisterAppHotkeys();
	m_tray.Remove();
}

void CMainWnd::Create(int x, int y, int cx, int cy)
{
	m_hWindow = XWnd_CreateWindow(x, y, cx, cy, spy::TrMut(L"AppTitle"), NULL, XC_SY_DEFAULT);
	if (!m_hWindow)
	{
		return;
	}
	XWnd_SetBkColor(m_hWindow, RGB(245, 247, 250));
	{
		HICON hBig = (HICON)LoadImageW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDI_APP_ICON),
			IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
		HICON hSmall = (HICON)LoadImageW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDI_APP_ICON),
			IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
		if (!hBig)
		{
			hBig = LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDI_APP_ICON));
		}
		if (!hSmall)
		{
			hSmall = hBig;
		}
		if (hBig)
		{
			XWnd_SetIcon(m_hWindow, hBig, TRUE);
		}
		if (hSmall)
		{
			XWnd_SetIcon(m_hWindow, hSmall, FALSE);
		}
		// Also set Win32 icons (packaged MSIX may ignore XCGUI-only icon).
		HWND hwndIcon = XWnd_GetHWnd(m_hWindow);
		if (hwndIcon)
		{
			if (hBig) SendMessageW(hwndIcon, WM_SETICON, ICON_BIG, (LPARAM)hBig);
			if (hSmall) SendMessageW(hwndIcon, WM_SETICON, ICON_SMALL, (LPARAM)hSmall);
		}
	}
	XWnd_RegisterMessageProc(m_hWindow, SpyWndProc);
	BuildUi(cx, cy);
	ApplyUiLanguage();

	HWND hwnd = XWnd_GetHWnd(m_hWindow);
	m_tray.Add(hwnd, WM_SPY_TRAY, spy::Tr(L"AppTitle"));
	m_tray.ShowBalloon(spy::Tr(L"TrayBalloonTitle"), spy::Tr(L"TrayBalloonText"));
	RegisterAppHotkeys();
	RefreshShotHistory();

	XWnd_ShowWindow(m_hWindow, SW_SHOW);
}

void CMainWnd::BuildUi(int cx, int cy)
{
	const int pad = 10;
	m_clientW = cx;
	m_clientH = cy;
	m_toolY = 4;
	m_toolH = 26;
	const int statusH = 22;

	// Structure only — localized strings applied in ApplyUiLanguage().
	m_hBtnShot = XBtn_Create(pad, m_toolY, 100, m_toolH, L"", m_hWindow);
	m_hBtnShotFull = XBtn_Create(pad + 110, m_toolY, 100, m_toolH, L"", m_hWindow);
	m_hBtnOpenShots = XBtn_Create(pad + 220, m_toolY, 110, m_toolH, L"", m_hWindow);
	XEle_RegisterEvent(m_hBtnShot, XE_BNCLICK, OnShot);
	XEle_RegisterEvent(m_hBtnShotFull, XE_BNCLICK, OnShotFull);
	XEle_RegisterEvent(m_hBtnOpenShots, XE_BNCLICK, OnOpenShots);

	BuildShotPanel();

	m_hStatus = XStatic_Create(pad, cy - statusH - 4, cx - pad * 2, statusH, L"", m_hWindow);
	XEle_SetBkTransparent(m_hStatus, TRUE);

	RelayoutTools();
}

void CMainWnd::PlaceToolEle(HELE hEle, int x, int y, int cx, int cy, BOOL show)
{
	if (!hEle)
	{
		return;
	}
	if (show)
	{
		RECT rc = {x, y, x + cx, y + cy};
		XEle_SetRect(hEle, &rc, FALSE);
	}
	XEle_ShowEle(hEle, show);
}

void CMainWnd::RelayoutTools()
{
	const int pad = 10;
	const int y = m_toolY;
	const int h = m_toolH;
	PlaceToolEle(m_hBtnShot, pad, y, 100, h, TRUE);
	PlaceToolEle(m_hBtnShotFull, pad + 110, y, 100, h, TRUE);
	PlaceToolEle(m_hBtnOpenShots, pad + 220, y, 110, h, TRUE);
	UpdateShotPanelVisible(TRUE);
	if (m_hStatus && m_clientW > 0 && m_clientH > 0)
	{
		const int statusH = 22;
		PlaceToolEle(m_hStatus, pad, m_clientH - statusH - 4, m_clientW - pad * 2, statusH, TRUE);
	}
}

void CMainWnd::BuildMenuBar(int cx)
{
	if (cx <= 0)
	{
		cx = m_clientW > 0 ? m_clientW : 980;
	}
	// XMenuBar has no SetItemText — destroy + recreate with current language.
	if (m_hMenuBar)
	{
		XEle_Destroy(m_hMenuBar);
		m_hMenuBar = NULL;
	}
	m_hMenuBar = XMenuBar_Create(0, 0, cx, 26, m_hWindow);
	XMenuBar_AddButton(m_hMenuBar, spy::TrMut(L"MenuEdit"));
	XMenuBar_AddButton(m_hMenuBar, spy::TrMut(L"MenuTools"));
	XMenuBar_AddButton(m_hMenuBar, spy::TrMut(L"MenuHelp"));

	XMenuBar_AddMenuItem(m_hMenuBar, 0, kMenuExit, spy::TrMut(L"MenuExit"));
	XMenuBar_AddMenuItem(m_hMenuBar, 1, kMenuOpenShots, spy::TrMut(L"MenuOpenShots"));
	XMenuBar_AddMenuItem(m_hMenuBar, 1, kMenuStartShot, spy::TrMut(L"MenuStartShot"));
	XMenuBar_AddMenuItem(m_hMenuBar, 1, kMenuFullShot, spy::TrMut(L"MenuFullShot"));
	XMenuBar_AddMenuItem(m_hMenuBar, 1, -1, NULL, XMENU_ROOT, XM_SEPARATOR);
	XMenuBar_AddMenuItem(m_hMenuBar, 1, kMenuSettings, spy::TrMut(L"MenuSettings"));
	XMenuBar_AddMenuItem(m_hMenuBar, 2, kMenuHelp, spy::TrMut(L"MenuUsage"));
	XMenuBar_AddMenuItem(m_hMenuBar, 2, kMenuAbout, spy::TrMut(L"MenuAbout"));

	XEle_RegisterEvent(m_hMenuBar, XE_MENUSELECT, SpyOnMenuBarSelect);
}

void CMainWnd::SetStatusText(const wchar_t* langKey)
{
	if (m_hStatus && langKey)
	{
		XStatic_SetText(m_hStatus, spy::TrMut(langKey));
	}
}

void CMainWnd::ApplyUiLanguage()
{
	HWND hwnd = XWnd_GetHWnd(m_hWindow);
	if (hwnd)
	{
		SetWindowTextW(hwnd, spy::Tr(L"AppTitle"));
	}
	BuildMenuBar(m_clientW);
	if (m_hBtnShot) XBtn_SetText(m_hBtnShot, spy::TrMut(L"BtnStartShot"));
	if (m_hBtnShotFull) XBtn_SetText(m_hBtnShotFull, spy::TrMut(L"BtnFullShot"));
	if (m_hBtnOpenShots) XBtn_SetText(m_hBtnOpenShots, spy::TrMut(L"BtnOpenFolder"));
	if (m_hShotTip) XStatic_SetText(m_hShotTip, spy::TrMut(L"ShotTip"));
	RelayoutShotHistory();
	if (m_hWindow)
	{
		XWnd_RedrawWnd(m_hWindow, TRUE);
	}
}

void CMainWnd::OnMenuSelect(int id)
{
	switch (id)
	{
	case kMenuExit:
		ExitApp();
		break;
	case kMenuOpenShots:
		OpenShotsFolder();
		break;
	case kMenuStartShot:
		StartRegionCapture();
		break;
	case kMenuFullShot:
		StartFullCapture();
		break;
	case kMenuSettings:
		ShowSettings();
		break;
	case kMenuHelp:
		ShowUsageHelp();
		break;
	case kMenuAbout:
		ShowAbout();
		break;
	default:
		break;
	}
}

void CMainWnd::ShowUsageHelp()
{
	HWND hwnd = XWnd_GetHWnd(m_hWindow);
	const std::wstring shots = spy::GetShotsDirectory();
	const std::wstring body = std::wstring(spy::Tr(L"UsageBody")) + shots;
	MessageBoxW(hwnd, body.c_str(), spy::Tr(L"UsageTitle"), MB_OK | MB_ICONINFORMATION);
}

void CMainWnd::ShowAbout()
{
	HWND hwnd = XWnd_GetHWnd(m_hWindow);
	const std::wstring body = std::wstring(spy::Tr(L"AboutBody")) + L"Lang: " + spy::LangCurrentName();
	MessageBoxW(hwnd, body.c_str(), spy::Tr(L"AboutTitle"), MB_OK | MB_ICONINFORMATION);
}

void CMainWnd::OpenShotsFolder()
{
	const std::wstring shots = spy::GetShotsDirectory();
	const std::wstring params = L"\"" + shots + L"\"";
	const INT_PTR ret = (INT_PTR)ShellExecuteW(
		XWnd_GetHWnd(m_hWindow), L"open", L"explorer.exe", params.c_str(), NULL, SW_SHOWNORMAL);
	if (ret <= 32)
	{
		SetStatusText(L"OpenFolderFail");
		return;
	}
	XStatic_SetText(m_hStatus, (wchar_t*)(std::wstring(spy::Tr(L"OpenFolderOk")) + shots).c_str());
}

void CMainWnd::RefreshShotHistory()
{
	SyncShotFileList();
	UpdateShotPanelVisible(TRUE);
	RelayoutShotHistory();
	SetStatusText(L"StatusHistoryHint");
	XWnd_RedrawWnd(m_hWindow, TRUE);
}

bool CMainWnd::HandleWndMessage(UINT message, WPARAM wParam, LPARAM lParam, BOOL* pBool)
{
	switch (message)
	{
	case WM_SIZE:
		if (m_hWindow)
		{
			RECT rc = {};
			if (XWnd_GetClientRect(m_hWindow, &rc))
			{
				m_clientW = rc.right - rc.left;
				m_clientH = rc.bottom - rc.top;
				RelayoutTools();
				RelayoutShotHistory();
			}
		}
		break;
	case WM_CLOSE:
		if (!m_exiting)
		{
			HideToTray();
			if (pBool) *pBool = TRUE;
			return true;
		}
		break;
	case WM_SPY_TRAY:
		OnTrayNotify(lParam);
		if (pBool) *pBool = TRUE;
		return true;
	case WM_SPY_SHOT_DELETE:
		RemoveShotAt((int)wParam);
		if (pBool) *pBool = TRUE;
		return true;
	case WM_HOTKEY:
		OnHotkey((int)wParam);
		if (pBool) *pBool = TRUE;
		return true;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case kTrayShow:
			ShowFromTray();
			if (pBool) *pBool = TRUE;
			return true;
		case kTraySettings:
			ShowFromTray();
			ShowSettings();
			if (pBool) *pBool = TRUE;
			return true;
		case kTrayStartShot:
			StartRegionCapture();
			if (pBool) *pBool = TRUE;
			return true;
		case kTrayFullShot:
			StartFullCapture();
			if (pBool) *pBool = TRUE;
			return true;
		case kTrayExit:
			ExitApp();
			if (pBool) *pBool = TRUE;
			return true;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return false;
}

void CMainWnd::RegisterAppHotkeys()
{
	HWND hwnd = XWnd_GetHWnd(m_hWindow);
	if (!hwnd)
	{
		return;
	}
	RegisterHotKey(hwnd, kHotkeyRegion, MOD_CONTROL | MOD_ALT, 'I');
	RegisterHotKey(hwnd, kHotkeyFull, MOD_CONTROL | MOD_ALT | MOD_SHIFT, 'I');
}

void CMainWnd::UnregisterAppHotkeys()
{
	HWND hwnd = XWnd_GetHWnd(m_hWindow);
	if (!hwnd)
	{
		return;
	}
	UnregisterHotKey(hwnd, kHotkeyRegion);
	UnregisterHotKey(hwnd, kHotkeyFull);
}

void CMainWnd::OnHotkey(int id)
{
	if (id == kHotkeyRegion)
	{
		StartRegionCapture();
	}
	else if (id == kHotkeyFull)
	{
		StartFullCapture();
	}
}

void CMainWnd::HideToTray()
{
	m_hiddenToTray = true;
	XWnd_ShowWindow(m_hWindow, SW_HIDE);
}

void CMainWnd::ShowFromTray()
{
	m_hiddenToTray = false;
	XWnd_ShowWindow(m_hWindow, SW_SHOW);
	HWND hwnd = XWnd_GetHWnd(m_hWindow);
	if (hwnd)
	{
		ShowWindow(hwnd, SW_RESTORE);
		SetForegroundWindow(hwnd);
	}
}

void CMainWnd::ExitApp()
{
	m_exiting = true;
	UnregisterAppHotkeys();
	m_tray.Remove();
	XWnd_CloseWindow(m_hWindow);
}

void CMainWnd::ShowSettings()
{
	HWND hwnd = XWnd_GetHWnd(m_hWindow);
	std::wstring langPref = spy::LangGetPreference();
	const std::wstring oldLang = langPref;
	if (!spy::ShowSettingsDialog(hwnd, m_hideOnCapture, langPref))
	{
		return;
	}

	spy::SaveAppSettings(m_hideOnCapture);
	if (_wcsicmp(oldLang.c_str(), langPref.c_str()) != 0)
	{
		spy::LangSetPreference(langPref);
		ApplyUiLanguage();
		SetStatusText(L"SettingsLangApplied");
		m_tray.ShowBalloon(spy::Tr(L"TrayBalloonTitle"), spy::Tr(L"TrayBalloonText"));
	}
	else
	{
		SetStatusText(m_hideOnCapture ? L"SettingsSavedHideOn" : L"SettingsSavedHideOff");
	}
}

void CMainWnd::OnTrayNotify(LPARAM lParam)
{
	if (lParam == WM_LBUTTONUP || lParam == WM_LBUTTONDBLCLK)
	{
		ShowFromTray();
	}
	else if (lParam == WM_RBUTTONUP || lParam == WM_RBUTTONDOWN)
	{
		PopupTrayMenu();
	}
}

void CMainWnd::PopupTrayMenu()
{
	HWND hwnd = XWnd_GetHWnd(m_hWindow);
	if (!hwnd)
	{
		return;
	}
	POINT pt = {};
	GetCursorPos(&pt);
	HMENU menu = CreatePopupMenu();
	AppendMenuW(menu, MF_STRING, kTrayShow, spy::Tr(L"TrayShow"));
	AppendMenuW(menu, MF_STRING, kTrayStartShot, spy::Tr(L"TrayStartShot"));
	AppendMenuW(menu, MF_STRING, kTrayFullShot, spy::Tr(L"TrayFullShot"));
	AppendMenuW(menu, MF_STRING, kTraySettings, spy::Tr(L"TraySettings"));
	AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
	AppendMenuW(menu, MF_STRING, kTrayExit, spy::Tr(L"TrayExit"));
	SetForegroundWindow(hwnd);
	TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL);
	PostMessageW(hwnd, WM_NULL, 0, 0);
	DestroyMenu(menu);
}

void CMainWnd::StartRegionCapture()
{
	if (m_capturing)
	{
		return;
	}
	m_capturing = true;
	const bool wasHidden = m_hiddenToTray;
	if (m_hideOnCapture)
	{
		HideToTray();
		Sleep(180);
	}

	std::wstring path;
	const bool ok = spy::RunRegionCaptureOverlay(path);

	if (m_hideOnCapture && !wasHidden)
	{
		ShowFromTray();
	}
	m_capturing = false;

	if (ok)
	{
		const bool copied = spy::CopyBmpFileToClipboard(path);
		std::wstring msg = copied
			? (std::wstring(spy::Tr(L"SavedClipboard")) + path)
			: (std::wstring(spy::Tr(L"SavedNoClipboard")) + path);
		XStatic_SetText(m_hStatus, (wchar_t*)msg.c_str());
		PushShotHistory(path);
	}
	else
	{
		SetStatusText(L"CaptureCancelled");
	}
}

void CMainWnd::StartFullCapture()
{
	if (m_capturing)
	{
		return;
	}
	m_capturing = true;
	const bool wasHidden = m_hiddenToTray;
	if (m_hideOnCapture)
	{
		HideToTray();
		Sleep(180);
	}
	std::wstring path = spy::MakeDefaultShotPath();
	const bool ok = spy::CaptureFullScreenAnnotated(path);
	if (m_hideOnCapture && !wasHidden)
	{
		ShowFromTray();
	}
	m_capturing = false;
	if (ok)
	{
		const bool copied = spy::CopyBmpFileToClipboard(path);
		std::wstring msg = copied
			? (std::wstring(spy::Tr(L"FullSavedClipboard")) + path)
			: (std::wstring(spy::Tr(L"FullSavedNoClipboard")) + path);
		XStatic_SetText(m_hStatus, (wchar_t*)msg.c_str());
		PushShotHistory(path);
	}
	else
	{
		SetStatusText(L"FullFailed");
	}
}

void CMainWnd::BuildShotPanel()
{
	const int pad = 10;
	m_hShotTip = XStatic_Create(pad, m_toolY + 30, 700, 40, L"", m_hWindow);
	XEle_SetBkTransparent(m_hShotTip, TRUE);
	XEle_SetTextColor(m_hShotTip, RGB(80, 80, 80));
	XStatic_SetTextAlign(m_hShotTip, DT_LEFT | DT_TOP | DT_WORDBREAK);

	m_hShotScroll = XSView_Create(pad, m_toolY + 78, 700, 400, m_hWindow);
	XSView_EnableHScroll(m_hShotScroll, FALSE);
	XSView_EnableVScroll(m_hShotScroll, TRUE);
	XSView_SetSpacing(m_hShotScroll, 4, 4, 4, 4);
	XEle_RegisterEvent(m_hShotScroll, XE_MENUSELECT, SpyOnShotMenuSelect);

	for (int i = 0; i < kShotSectionMax; ++i)
	{
		m_hShotSectionLabels[i] = XStatic_Create(8, 0, 200, 22, L"", m_hShotScroll);
		XEle_SetBkTransparent(m_hShotSectionLabels[i], TRUE);
		XEle_SetTextColor(m_hShotSectionLabels[i], RGB(60, 60, 60));
		XEle_ShowEle(m_hShotSectionLabels[i], FALSE);
	}
	for (int i = 0; i < kShotThumbMax; ++i)
	{
		m_hShotThumbFrames[i] = XEle_Create(8, 0, 168, 105, m_hShotScroll);
		XEle_EnableBorder(m_hShotThumbFrames[i], TRUE);
		XEle_SetBorderColor(m_hShotThumbFrames[i], RGB(24, 144, 255));
		XEle_SetBkColor(m_hShotThumbFrames[i], RGB(24, 144, 255));
		m_hShotThumbs[i] = XBtn_Create(2, 2, 164, 101, L"", m_hShotThumbFrames[i]);
		XEle_EnableDrawFocus(m_hShotThumbs[i], FALSE);
		XEle_EnableBorder(m_hShotThumbs[i], FALSE);
		XEle_SetUserData(m_hShotThumbs[i], -1);
		XEle_RegisterEvent(m_hShotThumbs[i], XE_BNCLICK, OnShotThumb);
		XEle_RegisterEvent(m_hShotThumbs[i], XM_MOUSEDBCLICK, SpyOnShotThumbDbClick);
		XEle_RegisterEvent(m_hShotThumbs[i], XM_RBUTTONUP, SpyOnShotThumbRButtonUp);
		XEle_ShowEle(m_hShotThumbFrames[i], FALSE);
	}

	spy::EnumShotFiles(m_shotFiles, kShotThumbMax);
	if (!m_shotFiles.empty())
	{
		m_lastShotPath = m_shotFiles[0].path;
	}
}

void CMainWnd::UpdateShotPanelVisible(BOOL show)
{
	const int pad = 10;
	const int yTip = m_toolY + m_toolH + 8;
	const int tipH = 40;
	const int tipW = (m_clientW > pad * 2) ? (m_clientW - pad * 2) : 700;
	PlaceToolEle(m_hShotTip, pad, yTip, tipW, tipH, show);

	const int statusH = 24;
	const int scrollY = yTip + tipH + 6;
	const int scrollH = (m_clientH > scrollY + statusH + 8) ? (m_clientH - scrollY - statusH - 8) : 200;
	PlaceToolEle(m_hShotScroll, pad, scrollY, tipW, scrollH, show);
}

bool CMainWnd::SyncShotFileList()
{
	std::vector<spy::ShotFileInfo> files;
	spy::EnumShotFiles(files, kShotThumbMax);
	bool changed = (files.size() != m_shotFiles.size());
	if (!changed)
	{
		for (size_t i = 0; i < files.size(); ++i)
		{
			if (_wcsicmp(files[i].path.c_str(), m_shotFiles[i].path.c_str()) != 0
				|| files[i].isToday != m_shotFiles[i].isToday
				|| files[i].dateKey != m_shotFiles[i].dateKey)
			{
				changed = true;
				break;
			}
		}
	}
	if (!changed)
	{
		return false;
	}
	m_shotFiles.swap(files);
	m_shotSelectedIndex = -1;
	if (!m_lastShotPath.empty())
	{
		for (int i = 0; i < (int)m_shotFiles.size(); ++i)
		{
			if (_wcsicmp(m_shotFiles[i].path.c_str(), m_lastShotPath.c_str()) == 0)
			{
				m_shotSelectedIndex = i;
				break;
			}
		}
	}
	if (m_lastShotPath.empty() && !m_shotFiles.empty())
	{
		m_lastShotPath = m_shotFiles[0].path;
	}
	return true;
}

int CMainWnd::ShotThumbIndex(HELE hEle) const
{
	if (!hEle)
	{
		return -1;
	}
	const int idx = (int)XEle_GetUserData(hEle);
	if (idx < 0 || idx >= (int)m_shotFiles.size())
	{
		return -1;
	}
	return idx;
}

void CMainWnd::UpdateShotThumbFrameColors()
{
	const COLORREF blue = RGB(24, 144, 255);
	const COLORREF red = RGB(220, 38, 38);
	for (int i = 0; i < kShotThumbMax; ++i)
	{
		if (!m_hShotThumbFrames[i])
		{
			continue;
		}
		const int fileIndex = (int)XEle_GetUserData(m_hShotThumbs[i]);
		const bool selected = (fileIndex >= 0 && fileIndex == m_shotSelectedIndex);
		const COLORREF c = selected ? red : blue;
		XEle_SetBorderColor(m_hShotThumbFrames[i], c);
		XEle_SetBkColor(m_hShotThumbFrames[i], c);
		XEle_RedrawEle(m_hShotThumbFrames[i], FALSE);
	}
}

void CMainWnd::SetShotThumbSelected(int fileIndex)
{
	if (fileIndex < 0 || fileIndex >= (int)m_shotFiles.size())
	{
		m_shotSelectedIndex = -1;
	}
	else
	{
		m_shotSelectedIndex = fileIndex;
		m_lastShotPath = m_shotFiles[fileIndex].path;
	}
	UpdateShotThumbFrameColors();
}

void CMainWnd::ClearButtonImage(HELE hBtn)
{
	if (!hBtn)
	{
		return;
	}
	XBtn_SetImageLeave(hBtn, NULL);
	XBtn_SetImageStay(hBtn, NULL);
	XBtn_SetImageDown(hBtn, NULL);
}

void CMainWnd::ApplyImageToButton(HELE hBtn, const std::wstring& path)
{
	if (!hBtn)
	{
		return;
	}
	ClearButtonImage(hBtn);
	XBtn_SetText(hBtn, L"");
	if (path.empty())
	{
		return;
	}
	HIMAGE img = XImage_LoadFile((wchar_t*)path.c_str(), TRUE);
	if (!img)
	{
		return;
	}
	XImage_SetDrawType(img, XC_IMAGE_STRETCH);
	XImage_EnableAutoDestroy(img, TRUE);
	XBtn_SetImageLeave(hBtn, img);
	XBtn_SetImageStay(hBtn, img);
	XBtn_SetImageDown(hBtn, img);
}

void CMainWnd::RelayoutShotHistory()
{
	if (!m_hShotScroll)
	{
		return;
	}
	RECT rcScroll = {};
	XEle_GetClientRect(m_hShotScroll, &rcScroll);
	const int viewW = (rcScroll.right - rcScroll.left > 40) ? (rcScroll.right - rcScroll.left - 24) : 600;
	const int thumbW = 168;
	const int thumbH = 105;
	const int gap = 10;
	const int sectionH = 26;
	const int cols = (viewW + gap) / (thumbW + gap);
	const int colCount = cols > 1 ? cols : 1;

	for (int i = 0; i < kShotSectionMax; ++i)
	{
		XEle_ShowEle(m_hShotSectionLabels[i], FALSE);
	}

	struct Section
	{
		std::wstring title;
		std::vector<int> indices;
	};
	std::vector<Section> sections;
	Section recent;
	recent.title = spy::Tr(L"Recent");
	std::map<std::wstring, int> dateSectionPos;

	for (int i = 0; i < (int)m_shotFiles.size() && i < kShotThumbMax; ++i)
	{
		if (m_shotFiles[i].isToday)
		{
			recent.indices.push_back(i);
			continue;
		}
		std::map<std::wstring, int>::iterator it = dateSectionPos.find(m_shotFiles[i].dateKey);
		if (it == dateSectionPos.end())
		{
			Section s;
			s.title = m_shotFiles[i].dateKey;
			s.indices.push_back(i);
			dateSectionPos[m_shotFiles[i].dateKey] = (int)sections.size();
			sections.push_back(s);
		}
		else
		{
			sections[it->second].indices.push_back(i);
		}
	}

	std::vector<Section> ordered;
	if (!recent.indices.empty())
	{
		ordered.push_back(recent);
		for (size_t s = 0; s < sections.size(); ++s)
		{
			ordered.push_back(sections[s]);
		}
	}
	else if (!m_shotFiles.empty())
	{
		Section only;
		only.title = spy::Tr(L"Recent");
		only.indices.push_back(0);
		ordered.push_back(only);
		for (size_t s = 0; s < sections.size(); ++s)
		{
			Section trimmed = sections[s];
			std::vector<int> keep;
			for (size_t k = 0; k < trimmed.indices.size(); ++k)
			{
				if (trimmed.indices[k] != 0)
				{
					keep.push_back(trimmed.indices[k]);
				}
			}
			if (!keep.empty())
			{
				trimmed.indices.swap(keep);
				ordered.push_back(trimmed);
			}
		}
	}

	bool slotUsed[kShotThumbMax];
	for (int i = 0; i < kShotThumbMax; ++i)
	{
		slotUsed[i] = false;
	}

	int y = 4;
	int labelUsed = 0;
	int thumbUsed = 0;
	for (size_t s = 0; s < ordered.size() && labelUsed < kShotSectionMax; ++s)
	{
		HELE hLabel = m_hShotSectionLabels[labelUsed++];
		XStatic_SetText(hLabel, (wchar_t*)ordered[s].title.c_str());
		RECT lr = {8, y, 8 + viewW, y + sectionH};
		XEle_SetRect(hLabel, &lr, FALSE);
		XEle_ShowEle(hLabel, TRUE);
		y += sectionH;

		int col = 0;
		for (size_t k = 0; k < ordered[s].indices.size() && thumbUsed < kShotThumbMax; ++k)
		{
			const int fileIndex = ordered[s].indices[k];
			const int slot = thumbUsed++;
			HELE hFrame = m_hShotThumbFrames[slot];
			HELE hThumb = m_hShotThumbs[slot];
			const int x = 8 + col * (thumbW + gap);
			RECT tr = {x, y, x + thumbW, y + thumbH};
			XEle_SetRect(hFrame, &tr, FALSE);
			XEle_SetUserData(hThumb, fileIndex);
			const std::wstring& path = m_shotFiles[fileIndex].path;
			if (_wcsicmp(m_shotThumbBoundPath[slot].c_str(), path.c_str()) != 0)
			{
				ApplyImageToButton(hThumb, path);
				m_shotThumbBoundPath[slot] = path;
			}
			XEle_ShowEle(hFrame, TRUE);
			XEle_ShowEle(hThumb, TRUE);
			slotUsed[slot] = true;
			++col;
			if (col >= colCount)
			{
				col = 0;
				y += thumbH + gap;
			}
		}
		if (col != 0)
		{
			y += thumbH + gap;
		}
		y += 8;
	}

	for (int i = 0; i < kShotThumbMax; ++i)
	{
		if (slotUsed[i])
		{
			continue;
		}
		XEle_ShowEle(m_hShotThumbFrames[i], FALSE);
		XEle_ShowEle(m_hShotThumbs[i], FALSE);
		XEle_SetUserData(m_hShotThumbs[i], -1);
		if (!m_shotThumbBoundPath[i].empty())
		{
			ClearButtonImage(m_hShotThumbs[i]);
			m_shotThumbBoundPath[i].clear();
		}
	}

	XSView_SetSize(m_hShotScroll, viewW + 8, y + 8);
	XSView_Adjust(m_hShotScroll);
	UpdateShotThumbFrameColors();
	if (m_hShotScroll)
	{
		XEle_RedrawEle(m_hShotScroll, FALSE);
	}
}

void CMainWnd::PushShotHistory(const std::wstring& path)
{
	if (path.empty())
	{
		return;
	}
	m_lastShotPath = path;
	for (size_t i = 0; i < m_shotFiles.size(); ++i)
	{
		if (_wcsicmp(m_shotFiles[i].path.c_str(), path.c_str()) == 0)
		{
			m_shotFiles.erase(m_shotFiles.begin() + (std::ptrdiff_t)i);
			break;
		}
	}
	spy::ShotFileInfo info;
	info.path = path;
	SYSTEMTIME st = {};
	GetLocalTime(&st);
	wchar_t key[32] = {0};
	_snwprintf_s(key, _countof(key), L"%04u-%02u-%02u",
		(unsigned)st.wYear, (unsigned)st.wMonth, (unsigned)st.wDay);
	info.dateKey = key;
	info.isToday = true;
	m_shotFiles.insert(m_shotFiles.begin(), info);
	if ((int)m_shotFiles.size() > kShotThumbMax)
	{
		m_shotFiles.resize((size_t)kShotThumbMax);
	}
	m_shotSelectedIndex = 0;
	RelayoutShotHistory();
}

void CMainWnd::RemoveShotAt(int index)
{
	if (index < 0 || index >= (int)m_shotFiles.size())
	{
		return;
	}
	const std::wstring path = m_shotFiles[index].path;
	const int ret = MessageBoxW(XWnd_GetHWnd(m_hWindow), spy::Tr(L"DeleteConfirm"), spy::Tr(L"DeleteTitle"),
		MB_YESNO | MB_ICONWARNING);
	if (ret != IDYES)
	{
		return;
	}

	for (int i = 0; i < kShotThumbMax; ++i)
	{
		if (_wcsicmp(m_shotThumbBoundPath[i].c_str(), path.c_str()) == 0)
		{
			ClearButtonImage(m_hShotThumbs[i]);
			m_shotThumbBoundPath[i].clear();
			XEle_SetUserData(m_hShotThumbs[i], -1);
		}
	}
	if (m_hShotScroll)
	{
		XEle_RedrawEle(m_hShotScroll, TRUE);
	}

	SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_NORMAL);
	if (!DeleteFileW(path.c_str()))
	{
		const DWORD err = GetLastError();
		wchar_t msg[128];
		_snwprintf_s(msg, _countof(msg), spy::Tr(L"DeleteFail"), (unsigned long)err);
		XStatic_SetText(m_hStatus, msg);
		RelayoutShotHistory();
		return;
	}

	m_shotFiles.erase(m_shotFiles.begin() + (std::ptrdiff_t)index);
	if (m_shotSelectedIndex == index)
	{
		m_shotSelectedIndex = -1;
	}
	else if (m_shotSelectedIndex > index)
	{
		--m_shotSelectedIndex;
	}
	if (_wcsicmp(m_lastShotPath.c_str(), path.c_str()) == 0)
	{
		m_lastShotPath = m_shotFiles.empty() ? L"" : m_shotFiles[0].path;
		if (m_shotSelectedIndex < 0 && !m_shotFiles.empty())
		{
			m_shotSelectedIndex = 0;
		}
	}
	m_shotCtxIndex = -1;
	RelayoutShotHistory();
	SetStatusText(L"Deleted");
}

void CMainWnd::DeleteShotPath(const std::wstring& path)
{
	int index = -1;
	for (int i = 0; i < (int)m_shotFiles.size(); ++i)
	{
		if (_wcsicmp(m_shotFiles[i].path.c_str(), path.c_str()) == 0)
		{
			index = i;
			break;
		}
	}
	if (index >= 0)
	{
		RemoveShotAt(index);
	}
}

void CMainWnd::OnSaveLastShot()
{
	if (m_lastShotPath.empty())
	{
		return;
	}
	spy::SaveShotAsDialog(XWnd_GetHWnd(m_hWindow), m_lastShotPath);
}

void CMainWnd::OnCopyLastShot()
{
	if (m_lastShotPath.empty())
	{
		return;
	}
	spy::CopyBmpFileToClipboard(m_lastShotPath);
}

void CMainWnd::EditShotPath(const std::wstring& path)
{
	if (path.empty() || m_capturing)
	{
		return;
	}
	m_capturing = true;
	std::wstring outPath;
	const bool ok = spy::RunAnnotateOnImage(path, outPath);
	m_capturing = false;
	if (ok)
	{
		spy::CopyBmpFileToClipboard(outPath);
		PushShotHistory(outPath);
		SetStatusText(L"EditSaved");
	}
	else
	{
		SetStatusText(L"EditCancelled");
	}
}

void CMainWnd::OnAnnotateLastShot()
{
	EditShotPath(m_lastShotPath);
}

void CMainWnd::PreviewShotPath(const std::wstring& path)
{
	if (path.empty())
	{
		return;
	}
	ShellExecuteW(XWnd_GetHWnd(m_hWindow), L"open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
	SetStatusText(L"PreviewOpened");
}

void CMainWnd::PinShotPath(const std::wstring& path)
{
	if (path.empty())
	{
		return;
	}
	spy::ShowPinnedShot(path);
	SetStatusText(L"Pinned");
}

void CMainWnd::OnShotThumbClick(HELE hEle)
{
	const int index = ShotThumbIndex(hEle);
	if (index < 0)
	{
		return;
	}
	SetShotThumbSelected(index);
	SetStatusText(L"Selected");
}

void CMainWnd::OnShotThumbDblClick(HELE hEle)
{
	const int index = ShotThumbIndex(hEle);
	if (index < 0)
	{
		return;
	}
	SetShotThumbSelected(index);
	EditShotPath(m_lastShotPath);
}

void CMainWnd::OnShotThumbRButtonUp(HELE hEle, POINT* pt)
{
	const int index = ShotThumbIndex(hEle);
	if (index < 0)
	{
		return;
	}
	(void)pt;
	SetShotThumbSelected(index);
	m_shotCtxIndex = index;

	POINT screenPt = {};
	GetCursorPos(&screenPt);

	HMENUX hMenu = XMenu_Create();
	XMenu_AddItem(hMenu, kCtxShotPreview, spy::TrMut(L"CtxPreview"));
	XMenu_AddItem(hMenu, kCtxShotCopy, spy::TrMut(L"CtxCopy"));
	XMenu_AddItem(hMenu, kCtxShotEdit, spy::TrMut(L"CtxEdit"));
	XMenu_AddItem(hMenu, kCtxShotPin, spy::TrMut(L"CtxPin"));
	XMenu_AddItem(hMenu, -1, NULL, XMENU_ROOT, XM_SEPARATOR);
	XMenu_AddItem(hMenu, kCtxShotDelete, spy::TrMut(L"CtxDelete"));
	XMenu_Popup(hMenu, XEle_GetHWnd(m_hShotScroll), screenPt.x, screenPt.y, m_hShotScroll);
}

void CMainWnd::OnShotContextMenuSelect(int id)
{
	if (m_shotCtxIndex < 0 || m_shotCtxIndex >= (int)m_shotFiles.size())
	{
		return;
	}
	const std::wstring path = m_shotFiles[m_shotCtxIndex].path;
	switch (id)
	{
	case kCtxShotPreview:
		PreviewShotPath(path);
		break;
	case kCtxShotCopy:
		if (spy::CopyBmpFileToClipboard(path))
		{
			SetStatusText(L"Copied");
		}
		else
		{
			SetStatusText(L"CopyFail");
		}
		break;
	case kCtxShotEdit:
		EditShotPath(path);
		break;
	case kCtxShotPin:
		PinShotPath(path);
		break;
	case kCtxShotDelete:
		PostMessageW(XWnd_GetHWnd(m_hWindow), WM_SPY_SHOT_DELETE, (WPARAM)m_shotCtxIndex, 0);
		break;
	default:
		break;
	}
}
