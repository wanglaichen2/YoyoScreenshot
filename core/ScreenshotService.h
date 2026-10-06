#pragma once

#include <windows.h>
#include <string>
#include <vector>

namespace spy
{
struct ShotFileInfo
{
	std::wstring path;
	std::wstring dateKey; // YYYY-MM-DD，用于分组
	bool isToday;
};

/** Capture window client+frame (or primary screen if hwnd==NULL) to BMP file. */
bool CaptureWindowToBmp(HWND hwnd, const std::wstring& filePath);
bool SaveHBitmapToBmpFile(HBITMAP hBitmap, const std::wstring& path);
std::wstring MakeDefaultShotPath();
std::wstring GetShotsDirectory();

/** Newest first, up to maxCount .bmp files under shots\. */
bool EnumRecentShotFiles(std::vector<std::wstring>& outPaths, int maxCount);

/** Newest first with date keys for history grouping. */
bool EnumShotFiles(std::vector<ShotFileInfo>& outFiles, int maxCount);

/** Copy BMP file to clipboard as CF_DIB (for pasting into chat/docs). */
bool CopyBmpFileToClipboard(const std::wstring& bmpPath);

/** Ask user where to save a copy of existing shot. */
bool SaveShotAsDialog(HWND owner, const std::wstring& srcPath);

/** Show BMP as a topmost floating pin window (drag to move, Esc/右键关闭). */
void ShowPinnedShot(const std::wstring& bmpPath);
}
