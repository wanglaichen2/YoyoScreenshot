#pragma once

#include <windows.h>
#include <string>

namespace spy
{
/** Modal options dialog (language + hide-on-capture). Returns true if OK. */
bool ShowSettingsDialog(HWND owner, bool& hideOnCapture, std::wstring& langPref);
}
