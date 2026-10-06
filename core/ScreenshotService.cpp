#include "ScreenshotService.h"
#include <stdio.h>
#include <time.h>
#include <vector>
#include <string>
#include <algorithm>
#include <commdlg.h>

namespace spy
{

bool SaveHBitmapToBmpFile(HBITMAP hBitmap, const std::wstring& path)
{
	BITMAP bmp = {};
	if (!GetObject(hBitmap, sizeof(bmp), &bmp))
	{
		return false;
	}

	BITMAPINFOHEADER bi = {};
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bmp.bmWidth;
	bi.biHeight = bmp.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = 24;
	bi.biCompression = BI_RGB;

	const DWORD stride = ((bmp.bmWidth * 3 + 3) & ~3);
	const DWORD imageSize = stride * bmp.bmHeight;
	std::vector<unsigned char> bits(imageSize);

	HDC hdc = GetDC(NULL);
	BITMAPINFO bmi = {};
	bmi.bmiHeader = bi;
	GetDIBits(hdc, hBitmap, 0, (UINT)bmp.bmHeight, &bits[0], &bmi, DIB_RGB_COLORS);
	ReleaseDC(NULL, hdc);

	BITMAPFILEHEADER bfh = {};
	bfh.bfType = 0x4D42;
	bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bfh.bfSize = bfh.bfOffBits + imageSize;

	FILE* fp = NULL;
	if (_wfopen_s(&fp, path.c_str(), L"wb") != 0 || !fp)
	{
		return false;
	}
	fwrite(&bfh, 1, sizeof(bfh), fp);
	fwrite(&bi, 1, sizeof(bi), fp);
	fwrite(&bits[0], 1, imageSize, fp);
	fclose(fp);
	return true;
}

std::wstring GetShotsDirectory()
{
	wchar_t dir[MAX_PATH] = {0};
	GetModuleFileNameW(NULL, dir, MAX_PATH);
	wchar_t* slash = wcsrchr(dir, L'\\');
	if (slash) *slash = 0;
	std::wstring shots = std::wstring(dir) + L"\\shots";
	CreateDirectoryW(shots.c_str(), NULL);
	return shots;
}

std::wstring MakeDefaultShotPath()
{
	time_t now = time(NULL);
	tm t = {};
	localtime_s(&t, &now);
	wchar_t path[MAX_PATH] = {0};
	_snwprintf_s(path, _countof(path), L"%s\\shot_%04d%02d%02d_%02d%02d%02d.bmp",
		GetShotsDirectory().c_str(),
		t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
	return path;
}

bool CaptureWindowToBmp(HWND hwnd, const std::wstring& filePath)
{
	RECT rc = {};
	HDC hdcSrc = NULL;
	if (hwnd && IsWindow(hwnd))
	{
		GetWindowRect(hwnd, &rc);
		hdcSrc = GetWindowDC(hwnd);
	}
	else
	{
		rc.left = 0;
		rc.top = 0;
		rc.right = GetSystemMetrics(SM_CXSCREEN);
		rc.bottom = GetSystemMetrics(SM_CYSCREEN);
		hdcSrc = GetDC(NULL);
	}
	if (!hdcSrc)
	{
		return false;
	}

	const int w = rc.right - rc.left;
	const int h = rc.bottom - rc.top;
	if (w <= 0 || h <= 0)
	{
		ReleaseDC(hwnd ? hwnd : NULL, hdcSrc);
		return false;
	}

	HDC hdcMem = CreateCompatibleDC(hdcSrc);
	HBITMAP hBmp = CreateCompatibleBitmap(hdcSrc, w, h);
	HGDIOBJ old = SelectObject(hdcMem, hBmp);
	BitBlt(hdcMem, 0, 0, w, h, hdcSrc, 0, 0, SRCCOPY);
	SelectObject(hdcMem, old);

	const bool ok = SaveHBitmapToBmpFile(hBmp, filePath);

	DeleteObject(hBmp);
	DeleteDC(hdcMem);
	ReleaseDC(hwnd ? hwnd : NULL, hdcSrc);
	return ok;
}

bool EnumRecentShotFiles(std::vector<std::wstring>& outPaths, int maxCount)
{
	std::vector<ShotFileInfo> files;
	if (!EnumShotFiles(files, maxCount))
	{
		return false;
	}
	outPaths.clear();
	outPaths.reserve(files.size());
	for (size_t i = 0; i < files.size(); ++i)
	{
		outPaths.push_back(files[i].path);
	}
	return !outPaths.empty();
}

namespace
{
std::wstring TodayDateKey()
{
	SYSTEMTIME st = {};
	GetLocalTime(&st);
	wchar_t key[32] = {0};
	_snwprintf_s(key, _countof(key), L"%04u-%02u-%02u",
		(unsigned)st.wYear, (unsigned)st.wMonth, (unsigned)st.wDay);
	return key;
}

std::wstring DateKeyFromShotPath(const std::wstring& path)
{
	const wchar_t* base = wcsrchr(path.c_str(), L'\\');
	base = base ? base + 1 : path.c_str();
	// shot_YYYYMMDD_HHMMSS.bmp
	if (wcsncmp(base, L"shot_", 5) == 0)
	{
		unsigned y = 0, mo = 0, d = 0;
		const wchar_t* p = base + 5;
		for (int i = 0; i < 4 && *p >= L'0' && *p <= L'9'; ++i, ++p) y = y * 10 + (*p - L'0');
		for (int i = 0; i < 2 && *p >= L'0' && *p <= L'9'; ++i, ++p) mo = mo * 10 + (*p - L'0');
		for (int i = 0; i < 2 && *p >= L'0' && *p <= L'9'; ++i, ++p) d = d * 10 + (*p - L'0');
		if (y >= 2000 && mo >= 1 && mo <= 12 && d >= 1 && d <= 31)
		{
			wchar_t key[32] = {0};
			_snwprintf_s(key, _countof(key), L"%04u-%02u-%02u", y, mo, d);
			return key;
		}
	}
	WIN32_FILE_ATTRIBUTE_DATA fad = {};
	if (GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fad))
	{
		FILETIME local = {};
		SYSTEMTIME st = {};
		FileTimeToLocalFileTime(&fad.ftLastWriteTime, &local);
		FileTimeToSystemTime(&local, &st);
		wchar_t key[32] = {0};
		_snwprintf_s(key, _countof(key), L"%04u-%02u-%02u",
			(unsigned)st.wYear, (unsigned)st.wMonth, (unsigned)st.wDay);
		return key;
	}
	return L"未知日期";
}
}

bool EnumShotFiles(std::vector<ShotFileInfo>& outFiles, int maxCount)
{
	outFiles.clear();
	if (maxCount <= 0)
	{
		return false;
	}
	const std::wstring dir = GetShotsDirectory();
	const std::wstring pattern = dir + L"\\*.bmp";
	WIN32_FIND_DATAW fd = {};
	HANDLE h = FindFirstFileW(pattern.c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	std::vector<std::wstring> paths;
	do
	{
		if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			paths.push_back(dir + L"\\" + fd.cFileName);
		}
	} while (FindNextFileW(h, &fd));
	FindClose(h);

	std::sort(paths.begin(), paths.end(), [](const std::wstring& a, const std::wstring& b) {
		return _wcsicmp(a.c_str(), b.c_str()) > 0;
	});
	if ((int)paths.size() > maxCount)
	{
		paths.resize((size_t)maxCount);
	}
	const std::wstring today = TodayDateKey();
	outFiles.reserve(paths.size());
	for (size_t i = 0; i < paths.size(); ++i)
	{
		ShotFileInfo info;
		info.path = paths[i];
		info.dateKey = DateKeyFromShotPath(paths[i]);
		info.isToday = (_wcsicmp(info.dateKey.c_str(), today.c_str()) == 0);
		outFiles.push_back(info);
	}
	return !outFiles.empty();
}

bool CopyBmpFileToClipboard(const std::wstring& bmpPath)
{
	HANDLE hf = CreateFileW(bmpPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hf == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	const DWORD size = GetFileSize(hf, NULL);
	if (size < sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER))
	{
		CloseHandle(hf);
		return false;
	}
	std::vector<unsigned char> file(size);
	DWORD read = 0;
	const BOOL okRead = ReadFile(hf, &file[0], size, &read, NULL);
	CloseHandle(hf);
	if (!okRead || read != size)
	{
		return false;
	}

	BITMAPFILEHEADER* bfh = (BITMAPFILEHEADER*)&file[0];
	if (bfh->bfType != 0x4D42 || bfh->bfOffBits >= size)
	{
		return false;
	}
	const DWORD dibSize = size - bfh->bfOffBits + sizeof(BITMAPINFOHEADER);
	// CF_DIB = BITMAPINFOHEADER + bits (no file header)
	const DWORD dibBytes = size - sizeof(BITMAPFILEHEADER);
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, dibBytes);
	if (!hMem)
	{
		return false;
	}
	void* p = GlobalLock(hMem);
	if (!p)
	{
		GlobalFree(hMem);
		return false;
	}
	memcpy(p, &file[sizeof(BITMAPFILEHEADER)], dibBytes);
	GlobalUnlock(hMem);

