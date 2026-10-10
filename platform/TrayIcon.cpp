#include "TrayIcon.h"
#include "../resources/resource.h"
#include <shellapi.h>
#include <string>
#include <stdio.h>

namespace spy
{
static HICON LoadAppIcon()
{
	HMODULE mod = GetModuleHandleW(NULL);
	HICON h = (HICON)LoadImageW(mod, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
	if (h)
	{
		return h;
	}
	h = LoadIconW(mod, MAKEINTRESOURCEW(IDI_APP_ICON));
	if (h)
	{
		return h;
	}

	wchar_t icoPath[MAX_PATH] = {0};
	GetModuleFileNameW(NULL, icoPath, MAX_PATH);
	wchar_t* slash = wcsrchr(icoPath, L'\\');
	if (slash)
	{
		*slash = 0;
	}
	std::wstring path = std::wstring(icoPath) + L"\\resources\\icon.ico";
	h = (HICON)LoadImageW(NULL, path.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
	if (!h)
	{
		path = std::wstring(icoPath) + L"\\icon.ico";
		h = (HICON)LoadImageW(NULL, path.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
	}
	if (!h)
	{
		h = LoadIconW(NULL, IDI_APPLICATION);
	}
	return h;
}

TrayIcon::TrayIcon()
	: m_hwnd(NULL)
	, m_id(1)
	, m_added(false)
{
}

TrayIcon::~TrayIcon()
{
	Remove();
}

bool TrayIcon::Add(HWND hwnd, UINT callbackMsg, const wchar_t* tip)
{
	Remove();
	if (!hwnd)
	{
		return false;
	}

	NOTIFYICONDATAW nid = {};
	nid.cbSize = sizeof(nid);
	nid.hWnd = hwnd;
	nid.uID = m_id;
	nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	nid.uCallbackMessage = callbackMsg;
	nid.hIcon = LoadAppIcon();

	wcsncpy_s(nid.szTip, tip ? tip : L"YouYou Screenshot", _TRUNCATE);

	if (!Shell_NotifyIconW(NIM_ADD, &nid))
	{
		return false;
	}
	m_hwnd = hwnd;
	m_added = true;
	return true;
}

void TrayIcon::Remove()
{
	if (!m_added || !m_hwnd)
	{
		return;
	}
	NOTIFYICONDATAW nid = {};
	nid.cbSize = sizeof(nid);
	nid.hWnd = m_hwnd;
	nid.uID = m_id;
	Shell_NotifyIconW(NIM_DELETE, &nid);
	m_added = false;
	m_hwnd = NULL;
}

void TrayIcon::ShowBalloon(const wchar_t* title, const wchar_t* text)
{
	if (!m_added || !m_hwnd)
	{
		return;
	}
	NOTIFYICONDATAW nid = {};
	nid.cbSize = sizeof(nid);
	nid.hWnd = m_hwnd;
	nid.uID = m_id;
	nid.uFlags = NIF_INFO;
	nid.dwInfoFlags = NIIF_INFO;
	wcsncpy_s(nid.szInfoTitle, title ? title : L"", _TRUNCATE);
	wcsncpy_s(nid.szInfo, text ? text : L"", _TRUNCATE);
	Shell_NotifyIconW(NIM_MODIFY, &nid);
}
}
