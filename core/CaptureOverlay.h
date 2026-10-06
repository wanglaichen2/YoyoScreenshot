#pragma once

#include <windows.h>
#include <string>

namespace spy
{
/** Interactive region capture with pen/rect/arrow annotations. Returns true if saved. */
bool RunRegionCaptureOverlay(std::wstring& outPath);

/** Annotate an existing BMP (secondary edit). Returns true if saved as a new file. */
bool RunAnnotateOnImage(const std::wstring& inPath, std::wstring& outPath);

/** Full-screen capture (no overlay) to path. */
bool CaptureFullScreenAnnotated(const std::wstring& filePath);
}
