#pragma once

#include "stdafx.h"
#include "../core/ScreenshotService.h"
#include "../platform/TrayIcon.h"
#include <vector>
#include <string>
#include <map>

enum ShotMenuId
{
	kMenuExit = 1002,
	kMenuOpenShots = 1101,
	kMenuStartShot = 1102,
	kMenuFullShot = 1103,
	kMenuSettings = 1104,
	kMenuHelp = 1201,
	kMenuAbout = 1202,

	kCtxShotPreview = 2101,
	kCtxShotCopy = 2102,
	kCtxShotEdit = 2103,
	kCtxShotPin = 2104,
	kCtxShotDelete = 2105,

	kTrayShow = 1024,
	kTraySettings = 1025,
	kTrayExit = 1026,
	kTrayStartShot = 1027,
	kTrayFullShot = 1028
};

enum ShotHotkeyId
{
	kHotkeyRegion = 1,
	kHotkeyFull = 2
};

class CMainWnd
{
public:
	CMainWnd();
	~CMainWnd();
	void Create(int x, int y, int cx, int cy);
	HWINDOW GetWnd() const { return m_hWindow; }

	void StartRegionCapture();
	void StartFullCapture();
	void OnMenuSelect(int id);
	void OnSaveLastShot();
	void OnAnnotateLastShot();
	void OnShotThumbClick(HELE hEle);
	void OnShotThumbDblClick(HELE hEle);
	void OnShotThumbRButtonUp(HELE hEle, POINT* pt);
	void OnShotContextMenuSelect(int id);
	void OnCopyLastShot();
	void EditShotPath(const std::wstring& path);
	void PreviewShotPath(const std::wstring& path);
	void PinShotPath(const std::wstring& path);
	void DeleteShotPath(const std::wstring& path);

	void HideToTray();
	void ShowFromTray();
	void ExitApp();
	void ShowSettings();
	void OnTrayNotify(LPARAM lParam);
	void OnHotkey(int id);
	bool IsExiting() const { return m_exiting; }
	bool HandleWndMessage(UINT message, WPARAM wParam, LPARAM lParam, BOOL* pBool);
	void RefreshShotHistory();

private:
	void BuildUi(int cx, int cy);
	void BuildMenuBar(int cx);
	/** Rebuild menu + refresh all localized main-window texts (call after LangSetPreference). */
	void ApplyUiLanguage();
	void SetStatusText(const wchar_t* langKey);
	void RelayoutTools();
	void PlaceToolEle(HELE hEle, int x, int y, int cx, int cy, BOOL show);
	void ShowUsageHelp();
	void ShowAbout();
	void OpenShotsFolder();
	void PopupTrayMenu();
	void RegisterAppHotkeys();
	void UnregisterAppHotkeys();
	void BuildShotPanel();
	void UpdateShotPanelVisible(BOOL show);
	bool SyncShotFileList();
	void RelayoutShotHistory();
	void RemoveShotAt(int index);
	void SetShotThumbSelected(int fileIndex);
	void UpdateShotThumbFrameColors();
	void PushShotHistory(const std::wstring& path);
	void ApplyImageToButton(HELE hBtn, const std::wstring& path);
	void ClearButtonImage(HELE hBtn);
	int ShotThumbIndex(HELE hEle) const;

	HWINDOW m_hWindow;
	HELE m_hMenuBar;
	HELE m_hStatus;
	HELE m_hBtnShot;
	HELE m_hBtnShotFull;
	HELE m_hBtnOpenShots;
	HELE m_hShotTip;
	HELE m_hShotScroll;
	static const int kShotSectionMax = 16;
	static const int kShotThumbMax = 48;
	HELE m_hShotSectionLabels[kShotSectionMax];
	HELE m_hShotThumbFrames[kShotThumbMax];
	HELE m_hShotThumbs[kShotThumbMax];
	int m_shotCtxIndex;
	int m_shotSelectedIndex;

	int m_toolY;
	int m_toolH;
	int m_clientW;
	int m_clientH;
	bool m_exiting;
	bool m_hiddenToTray;
	bool m_hideOnCapture;
	bool m_capturing;
	spy::TrayIcon m_tray;
	std::wstring m_lastShotPath;
	std::vector<spy::ShotFileInfo> m_shotFiles;
	std::wstring m_shotThumbBoundPath[kShotThumbMax];
};
