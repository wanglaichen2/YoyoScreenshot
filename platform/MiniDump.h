#pragma once

#ifdef _WIN32

extern bool InitMiniDump();
extern bool InitMiniDumpFullMemory();
extern bool GetMiniDumpDirectory(char* dir, unsigned int dirSize);
extern void WriteMiniDumpNow(const char* reason);
extern void DisableSetUnhandledExceptionFilter();

#endif
