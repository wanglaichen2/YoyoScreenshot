#include "stdafx.h"
#include "MainWnd.h"
#include "../platform/MiniDump.h"
#include <direct.h>
#include <stdio.h>
#include <string.h>

CMainWnd g_MainWnd;

static void ChdirToExe()
{
	char modulePath[MAX_PATH] = {0};
	if (0 == GetModuleFileNameA(NULL, modulePath, MAX_PATH))
	{
		return;
	}
	char* slash = strrchr(modulePath, '\\');
	if (slash)
	{
		*slash = 0;
		_chdir(modulePath);
	}
}

int main()
{
	ChdirToExe();
	InitMiniDump();

	HANDLE mutex = CreateMutexA(NULL, FALSE, "Global\\YoyoScreenshot_Single");
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		CloseHandle(mutex);
		MessageBoxW(NULL, L"悠悠截图已在运行。", L"提示", MB_OK | MB_ICONINFORMATION);
		return 0;
	}

	if (!XInitXCGUI())
	{
		CloseHandle(mutex);
		return 1;
	}

	g_MainWnd.Create(120, 80, 980, 640);
	XRunXCGUI();

	CloseHandle(mutex);
	return 0;
}