	if (!OpenClipboard(NULL))
	{
		GlobalFree(hMem);
		return false;
	}
	EmptyClipboard();
	const bool ok = SetClipboardData(CF_DIB, hMem) != NULL;
	CloseClipboard();
	if (!ok)
	{
		GlobalFree(hMem);
	}
	return ok;
	(void)dibSize;
}

bool SaveShotAsDialog(HWND owner, const std::wstring& srcPath)
{
	wchar_t file[MAX_PATH] = L"shot_copy.bmp";
	OPENFILENAMEW ofn = {};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = owner;
	ofn.lpstrFilter = L"BMP 图片\0*.bmp\0所有文件\0*.*\0";
	ofn.lpstrFile = file;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
	ofn.lpstrDefExt = L"bmp";
	if (!GetSaveFileNameW(&ofn))
	{
		return false;
	}
	return CopyFileW(srcPath.c_str(), file, FALSE) != FALSE;
}

namespace
{
struct PinWndState
{
	HBITMAP bmp;
	int w;
	int h;
	bool dragging;
	POINT dragPt;
};

LRESULT CALLBACK PinShotWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PinWndState* st = reinterpret_cast<PinWndState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
	switch (msg)
	{
	case WM_CREATE:
	{
		CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
		return 0;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		if (st && st->bmp)
		{
			HDC mem = CreateCompatibleDC(hdc);
			HGDIOBJ old = SelectObject(mem, st->bmp);
			BitBlt(hdc, 0, 0, st->w, st->h, mem, 0, 0, SRCCOPY);
			SelectObject(mem, old);
			DeleteDC(mem);
		}
		EndPaint(hwnd, &ps);
		return 0;
	}
	case WM_LBUTTONDOWN:
		if (st)
		{
			st->dragging = true;
			st->dragPt.x = (short)LOWORD(lParam);
			st->dragPt.y = (short)HIWORD(lParam);
			SetCapture(hwnd);
		}
		return 0;
	case WM_MOUSEMOVE:
		if (st && st->dragging && (wParam & MK_LBUTTON))
		{
			POINT cur = {(short)LOWORD(lParam), (short)HIWORD(lParam)};
			RECT rc = {};
			GetWindowRect(hwnd, &rc);
			SetWindowPos(hwnd, NULL,
				rc.left + (cur.x - st->dragPt.x),
				rc.top + (cur.y - st->dragPt.y),
				0, 0, SWP_NOSIZE | SWP_NOZORDER);
		}
		return 0;
	case WM_LBUTTONUP:
		if (st)
		{
			st->dragging = false;
		}
		ReleaseCapture();
		return 0;
	case WM_RBUTTONUP:
	case WM_KEYDOWN:
		if (msg == WM_RBUTTONUP || wParam == VK_ESCAPE || wParam == VK_RETURN)
		{
			DestroyWindow(hwnd);
		}
		return 0;
	case WM_DESTROY:
		if (st)
		{
			if (st->bmp)
			{
				DeleteObject(st->bmp);
			}
			delete st;
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
		}
		return 0;
	default:
		break;
	}
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}
}

