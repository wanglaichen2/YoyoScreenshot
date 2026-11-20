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

/** Capture window client+frame (or primary screen if hwnd==NULL) to image file (PNG). */
bool CaptureWindowToBmp(HWND hwnd, const std::wstring& filePath);
/** 将 HBITMAP 存为 PNG（函数名保留兼容旧调用）。 */
bool SaveHBitmapToBmpFile(HBITMAP hBitmap, const std::wstring& path);
std::wstring MakeDefaultShotPath();
std::wstring GetShotsDirectory();

/** Newest first, up to maxCount shot files (.png / 旧 .bmp) under shots\. */
bool EnumRecentShotFiles(std::vector<std::wstring>& outPaths, int maxCount);

/** Newest first with date keys for history grouping. */
bool EnumShotFiles(std::vector<ShotFileInfo>& outFiles, int maxCount);

/** 加载截图为 HBITMAP（支持 .png / .bmp；调用方 DeleteObject）。 */
HBITMAP LoadShotAsHBitmap(const std::wstring& path);

/** 把截图文件复制到剪贴板（CF_DIB，便于粘贴到聊天/文档）。 */
bool CopyBmpFileToClipboard(const std::wstring& shotPath);

/** Ask user where to save a copy of existing shot. */
bool SaveShotAsDialog(HWND owner, const std::wstring& srcPath);

/** 贴图预览（拖动移动，Esc/右键关闭）；支持 PNG/BMP。 */
void ShowPinnedShot(const std::wstring& shotPath);
}
