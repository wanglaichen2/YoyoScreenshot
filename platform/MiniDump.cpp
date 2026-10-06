#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>
#include <string.h>
#include <direct.h>
#include <string>
#include "MiniDump.h"

#pragma comment(lib, "dbghelp.lib")

namespace
{
char s_dumpDir[MAX_PATH] = {0};

bool EnsureDumpDir()
{
	if (s_dumpDir[0] != 0)
	{
		return true;
	}

	char modulePath[MAX_PATH] = {0};
	if (0 == GetModuleFileNameA(NULL, modulePath, MAX_PATH))
	{
		return false;
	}

	char* slash = strrchr(modulePath, '\\');
	if (NULL == slash)
	{
		slash = strrchr(modulePath, '/');
	}
	if (NULL != slash)
	{
		*slash = 0;
	}

	_snprintf(s_dumpDir, MAX_PATH, "%s\\dumps", modulePath);
	s_dumpDir[MAX_PATH - 1] = 0;
	CreateDirectoryA(s_dumpDir, NULL);
	return s_dumpDir[0] != 0;
}

std::string MakeDumpFileName()
{
	EnsureDumpDir();
	SYSTEMTIME tm = {};
	GetLocalTime(&tm);
	char path[MAX_PATH] = {0};
	_snprintf(path, MAX_PATH, "%s\\DUMP%04d%02d%02d%02d%02d%02d%03d.dmp",
		s_dumpDir[0] ? s_dumpDir : ".",
		tm.wYear, tm.wMonth, tm.wDay, tm.wHour, tm.wMinute, tm.wSecond,
		tm.wMilliseconds);
	path[MAX_PATH - 1] = 0;
	return path;
}

void WriteDump(EXCEPTION_POINTERS* ep, const std::string& file)
{
	HANDLE hFile = CreateFileA(file.c_str(), GENERIC_READ | GENERIC_WRITE,
		0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == NULL || hFile == INVALID_HANDLE_VALUE)
	{
		return;
	}
	MINIDUMP_EXCEPTION_INFORMATION info = {};
	info.ThreadId = GetCurrentThreadId();
	info.ExceptionPointers = ep;
	info.ClientPointers = FALSE;
	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile,
		MiniDumpNormal, ep ? &info : NULL, NULL, NULL);
	CloseHandle(hFile);
}

LONG WINAPI UnhandledDumpFilter(EXCEPTION_POINTERS* ep)
{
	WriteDump(ep, MakeDumpFileName());
	return EXCEPTION_EXECUTE_HANDLER;
}
}

bool GetMiniDumpDirectory(char* dir, unsigned int dirSize)
{
	EnsureDumpDir();
	if (NULL == dir || dirSize == 0)
	{
		return s_dumpDir[0] != 0;
	}
	_snprintf(dir, dirSize, "%s\\", s_dumpDir);
	dir[dirSize - 1] = 0;
	return s_dumpDir[0] != 0;
}

bool InitMiniDump()
{
	EnsureDumpDir();
	SetUnhandledExceptionFilter(UnhandledDumpFilter);
	return true;
}

bool InitMiniDumpFullMemory()
{
	return InitMiniDump();
}

void WriteMiniDumpNow(const char* reason)
{
	(void)reason;
	WriteDump(NULL, MakeDumpFileName());
}

void DisableSetUnhandledExceptionFilter()
{
}