void ShowPinnedShot(const std::wstring& bmpPath)
{
	HBITMAP loaded = (HBITMAP)LoadImageW(NULL, bmpPath.c_str(), IMAGE_BITMAP, 0, 0,
		LR_LOADFROMFILE | LR_CREATEDIBSECTION);
	if (!loaded)
	{
		return;
	}
	BITMAP bm = {};
	GetObject(loaded, sizeof(bm), &bm);
	if (bm.bmWidth <= 0 || bm.bmHeight <= 0)
	{
		DeleteObject(loaded);
		return;
	}

	static bool registered = false;
	if (!registered)
	{
		WNDCLASSW wc = {};
		wc.lpfnWndProc = PinShotWndProc;
		wc.hInstance = GetModuleHandle(NULL);
		wc.lpszClassName = L"YoyoPinnedShot";
		wc.hCursor = LoadCursor(NULL, IDC_SIZEALL);
		wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		RegisterClassW(&wc);
		registered = true;
	}

	PinWndState* st = new PinWndState();
	st->bmp = loaded;
	st->w = bm.bmWidth;
	st->h = bm.bmHeight;
	st->dragging = false;

	const int vx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	const int vy = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	const int sx = GetSystemMetrics(SM_XVIRTUALSCREEN);
	const int sy = GetSystemMetrics(SM_YVIRTUALSCREEN);
	int showW = st->w;
	int showH = st->h;
	if (showW > vx * 8 / 10) showW = vx * 8 / 10;
	if (showH > vy * 8 / 10) showH = vy * 8 / 10;
	const int x = sx + (vx - showW) / 2;
	const int y = sy + (vy - showH) / 2;

	HWND hwnd = CreateWindowExW(
		WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
		L"YoyoPinnedShot",
		L"贴图",
		WS_POPUP | WS_VISIBLE | WS_BORDER,
		x, y, showW, showH,
		NULL, NULL, GetModuleHandle(NULL), st);
	if (!hwnd)
	{
		DeleteObject(loaded);
		delete st;
		return;
	}
	if (showW != st->w || showH != st->h)
	{
		// 保留原图尺寸窗口；若屏幕放不下则缩小客户区并拉伸绘制
		st->w = showW;
		st->h = showH;
		// 重新做成适应尺寸的位图
		HDC screen = GetDC(NULL);
		HDC src = CreateCompatibleDC(screen);
		HDC dst = CreateCompatibleDC(screen);
		HGDIOBJ oldSrc = SelectObject(src, loaded);
		HBITMAP scaled = CreateCompatibleBitmap(screen, showW, showH);
		HGDIOBJ oldDst = SelectObject(dst, scaled);
		SetStretchBltMode(dst, HALFTONE);
		StretchBlt(dst, 0, 0, showW, showH, src, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
		SelectObject(dst, oldDst);
		SelectObject(src, oldSrc);
		DeleteDC(dst);
		DeleteDC(src);
		ReleaseDC(NULL, screen);
		DeleteObject(loaded);
		st->bmp = scaled;
		SetWindowPos(hwnd, NULL, 0, 0, showW, showH, SWP_NOMOVE | SWP_NOZORDER);
	}
	SetForegroundWindow(hwnd);
}
}
