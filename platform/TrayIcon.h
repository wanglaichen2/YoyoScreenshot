#pragma once

#include <windows.h>
#include <string>

namespace spy
{
class TrayIcon
{
public:
	TrayIcon();
	~TrayIcon();

	bool Add(HWND hwnd, UINT callbackMsg, const wchar_t* tip);
	void Remove();
	void ShowBalloon(const wchar_t* title, const wchar_t* text);

private:
	HWND m_hwnd;
	UINT m_id;
	bool m_added;
};
}
