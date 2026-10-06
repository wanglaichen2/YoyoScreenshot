#include "CaptureOverlay.h"
#include "ScreenshotService.h"

#include <vector>
#include <string>
#include <cstring>
#include <stdio.h>
#include <math.h>

#pragma comment(lib, "msimg32.lib")

namespace spy
{
namespace
{
enum DrawTool
{
	kToolRect = 0,
	kToolEllipse = 1,
	kToolArrow = 2,
	kToolPen = 3,
	kToolMosaic = 4,
	kToolText = 5,
	kToolHighlight = 6,
	kToolStep = 7,
	kToolColorPick = 8
};

enum AnnKind
{
	kAnnPen = 1,
	kAnnRect = 2,
	kAnnArrow = 3,
	kAnnEllipse = 4,
	kAnnMosaic = 5,
	kAnnText = 6,
	kAnnStep = 7,
	kAnnHighlight = 8
};

struct Annotation
{
	AnnKind kind;
	COLORREF color;
	int width;
	std::vector<POINT> points;
	POINT a;
	POINT b;
	std::wstring text;
	int stepNo;
};

enum EditBtnId
{
	kBtnRect = 6001,
	kBtnEllipse,
	kBtnArrow,
	kBtnPen,
	kBtnMosaic,
	kBtnText,
	kBtnHighlight,
	kBtnStep,
	kBtnColor,
	kBtnOcr,
	kBtnTranslate,
	kBtnNote,
	kBtnScissors,
	kBtnImage,
	kBtnPin,
	kBtnUndo,
	kBtnSave,
	kBtnCancel,
	kBtnOk
};

struct EditBtn
{
	int id;
	RECT rc;
	bool sepAfter;
};

struct OverlayState
{
	HWND hwnd;
	HWND toolbar;
	HWND pixelHud;
	HBITMAP screenBmp;
	HBITMAP dimmedBmp;
	HBITMAP backBmp;
	HDC screenDc;
	HDC dimmedDc;
	HDC backDc;
	HGDIOBJ oldScreen;
	HGDIOBJ oldDimmed;
	HGDIOBJ oldBack;
	int screenW;
	int screenH;
	int screenX;
	int screenY;
	bool selecting;
	bool selectionLocked;
	bool dragging;
	POINT start;
	RECT sel;
	RECT lastPaintSel;
	DrawTool tool;
	COLORREF color;
	int penWidth;
	int nextStep;
	int colorIndex;
	std::vector<Annotation> anns;
	Annotation current;
	bool drawing;
	bool confirmed;
	bool cancelled;
	bool closed;
	bool annotateMode;
	std::wstring outPath;
	std::vector<EditBtn> editBtns;
	int hotBtn;
	int pressBtn;
	POINT cursorPos;
	COLORREF hoverColor;
	bool colorAsRgb;
	bool showPixelHud;
};

const wchar_t* kPixelHudClass = L"YoyoPixelHud";
const int kHudW = 168;
const int kHudH = 248;
const int kMagSize = 140;
const int kMagCells = 15;

OverlayState* g_ov = NULL;

const COLORREF kColors[] = {
	RGB(255, 77, 79),
	RGB(250, 173, 20),
	RGB(82, 196, 26),
	RGB(24, 144, 255),
	RGB(114, 46, 209),
	RGB(0, 0, 0),
	RGB(255, 255, 255)
};
const int kColorCount = (int)(sizeof(kColors) / sizeof(kColors[0]));

const wchar_t* kEditBarClass = L"YoyoEditBar";

RECT NormRect(POINT a, POINT b)
{
	RECT rc;
	rc.left = (a.x < b.x) ? a.x : b.x;
	rc.top = (a.y < b.y) ? a.y : b.y;
	rc.right = (a.x > b.x) ? a.x : b.x;
	rc.bottom = (a.y > b.y) ? a.y : b.y;
	return rc;
}

RECT InflateCopy(RECT rc, int pad)
{
	rc.left -= pad;
	rc.top -= pad;
	rc.right += pad;
	rc.bottom += pad;
	return rc;
}

RECT UnionTwo(RECT a, RECT b)
{
	if (a.right <= a.left || a.bottom <= a.top)
	{
		return b;
	}
	if (b.right <= b.left || b.bottom <= b.top)
	{
		return a;
	}
	RECT u;
	u.left = (a.left < b.left) ? a.left : b.left;
	u.top = (a.top < b.top) ? a.top : b.top;
	u.right = (a.right > b.right) ? a.right : b.right;
	u.bottom = (a.bottom > b.bottom) ? a.bottom : b.bottom;
	return u;
}

RECT ClampClient(RECT rc, int w, int h)
{
	if (rc.left < 0) rc.left = 0;
	if (rc.top < 0) rc.top = 0;
	if (rc.right > w) rc.right = w;
	if (rc.bottom > h) rc.bottom = h;
	return rc;
}

RECT AnnBounds(const Annotation& a)
{
	RECT rc = {0, 0, 0, 0};
	int pad = a.width + 18;
	if (a.kind == kAnnPen && !a.points.empty())
	{
		rc.left = rc.right = a.points[0].x;
		rc.top = rc.bottom = a.points[0].y;
		for (size_t i = 1; i < a.points.size(); ++i)
		{
			if (a.points[i].x < rc.left) rc.left = a.points[i].x;
			if (a.points[i].y < rc.top) rc.top = a.points[i].y;
			if (a.points[i].x > rc.right) rc.right = a.points[i].x;
			if (a.points[i].y > rc.bottom) rc.bottom = a.points[i].y;
		}
	}
	else if (a.kind == kAnnText || a.kind == kAnnStep)
	{
		const int tw = (a.kind == kAnnStep) ? 28 : 80;
		const int th = (a.kind == kAnnStep) ? 28 : 28;
		rc.left = a.a.x;
		rc.top = a.a.y;
		rc.right = a.a.x + tw;
		rc.bottom = a.a.y + th;
		pad = 8;
	}
	else
	{
		rc.left = (a.a.x < a.b.x) ? a.a.x : a.b.x;
		rc.top = (a.a.y < a.b.y) ? a.a.y : a.b.y;
		rc.right = (a.a.x > a.b.x) ? a.a.x : a.b.x;
		rc.bottom = (a.a.y > a.b.y) ? a.a.y : a.b.y;
	}
	return InflateCopy(rc, pad);
}

void InvalidateOverlayRect(RECT rc)
{
	if (!g_ov || !g_ov->hwnd)
	{
		return;
	}
	rc = ClampClient(rc, g_ov->screenW, g_ov->screenH);
	if (rc.right <= rc.left || rc.bottom <= rc.top)
	{
		return;
	}
	InvalidateRect(g_ov->hwnd, &rc, FALSE);
}

void InvalidateSelectionChange(const RECT& oldSel, const RECT& newSel)
{
	RECT dirty = UnionTwo(InflateCopy(oldSel, 4), InflateCopy(newSel, 4));
	InvalidateOverlayRect(dirty);
}

bool PtInSel(const RECT& rc, int x, int y)
{
	return x >= rc.left && x <= rc.right && y >= rc.top && y <= rc.bottom;
}

POINT ClampToSel(const RECT& rc, int x, int y)
{
	POINT p;
	p.x = x;
	p.y = y;
	if (p.x < rc.left) p.x = rc.left;
	if (p.x > rc.right) p.x = rc.right;
	if (p.y < rc.top) p.y = rc.top;
	if (p.y > rc.bottom) p.y = rc.bottom;
	return p;
}

void DrawArrowHead(HDC hdc, POINT from, POINT to, COLORREF color, int width)
{
	const double dx = (double)(to.x - from.x);
	const double dy = (double)(to.y - from.y);
	const double len = sqrt(dx * dx + dy * dy);
	if (len < 1.0)
	{
		return;
	}
	const double ux = dx / len;
	const double uy = dy / len;
	const double px = -uy;
	const double py = ux;
	const int ah = 14 + width;
	const int aw = 6 + width / 2;
	POINT tip = to;
	POINT left = {
		(LONG)(to.x - ux * ah + px * aw),
		(LONG)(to.y - uy * ah + py * aw)
	};
	POINT right = {
		(LONG)(to.x - ux * ah - px * aw),
		(LONG)(to.y - uy * ah - py * aw)
	};
	HPEN pen = CreatePen(PS_SOLID, 1, color);
	HBRUSH br = CreateSolidBrush(color);
	HGDIOBJ oldPen = SelectObject(hdc, pen);
	HGDIOBJ oldBr = SelectObject(hdc, br);
	POINT tri[3] = { tip, left, right };
	Polygon(hdc, tri, 3);
	SelectObject(hdc, oldPen);
	SelectObject(hdc, oldBr);
	DeleteObject(pen);
	DeleteObject(br);
}

void DrawMosaic(HDC hdc, POINT a, POINT b)
{
	RECT r = NormRect(a, b);
	const int w = r.right - r.left;
	const int h = r.bottom - r.top;
	if (w < 4 || h < 4)
	{
		return;
	}
	const int block = 10;
	int sw = w / block;
	int sh = h / block;
	if (sw < 1) sw = 1;
	if (sh < 1) sh = 1;
	HDC tmp = CreateCompatibleDC(hdc);
	HBITMAP tiny = CreateCompatibleBitmap(hdc, sw, sh);
	HGDIOBJ ot = SelectObject(tmp, tiny);
	SetStretchBltMode(tmp, COLORONCOLOR);
	StretchBlt(tmp, 0, 0, sw, sh, hdc, r.left, r.top, w, h, SRCCOPY);
	SetStretchBltMode(hdc, COLORONCOLOR);
	StretchBlt(hdc, r.left, r.top, w, h, tmp, 0, 0, sw, sh, SRCCOPY);
	SelectObject(tmp, ot);
	DeleteObject(tiny);
	DeleteDC(tmp);
}

void DrawAnnotation(HDC hdc, const Annotation& a)
{
	if (a.kind == kAnnMosaic)
	{
		DrawMosaic(hdc, a.a, a.b);
		return;
	}

	if (a.kind == kAnnHighlight)
	{
		RECT r = NormRect(a.a, a.b);
		const int ww = r.right - r.left;
		const int hh = r.bottom - r.top;
		if (ww > 2 && hh > 2)
		{
			BITMAPINFO bmi = {};
			bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmi.bmiHeader.biWidth = ww;
			bmi.bmiHeader.biHeight = -hh;
			bmi.bmiHeader.biPlanes = 1;
			bmi.bmiHeader.biBitCount = 32;
			bmi.bmiHeader.biCompression = BI_RGB;
			void* bits = NULL;
			HDC mdc = CreateCompatibleDC(hdc);
			HBITMAP mbmp = CreateDIBSection(mdc, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
			if (mbmp && bits)
			{
				HGDIOBJ om = SelectObject(mdc, mbmp);
				const BYTE r8 = GetRValue(a.color);
				const BYTE g8 = GetGValue(a.color);
				const BYTE b8 = GetBValue(a.color);
				DWORD* px = (DWORD*)bits;
				for (int i = 0; i < ww * hh; ++i)
				{
					px[i] = (80u << 24) | (r8 << 16) | (g8 << 8) | b8;
				}
				BLENDFUNCTION bf = {};
				bf.BlendOp = AC_SRC_OVER;
				bf.SourceConstantAlpha = 255;
				bf.AlphaFormat = AC_SRC_ALPHA;
				AlphaBlend(hdc, r.left, r.top, ww, hh, mdc, 0, 0, ww, hh, bf);
				SelectObject(mdc, om);
			}
			if (mbmp) DeleteObject(mbmp);
			DeleteDC(mdc);
		}
		HPEN pen = CreatePen(PS_SOLID, a.width, a.color);
		HGDIOBJ old = SelectObject(hdc, pen);
		HGDIOBJ oldBr = SelectObject(hdc, GetStockObject(NULL_BRUSH));
		Rectangle(hdc, a.a.x, a.a.y, a.b.x, a.b.y);
		SelectObject(hdc, oldBr);
		SelectObject(hdc, old);
		DeleteObject(pen);
		return;
	}

	if (a.kind == kAnnText)
	{
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, a.color);
		HFONT font = CreateFontW(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei UI");
		HGDIOBJ of = SelectObject(hdc, font);
		const wchar_t* t = a.text.empty() ? L"文字" : a.text.c_str();
		TextOutW(hdc, a.a.x, a.a.y, t, (int)wcslen(t));
		SelectObject(hdc, of);
		DeleteObject(font);
		return;
	}

	if (a.kind == kAnnStep)
	{
		const int r = 14;
		HBRUSH br = CreateSolidBrush(a.color);
		HPEN pen = CreatePen(PS_SOLID, 2, a.color);
		HGDIOBJ ob = SelectObject(hdc, br);
		HGDIOBJ op = SelectObject(hdc, pen);
		Ellipse(hdc, a.a.x - r, a.a.y - r, a.a.x + r, a.a.y + r);
		SelectObject(hdc, GetStockObject(NULL_BRUSH));
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, RGB(255, 255, 255));
		HFONT font = CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
		HGDIOBJ of = SelectObject(hdc, font);
		wchar_t buf[16];
		_snwprintf_s(buf, _TRUNCATE, L"%d", a.stepNo > 0 ? a.stepNo : 1);
		SIZE sz = {};
		GetTextExtentPoint32W(hdc, buf, (int)wcslen(buf), &sz);
		TextOutW(hdc, a.a.x - sz.cx / 2, a.a.y - sz.cy / 2, buf, (int)wcslen(buf));
		SelectObject(hdc, of);
		SelectObject(hdc, op);
		SelectObject(hdc, ob);
		DeleteObject(font);
		DeleteObject(pen);
		DeleteObject(br);
		return;
	}

	HPEN pen = CreatePen(PS_SOLID, a.width, a.color);
	HGDIOBJ old = SelectObject(hdc, pen);
	HGDIOBJ oldBr = SelectObject(hdc, GetStockObject(NULL_BRUSH));
	if (a.kind == kAnnPen && a.points.size() >= 2)
	{
		MoveToEx(hdc, a.points[0].x, a.points[0].y, NULL);
		for (size_t i = 1; i < a.points.size(); ++i)
		{
			LineTo(hdc, a.points[i].x, a.points[i].y);
		}
	}
	else if (a.kind == kAnnRect)
	{
		Rectangle(hdc, a.a.x, a.a.y, a.b.x, a.b.y);
	}
	else if (a.kind == kAnnEllipse)
	{
		Ellipse(hdc, a.a.x, a.a.y, a.b.x, a.b.y);
	}
	else if (a.kind == kAnnArrow)
	{
		MoveToEx(hdc, a.a.x, a.a.y, NULL);
		LineTo(hdc, a.b.x, a.b.y);
		DrawArrowHead(hdc, a.a, a.b, a.color, a.width);
	}
	SelectObject(hdc, oldBr);
	SelectObject(hdc, old);
	DeleteObject(pen);
}

void PaintOverlay(HWND hwnd)
{
	if (!g_ov || !g_ov->dimmedBmp || !g_ov->screenBmp || !g_ov->backBmp)
	{
		return;
	}
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);
	RECT dirty = ps.rcPaint;
	if (dirty.right <= dirty.left || dirty.bottom <= dirty.top)
	{
		dirty.left = 0;
		dirty.top = 0;
		dirty.right = g_ov->screenW;
		dirty.bottom = g_ov->screenH;
	}

	const int dx = dirty.left;
	const int dy = dirty.top;
	const int dw = dirty.right - dirty.left;
	const int dh = dirty.bottom - dirty.top;

	BitBlt(g_ov->backDc, dx, dy, dw, dh, g_ov->dimmedDc, dx, dy, SRCCOPY);

	RECT sel = g_ov->sel;
	const bool hasSel = (sel.right - sel.left > 2 && sel.bottom - sel.top > 2);
	if (hasSel)
	{
		BitBlt(g_ov->backDc, sel.left, sel.top, sel.right - sel.left, sel.bottom - sel.top,
			g_ov->screenDc, sel.left, sel.top, SRCCOPY);
		HPEN border = CreatePen(PS_SOLID, 2, RGB(0, 170, 255));
		HGDIOBJ ob = SelectObject(g_ov->backDc, border);
		HGDIOBJ obr = SelectObject(g_ov->backDc, GetStockObject(NULL_BRUSH));
		Rectangle(g_ov->backDc, sel.left, sel.top, sel.right, sel.bottom);
		SelectObject(g_ov->backDc, obr);
		SelectObject(g_ov->backDc, ob);
		DeleteObject(border);
	}

	for (size_t i = 0; i < g_ov->anns.size(); ++i)
	{
		DrawAnnotation(g_ov->backDc, g_ov->anns[i]);
	}
	if (g_ov->drawing)
	{
		DrawAnnotation(g_ov->backDc, g_ov->current);
	}

	BitBlt(hdc, dx, dy, dw, dh, g_ov->backDc, dx, dy, SRCCOPY);
	g_ov->lastPaintSel = sel;
	EndPaint(hwnd, &ps);
}

bool InitOverlayBuffers(OverlayState& st)
{
	if (!st.screenBmp || st.screenW <= 0 || st.screenH <= 0)
	{
		return false;
	}
	HDC screen = GetDC(NULL);
	st.screenDc = CreateCompatibleDC(screen);
	st.dimmedDc = CreateCompatibleDC(screen);
	st.backDc = CreateCompatibleDC(screen);
	st.dimmedBmp = CreateCompatibleBitmap(screen, st.screenW, st.screenH);
	st.backBmp = CreateCompatibleBitmap(screen, st.screenW, st.screenH);
	ReleaseDC(NULL, screen);
	if (!st.screenDc || !st.dimmedDc || !st.backDc || !st.dimmedBmp || !st.backBmp)
	{
		return false;
	}
	st.oldScreen = SelectObject(st.screenDc, st.screenBmp);
	st.oldDimmed = SelectObject(st.dimmedDc, st.dimmedBmp);
	st.oldBack = SelectObject(st.backDc, st.backBmp);

	BitBlt(st.dimmedDc, 0, 0, st.screenW, st.screenH, st.screenDc, 0, 0, SRCCOPY);
	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = st.screenW;
	bmi.bmiHeader.biHeight = -st.screenH;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	void* bits = NULL;
	HDC mdc = CreateCompatibleDC(st.dimmedDc);
	HBITMAP overlay = CreateDIBSection(mdc, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
	if (overlay && bits)
	{
		HGDIOBJ om = SelectObject(mdc, overlay);
		DWORD* px = (DWORD*)bits;
		const size_t n = (size_t)st.screenW * (size_t)st.screenH;
		for (size_t i = 0; i < n; ++i)
		{
			px[i] = 0x99000000;
		}
		BLENDFUNCTION bf = {};
		bf.BlendOp = AC_SRC_OVER;
		bf.SourceConstantAlpha = 255;
		bf.AlphaFormat = AC_SRC_ALPHA;
		AlphaBlend(st.dimmedDc, 0, 0, st.screenW, st.screenH, mdc, 0, 0, st.screenW, st.screenH, bf);
		SelectObject(mdc, om);
	}
	if (overlay) DeleteObject(overlay);
	DeleteDC(mdc);
	return true;
}

void ReleaseOverlayBuffers(OverlayState& st)
{
	if (st.screenDc && st.oldScreen)
	{
		SelectObject(st.screenDc, st.oldScreen);
		st.oldScreen = NULL;
	}
	if (st.dimmedDc && st.oldDimmed)
	{
		SelectObject(st.dimmedDc, st.oldDimmed);
		st.oldDimmed = NULL;
	}
	if (st.backDc && st.oldBack)
	{
		SelectObject(st.backDc, st.oldBack);
		st.oldBack = NULL;
	}
	if (st.screenDc) { DeleteDC(st.screenDc); st.screenDc = NULL; }
	if (st.dimmedDc) { DeleteDC(st.dimmedDc); st.dimmedDc = NULL; }
	if (st.backDc) { DeleteDC(st.backDc); st.backDc = NULL; }
	if (st.dimmedBmp) { DeleteObject(st.dimmedBmp); st.dimmedBmp = NULL; }
	if (st.backBmp) { DeleteObject(st.backBmp); st.backBmp = NULL; }
}

void DestroyToolbar()
{
	if (g_ov && g_ov->toolbar)
	{
		DestroyWindow(g_ov->toolbar);
		g_ov->toolbar = NULL;
		g_ov->editBtns.clear();
	}
}

void DestroyPixelHud()
{
	if (g_ov && g_ov->pixelHud)
	{
		DestroyWindow(g_ov->pixelHud);
		g_ov->pixelHud = NULL;
	}
}

void SetPixelHudVisible(bool visible)
{
	if (!g_ov)
	{
		return;
	}
	g_ov->showPixelHud = visible;
	if (!visible)
	{
		DestroyPixelHud();
	}
}

bool CopyTextToClipboard(HWND owner, const std::wstring& text)
{
	if (!OpenClipboard(owner))
	{
		return false;
	}
	EmptyClipboard();
	const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
	HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes);
	if (!mem)
	{
		CloseClipboard();
		return false;
	}
	void* p = GlobalLock(mem);
	if (p)
	{
		memcpy(p, text.c_str(), bytes);
		GlobalUnlock(mem);
	}
	SetClipboardData(CF_UNICODETEXT, mem);
	CloseClipboard();
	return true;
}

std::wstring FormatHoverColor(COLORREF c, bool asRgb)
{
	wchar_t buf[64] = {};
	if (asRgb)
	{
		_snwprintf_s(buf, _TRUNCATE, L"RGB(%d, %d, %d)",
			(int)GetRValue(c), (int)GetGValue(c), (int)GetBValue(c));
	}
	else
	{
		_snwprintf_s(buf, _TRUNCATE, L"#%02X%02X%02X",
			(int)GetRValue(c), (int)GetGValue(c), (int)GetBValue(c));
	}
	return buf;
}

void PaintPixelHud(HWND hwnd)
{
	if (!g_ov || !g_ov->screenDc)
	{
		return;
	}
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);
	RECT rc;
	GetClientRect(hwnd, &rc);
	HDC mem = CreateCompatibleDC(hdc);
	HBITMAP bmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
	HGDIOBJ om = SelectObject(mem, bmp);

	HBRUSH bg = CreateSolidBrush(RGB(250, 250, 250));
	FillRect(mem, &rc, bg);
	DeleteObject(bg);

	const int magL = (kHudW - kMagSize) / 2;
	const int magT = 10;
	const int magR = magL + kMagSize;
	const int magB = magT + kMagSize;

	const int cx = g_ov->cursorPos.x;
	const int cy = g_ov->cursorPos.y;
	const int half = kMagCells / 2;
	const int cell = kMagSize / kMagCells;
	const int drawSize = cell * kMagCells;
	const int ox = magL + (kMagSize - drawSize) / 2;
	const int oy = magT + (kMagSize - drawSize) / 2;

	HBRUSH magBg = CreateSolidBrush(RGB(30, 30, 30));
	RECT magRc = { magL, magT, magR, magB };
	FillRect(mem, &magRc, magBg);
	DeleteObject(magBg);

	for (int j = 0; j < kMagCells; ++j)
	{
		for (int i = 0; i < kMagCells; ++i)
		{
			int sx = cx - half + i;
			int sy = cy - half + j;
			COLORREF c = RGB(40, 40, 40);
			if (sx >= 0 && sy >= 0 && sx < g_ov->screenW && sy < g_ov->screenH)
			{
				c = GetPixel(g_ov->screenDc, sx, sy);
				if (c == CLR_INVALID)
				{
					c = RGB(40, 40, 40);
				}
			}
			RECT cellRc = {
				ox + i * cell,
				oy + j * cell,
				ox + (i + 1) * cell,
				oy + (j + 1) * cell
			};
			HBRUSH br = CreateSolidBrush(c);
			FillRect(mem, &cellRc, br);
			DeleteObject(br);
		}
	}

	// center pixel outline
	const int mid = half;
	RECT cross = {
		ox + mid * cell,
		oy + mid * cell,
		ox + (mid + 1) * cell,
		oy + (mid + 1) * cell
	};
	HPEN crossPen = CreatePen(PS_SOLID, 2, RGB(50, 50, 55));
	HGDIOBJ op = SelectObject(mem, crossPen);
	HGDIOBJ obr = SelectObject(mem, GetStockObject(NULL_BRUSH));
	Rectangle(mem, cross.left, cross.top, cross.right, cross.bottom);
	SelectObject(mem, obr);
	SelectObject(mem, op);
	DeleteObject(crossPen);

	HPEN border = CreatePen(PS_SOLID, 1, RGB(210, 210, 214));
	op = SelectObject(mem, border);
	obr = SelectObject(mem, GetStockObject(NULL_BRUSH));
	Rectangle(mem, magL, magT, magR, magB);
	RoundRect(mem, 0, 0, rc.right - 1, rc.bottom - 1, 12, 12);
	SelectObject(mem, obr);
	SelectObject(mem, op);
	DeleteObject(border);

	SetBkMode(mem, TRANSPARENT);
	SetTextColor(mem, RGB(40, 40, 44));
	HFONT font = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei UI");
	HGDIOBJ of = SelectObject(mem, font);

	int ty = magB + 10;
	wchar_t line[96] = {};
	_snwprintf_s(line, _TRUNCATE, L"坐标: %d, %d",
		g_ov->screenX + cx, g_ov->screenY + cy);
	TextOutW(mem, magL, ty, line, (int)wcslen(line));
	ty += 20;

	COLORREF hc = g_ov->hoverColor;
	RECT sw = { magL, ty + 1, magL + 14, ty + 15 };
	HBRUSH swBr = CreateSolidBrush(hc);
	FillRect(mem, &sw, swBr);
	DeleteObject(swBr);
	FrameRect(mem, &sw, (HBRUSH)GetStockObject(GRAY_BRUSH));
	std::wstring colorText = FormatHoverColor(hc, g_ov->colorAsRgb);
	TextOutW(mem, magL + 20, ty, colorText.c_str(), (int)colorText.size());
	ty += 22;

	TextOutW(mem, magL, ty, L"按 Q 复制颜色值", (int)wcslen(L"按 Q 复制颜色值"));
	ty += 18;
	TextOutW(mem, magL, ty, L"按 Shift 切换RGB/HEX", (int)wcslen(L"按 Shift 切换RGB/HEX"));

	SelectObject(mem, of);
	DeleteObject(font);

	BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
	SelectObject(mem, om);
	DeleteObject(bmp);
	DeleteDC(mem);
	EndPaint(hwnd, &ps);
}

LRESULT CALLBACK PixelHudWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_PAINT)
	{
		PaintPixelHud(hwnd);
		return 0;
	}
	if (msg == WM_ERASEBKGND)
	{
		return 1;
	}
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void EnsurePixelHudClass()
{
	static bool registered = false;
	if (registered)
	{
		return;
	}
	WNDCLASSW wc = {};
	wc.lpfnWndProc = PixelHudWndProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = kPixelHudClass;
	wc.hbrBackground = NULL;
	RegisterClassW(&wc);
	registered = true;
}

void UpdatePixelHud(int x, int y)
{
	if (!g_ov || !g_ov->hwnd || !g_ov->showPixelHud || !g_ov->screenDc)
	{
		return;
	}
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x >= g_ov->screenW) x = g_ov->screenW - 1;
	if (y >= g_ov->screenH) y = g_ov->screenH - 1;

	g_ov->cursorPos.x = x;
	g_ov->cursorPos.y = y;
	COLORREF c = GetPixel(g_ov->screenDc, x, y);
	g_ov->hoverColor = (c == CLR_INVALID) ? RGB(0, 0, 0) : c;

	EnsurePixelHudClass();

	int wx = g_ov->screenX + x + 24;
	int wy = g_ov->screenY + y + 24;
	if (wx + kHudW > g_ov->screenX + g_ov->screenW - 4)
	{
		wx = g_ov->screenX + x - kHudW - 24;
	}
	if (wy + kHudH > g_ov->screenY + g_ov->screenH - 4)
	{
		wy = g_ov->screenY + y - kHudH - 24;
	}
	if (wx < g_ov->screenX + 4) wx = g_ov->screenX + 4;
	if (wy < g_ov->screenY + 4) wy = g_ov->screenY + 4;

	if (!g_ov->pixelHud)
	{
		g_ov->pixelHud = CreateWindowExW(
			WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED,
			kPixelHudClass, L"",
			WS_POPUP | WS_VISIBLE,
			wx, wy, kHudW, kHudH,
			g_ov->hwnd, NULL, GetModuleHandle(NULL), NULL);
		if (g_ov->pixelHud)
		{
			SetLayeredWindowAttributes(g_ov->pixelHud, 0, 255, LWA_ALPHA);
			HRGN rgn = CreateRoundRectRgn(0, 0, kHudW + 1, kHudH + 1, 12, 12);
			SetWindowRgn(g_ov->pixelHud, rgn, TRUE);
		}
	}
	else
	{
		SetWindowPos(g_ov->pixelHud, HWND_TOPMOST, wx, wy, kHudW, kHudH,
			SWP_NOACTIVATE | SWP_SHOWWINDOW);
		InvalidateRect(g_ov->pixelHud, NULL, FALSE);
	}
}

bool SaveSelection()
{
	if (!g_ov || !g_ov->screenBmp || !g_ov->screenDc)
	{
		return false;
	}
	RECT sel = g_ov->sel;
	int w = sel.right - sel.left;
	int h = sel.bottom - sel.top;
	if (w < 2 || h < 2)
	{
		return false;
	}

	HDC screenDc = GetDC(NULL);
	HDC dst = CreateCompatibleDC(screenDc);
	HBITMAP outBmp = CreateCompatibleBitmap(screenDc, w, h);
	HGDIOBJ oldDst = SelectObject(dst, outBmp);
	BitBlt(dst, 0, 0, w, h, g_ov->screenDc, sel.left, sel.top, SRCCOPY);

	for (size_t i = 0; i < g_ov->anns.size(); ++i)
	{
		Annotation a = g_ov->anns[i];
		for (size_t p = 0; p < a.points.size(); ++p)
		{
			a.points[p].x -= sel.left;
			a.points[p].y -= sel.top;
		}
		a.a.x -= sel.left;
		a.a.y -= sel.top;
		a.b.x -= sel.left;
		a.b.y -= sel.top;
		DrawAnnotation(dst, a);
	}

	SelectObject(dst, oldDst);
	DeleteDC(dst);
	ReleaseDC(NULL, screenDc);

	g_ov->outPath = MakeDefaultShotPath();
	const bool ok = SaveHBitmapToBmpFile(outBmp, g_ov->outPath);
	if (ok)
	{
		CopyBmpFileToClipboard(g_ov->outPath);
	}
	DeleteObject(outBmp);
	return ok;
}

void FinishOverlay(bool ok)
{
	if (!g_ov)
	{
		return;
	}
	if (ok)
	{
		g_ov->confirmed = SaveSelection();
		if (!g_ov->confirmed)
		{
			g_ov->cancelled = true;
		}
	}
	else
	{
		g_ov->cancelled = true;
	}
	DestroyToolbar();
	DestroyPixelHud();
	g_ov->closed = true;
	if (g_ov->hwnd)
	{
		DestroyWindow(g_ov->hwnd);
		g_ov->hwnd = NULL;
	}
}

void RefreshEditBar()
{
	if (g_ov && g_ov->toolbar)
	{
		InvalidateRect(g_ov->toolbar, NULL, FALSE);
	}
}

bool PromptAnnotationText(HWND owner, std::wstring& out)
{
	out.clear();
	const int ew = 280;
	const int eh = 32;
	POINT pt = {};
	GetCursorPos(&pt);
	HWND edit = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, L"EDIT", L"",
		WS_POPUP | WS_BORDER | WS_VISIBLE | ES_AUTOHSCROLL,
		pt.x, pt.y, ew, eh, owner, NULL, GetModuleHandle(NULL), NULL);
	if (!edit)
	{
		out = L"文字";
		return true;
	}
	HFONT font = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei UI");
	SendMessageW(edit, WM_SETFONT, (WPARAM)font, TRUE);
	SetFocus(edit);
	SetWindowTextW(edit, L"");

	bool done = false;
	bool ok = false;
	MSG msg;
	while (!done && GetMessageW(&msg, NULL, 0, 0) > 0)
	{
		if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN)
		{
			ok = true;
			done = true;
			break;
		}
		if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)
		{
			ok = false;
			done = true;
			break;
		}
		if (msg.message == WM_LBUTTONDOWN)
		{
			POINT c = { (short)LOWORD(msg.lParam), (short)HIWORD(msg.lParam) };
			HWND hit = WindowFromPoint(msg.pt);
			if (hit != edit)
			{
				ok = true;
				done = true;
				break;
			}
			(void)c;
		}
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	if (ok)
	{
		wchar_t buf[512] = {};
		GetWindowTextW(edit, buf, 511);
		out = buf;
		if (out.empty())
		{
			out = L"文字";
		}
	}
	DestroyWindow(edit);
	DeleteObject(font);
	return ok;
}

int HitEditBtn(int x, int y)
{
	if (!g_ov)
	{
		return 0;
	}
	for (size_t i = 0; i < g_ov->editBtns.size(); ++i)
	{
		const RECT& r = g_ov->editBtns[i].rc;
		if (x >= r.left && x < r.right && y >= r.top && y < r.bottom)
		{
			return g_ov->editBtns[i].id;
		}
	}
	return 0;
}

bool IsToolSelected(int id)
{
	if (!g_ov) return false;
	switch (id)
	{
	case kBtnRect: return g_ov->tool == kToolRect;
	case kBtnEllipse: return g_ov->tool == kToolEllipse;
	case kBtnArrow: return g_ov->tool == kToolArrow;
	case kBtnPen: return g_ov->tool == kToolPen;
	case kBtnMosaic: return g_ov->tool == kToolMosaic;
	case kBtnText: return g_ov->tool == kToolText;
	case kBtnHighlight: return g_ov->tool == kToolHighlight;
	case kBtnStep: return g_ov->tool == kToolStep;
	case kBtnColor: return g_ov->tool == kToolColorPick;
	default: return false;
	}
}

bool IsBtnEnabled(int id)
{
	if (!g_ov) return false;
	if (id == kBtnUndo)
	{
		return !g_ov->anns.empty();
	}
	return true;
}

void DrawIconLine(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c, int w = 2)
{
	HPEN pen = CreatePen(PS_SOLID, w, c);
	HGDIOBJ op = SelectObject(hdc, pen);
	MoveToEx(hdc, x1, y1, NULL);
	LineTo(hdc, x2, y2);
	SelectObject(hdc, op);
	DeleteObject(pen);
}

void PaintEditBtnIcon(HDC hdc, const RECT& rc, int id, COLORREF ink)
{
	const int cx = (rc.left + rc.right) / 2;
	const int cy = (rc.top + rc.bottom) / 2;
	HPEN pen = CreatePen(PS_SOLID, 2, ink);
	HGDIOBJ op = SelectObject(hdc, pen);
	HGDIOBJ ob = SelectObject(hdc, GetStockObject(NULL_BRUSH));

	switch (id)
	{
	case kBtnRect:
		RoundRect(hdc, cx - 7, cy - 7, cx + 7, cy + 7, 3, 3);
		break;
	case kBtnEllipse:
		Ellipse(hdc, cx - 7, cy - 7, cx + 7, cy + 7);
		break;
	case kBtnArrow:
		MoveToEx(hdc, cx - 7, cy + 7, NULL);
		LineTo(hdc, cx + 6, cy - 6);
		DrawArrowHead(hdc, {cx - 7, cy + 7}, {cx + 6, cy - 6}, ink, 2);
		break;
	case kBtnPen:
		MoveToEx(hdc, cx - 6, cy + 6, NULL);
		LineTo(hdc, cx + 2, cy - 5);
		Ellipse(hdc, cx + 2, cy - 8, cx + 7, cy - 3);
		break;
	case kBtnMosaic:
	{
		HBRUSH b1 = CreateSolidBrush(ink);
		HBRUSH b2 = CreateSolidBrush(RGB(180, 180, 180));
		RECT c1 = { cx - 7, cy - 7, cx - 1, cy - 1 };
		RECT c2 = { cx - 1, cy - 7, cx + 5, cy - 1 };
		RECT c3 = { cx - 7, cy - 1, cx - 1, cy + 5 };
		RECT c4 = { cx - 1, cy - 1, cx + 5, cy + 5 };
		FillRect(hdc, &c1, b1);
		FillRect(hdc, &c2, b2);
		FillRect(hdc, &c3, b2);
		FillRect(hdc, &c4, b1);
		DeleteObject(b1);
		DeleteObject(b2);
		break;
	}
	case kBtnText:
	{
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, ink);
		HFONT f = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
		HGDIOBJ of = SelectObject(hdc, f);
		SIZE sz = {};
		GetTextExtentPoint32W(hdc, L"T", 1, &sz);
		TextOutW(hdc, cx - sz.cx / 2, cy - sz.cy / 2, L"T", 1);
		SelectObject(hdc, of);
		DeleteObject(f);
		break;
	}
	case kBtnHighlight:
		Rectangle(hdc, cx - 8, cy - 8, cx + 8, cy + 8);
		Rectangle(hdc, cx - 4, cy - 4, cx + 4, cy + 4);
		break;
	case kBtnStep:
	{
		Ellipse(hdc, cx - 8, cy - 8, cx + 8, cy + 8);
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, ink);
		HFONT f = CreateFontW(12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
		HGDIOBJ of = SelectObject(hdc, f);
		SIZE sz = {};
		GetTextExtentPoint32W(hdc, L"1", 1, &sz);
		TextOutW(hdc, cx - sz.cx / 2, cy - sz.cy / 2, L"1", 1);
		SelectObject(hdc, of);
		DeleteObject(f);
		break;
	}
	case kBtnColor:
	{
		HBRUSH br = CreateSolidBrush(g_ov ? g_ov->color : ink);
		HGDIOBJ obr = SelectObject(hdc, br);
		Ellipse(hdc, cx - 7, cy - 7, cx + 7, cy + 7);
		SelectObject(hdc, obr);
		DeleteObject(br);
		MoveToEx(hdc, cx + 4, cy + 4, NULL);
		LineTo(hdc, cx + 9, cy + 9);
		break;
	}
	case kBtnOcr:
	{
		MoveToEx(hdc, cx - 8, cy - 8, NULL); LineTo(hdc, cx - 4, cy - 8); LineTo(hdc, cx - 4, cy - 4);
		MoveToEx(hdc, cx + 8, cy - 8, NULL); LineTo(hdc, cx + 4, cy - 8); LineTo(hdc, cx + 4, cy - 4);
		MoveToEx(hdc, cx - 8, cy + 8, NULL); LineTo(hdc, cx - 4, cy + 8); LineTo(hdc, cx - 4, cy + 4);
		MoveToEx(hdc, cx + 8, cy + 8, NULL); LineTo(hdc, cx + 4, cy + 8); LineTo(hdc, cx + 4, cy + 4);
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, ink);
		HFONT f = CreateFontW(12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
		HGDIOBJ of = SelectObject(hdc, f);
		TextOutW(hdc, cx - 4, cy - 7, L"T", 1);
		SelectObject(hdc, of);
		DeleteObject(f);
		break;
	}
	case kBtnTranslate:
	{
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, ink);
		HFONT f = CreateFontW(11, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei UI");
		HGDIOBJ of = SelectObject(hdc, f);
		TextOutW(hdc, cx - 10, cy - 8, L"中A", 2);
		SelectObject(hdc, of);
		DeleteObject(f);
		break;
	}
	case kBtnNote:
		RoundRect(hdc, cx - 7, cy - 8, cx + 7, cy + 8, 2, 2);
		DrawIconLine(hdc, cx - 4, cy - 3, cx + 4, cy - 3, ink, 1);
		DrawIconLine(hdc, cx - 4, cy + 1, cx + 2, cy + 1, ink, 1);
		DrawIconLine(hdc, cx + 2, cy + 3, cx + 6, cy + 7, ink, 2);
		break;
	case kBtnScissors:
		Ellipse(hdc, cx - 8, cy - 3, cx - 2, cy + 3);
		Ellipse(hdc, cx + 2, cy - 3, cx + 8, cy + 3);
		MoveToEx(hdc, cx - 2, cy, NULL);
		LineTo(hdc, cx + 2, cy - 7);
		MoveToEx(hdc, cx + 2, cy, NULL);
		LineTo(hdc, cx - 2, cy - 7);
		break;
	case kBtnImage:
		Rectangle(hdc, cx - 8, cy - 6, cx + 8, cy + 6);
		MoveToEx(hdc, cx - 6, cy + 4, NULL);
		LineTo(hdc, cx - 1, cy - 1);
		LineTo(hdc, cx + 3, cy + 3);
		LineTo(hdc, cx + 6, cy - 2);
		Ellipse(hdc, cx + 2, cy - 4, cx + 5, cy - 1);
		break;
	case kBtnPin:
		Ellipse(hdc, cx - 3, cy - 7, cx + 3, cy - 1);
		MoveToEx(hdc, cx, cy - 1, NULL);
		LineTo(hdc, cx, cy + 8);
		MoveToEx(hdc, cx - 5, cy - 2, NULL);
		LineTo(hdc, cx + 5, cy - 2);
		break;
	case kBtnUndo:
		Arc(hdc, cx - 7, cy - 7, cx + 7, cy + 7, cx + 6, cy - 2, cx - 2, cy - 7);
		MoveToEx(hdc, cx - 6, cy - 7, NULL);
		LineTo(hdc, cx - 2, cy - 10);
		MoveToEx(hdc, cx - 6, cy - 7, NULL);
		LineTo(hdc, cx - 1, cy - 3);
		break;
	case kBtnSave:
		MoveToEx(hdc, cx, cy - 6, NULL);
		LineTo(hdc, cx, cy + 2);
		MoveToEx(hdc, cx - 4, cy - 1, NULL);
		LineTo(hdc, cx, cy + 3);
		LineTo(hdc, cx + 4, cy - 1);
		MoveToEx(hdc, cx - 7, cy + 6, NULL);
		LineTo(hdc, cx + 7, cy + 6);
		break;
	case kBtnCancel:
		MoveToEx(hdc, cx - 6, cy - 6, NULL);
		LineTo(hdc, cx + 6, cy + 6);
		MoveToEx(hdc, cx + 6, cy - 6, NULL);
		LineTo(hdc, cx - 6, cy + 6);
		break;
	case kBtnOk:
		MoveToEx(hdc, cx - 7, cy, NULL);
		LineTo(hdc, cx - 2, cy + 6);
		LineTo(hdc, cx + 8, cy - 6);
		break;
	default:
		break;
	}

	SelectObject(hdc, ob);
	SelectObject(hdc, op);
	DeleteObject(pen);
}

void PaintEditBar(HWND hwnd)
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);
	RECT rc;
	GetClientRect(hwnd, &rc);
	HDC mem = CreateCompatibleDC(hdc);
	HBITMAP bmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
	HGDIOBJ om = SelectObject(mem, bmp);

	HBRUSH bg = CreateSolidBrush(RGB(244, 244, 245));
	FillRect(mem, &rc, bg);
	DeleteObject(bg);

	HRGN rgn = CreateRoundRectRgn(0, 0, rc.right + 1, rc.bottom + 1, 22, 22);
	SelectClipRgn(mem, rgn);

	if (g_ov)
	{
		for (size_t i = 0; i < g_ov->editBtns.size(); ++i)
		{
			const EditBtn& b = g_ov->editBtns[i];
			const bool hot = (b.id == g_ov->hotBtn);
			const bool press = (b.id == g_ov->pressBtn);
			const bool sel = IsToolSelected(b.id);
			const bool en = IsBtnEnabled(b.id);

			if (sel || hot || press)
			{
				COLORREF fill = press ? RGB(210, 210, 214) : (sel ? RGB(220, 220, 224) : RGB(232, 232, 236));
				HBRUSH br = CreateSolidBrush(fill);
				HRGN btnRgn = CreateRoundRectRgn(b.rc.left, b.rc.top, b.rc.right, b.rc.bottom, 8, 8);
				FillRgn(mem, btnRgn, br);
				DeleteObject(btnRgn);
				DeleteObject(br);
			}

			COLORREF ink = en ? RGB(40, 40, 44) : RGB(180, 180, 184);
			PaintEditBtnIcon(mem, b.rc, b.id, ink);

			if (b.sepAfter)
			{
				HPEN sep = CreatePen(PS_SOLID, 1, RGB(210, 210, 214));
				HGDIOBJ op = SelectObject(mem, sep);
				const int sx = b.rc.right + 5;
				MoveToEx(mem, sx, 10, NULL);
				LineTo(mem, sx, rc.bottom - 10);
				SelectObject(mem, op);
				DeleteObject(sep);
			}
		}
	}

	SelectClipRgn(mem, NULL);
	DeleteObject(rgn);

	HPEN border = CreatePen(PS_SOLID, 1, RGB(220, 220, 224));
	HGDIOBJ op = SelectObject(mem, border);
	HGDIOBJ obr = SelectObject(mem, GetStockObject(NULL_BRUSH));
	RoundRect(mem, 0, 0, rc.right - 1, rc.bottom - 1, 22, 22);
	SelectObject(mem, obr);
	SelectObject(mem, op);
	DeleteObject(border);

	BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
	SelectObject(mem, om);
	DeleteObject(bmp);
	DeleteDC(mem);
	EndPaint(hwnd, &ps);
}

void OnEditCommand(int id)
{
	if (!g_ov)
	{
		return;
	}
	switch (id)
	{
	case kBtnRect: g_ov->tool = kToolRect; break;
	case kBtnEllipse: g_ov->tool = kToolEllipse; break;
	case kBtnArrow: g_ov->tool = kToolArrow; break;
	case kBtnPen: g_ov->tool = kToolPen; break;
	case kBtnMosaic: g_ov->tool = kToolMosaic; break;
	case kBtnText: g_ov->tool = kToolText; break;
	case kBtnHighlight: g_ov->tool = kToolHighlight; break;
	case kBtnStep: g_ov->tool = kToolStep; break;
	case kBtnColor:
		g_ov->colorIndex = (g_ov->colorIndex + 1) % kColorCount;
		g_ov->color = kColors[g_ov->colorIndex];
		g_ov->tool = kToolColorPick;
		break;
	case kBtnOcr:
	case kBtnTranslate:
	case kBtnNote:
	case kBtnScissors:
	case kBtnImage:
		MessageBoxW(g_ov->hwnd, L"该功能后续版本开放", L"截图编辑", MB_OK | MB_ICONINFORMATION);
		break;
	case kBtnPin:
		if (SaveSelection())
		{
			ShowPinnedShot(g_ov->outPath);
			g_ov->confirmed = true;
			DestroyToolbar();
			g_ov->closed = true;
			if (g_ov->hwnd)
			{
				DestroyWindow(g_ov->hwnd);
				g_ov->hwnd = NULL;
			}
		}
		break;
	case kBtnUndo:
		if (!g_ov->anns.empty())
		{
			RECT dirty = AnnBounds(g_ov->anns.back());
			g_ov->anns.pop_back();
			InvalidateOverlayRect(UnionTwo(dirty, InflateCopy(g_ov->sel, 4)));
		}
		break;
	case kBtnSave:
		if (SaveSelection())
		{
			SaveShotAsDialog(g_ov->hwnd, g_ov->outPath);
		}
		break;
	case kBtnCancel:
		FinishOverlay(false);
		return;
	case kBtnOk:
		FinishOverlay(true);
		return;
	default:
		break;
	}
	RefreshEditBar();
}

LRESULT CALLBACK EditBarWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_PAINT:
		PaintEditBar(hwnd);
		return 0;
	case WM_ERASEBKGND:
		return 1;
	case WM_MOUSEMOVE:
	{
		const int x = (short)LOWORD(lParam);
		const int y = (short)HIWORD(lParam);
		const int hit = HitEditBtn(x, y);
		if (g_ov && hit != g_ov->hotBtn)
		{
			g_ov->hotBtn = hit;
			InvalidateRect(hwnd, NULL, FALSE);
		}
		TRACKMOUSEEVENT tme = {};
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = hwnd;
		TrackMouseEvent(&tme);
		return 0;
	}
	case WM_MOUSELEAVE:
		if (g_ov)
		{
			g_ov->hotBtn = 0;
			g_ov->pressBtn = 0;
			InvalidateRect(hwnd, NULL, FALSE);
		}
		return 0;
	case WM_LBUTTONDOWN:
	{
		const int x = (short)LOWORD(lParam);
		const int y = (short)HIWORD(lParam);
		const int hit = HitEditBtn(x, y);
		if (g_ov && hit && IsBtnEnabled(hit))
		{
			g_ov->pressBtn = hit;
			SetCapture(hwnd);
			InvalidateRect(hwnd, NULL, FALSE);
		}
		return 0;
	}
	case WM_LBUTTONUP:
	{
		const int x = (short)LOWORD(lParam);
		const int y = (short)HIWORD(lParam);
		const int hit = HitEditBtn(x, y);
		const int press = g_ov ? g_ov->pressBtn : 0;
		if (g_ov)
		{
			g_ov->pressBtn = 0;
		}
		ReleaseCapture();
		if (press && hit == press && IsBtnEnabled(hit))
		{
			OnEditCommand(hit);
		}
		else
		{
			InvalidateRect(hwnd, NULL, FALSE);
		}
		return 0;
	}
	default:
		break;
	}
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void EnsureEditBarClass()
{
	static bool registered = false;
	if (registered)
	{
		return;
	}
	WNDCLASSW wc = {};
	wc.lpfnWndProc = EditBarWndProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = kEditBarClass;
	wc.hbrBackground = NULL;
	RegisterClassW(&wc);
	registered = true;
}

void LayoutEditButtons(int barW, int barH)
{
	if (!g_ov)
	{
		return;
	}
	g_ov->editBtns.clear();
	const int cell = 32;
	const int gap = 2;
	int x = 12;
	const int y = (barH - cell) / 2;

	auto add = [&](int id, bool sep) {
		EditBtn b = {};
		b.id = id;
		b.rc.left = x;
		b.rc.top = y;
		b.rc.right = x + cell;
		b.rc.bottom = y + cell;
		b.sepAfter = sep;
		g_ov->editBtns.push_back(b);
		x += cell + gap;
		if (sep)
		{
			x += 10;
		}
	};

	add(kBtnRect, false);
	add(kBtnEllipse, false);
	add(kBtnArrow, false);
	add(kBtnPen, false);
	add(kBtnMosaic, false);
	add(kBtnText, false);
	add(kBtnHighlight, false);
	add(kBtnStep, false);
	add(kBtnColor, true);

	add(kBtnOcr, false);
	add(kBtnTranslate, false);
	add(kBtnNote, false);
	add(kBtnScissors, false);
	add(kBtnImage, false);
	add(kBtnPin, true);

	add(kBtnUndo, false);
	add(kBtnSave, false);
	add(kBtnCancel, false);
	add(kBtnOk, false);

	(void)barW;
}

void PlaceToolbar()
{
	if (!g_ov || !g_ov->hwnd || !g_ov->selectionLocked)
	{
		DestroyToolbar();
		return;
	}
	EnsureEditBarClass();

	RECT sel = g_ov->sel;
	const int barW = 680;
	const int barH = 44;
	int x = sel.left + (sel.right - sel.left - barW) / 2;
	int y = sel.bottom + 12;
	if (x < 8) x = 8;
	if (x + barW > g_ov->screenW - 8) x = g_ov->screenW - barW - 8;
	if (y + barH > g_ov->screenH - 8)
	{
		y = sel.top - barH - 12;
	}
	if (y < 8) y = 8;

	LayoutEditButtons(barW, barH);

	if (!g_ov->toolbar)
	{
		g_ov->toolbar = CreateWindowExW(
			WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
			kEditBarClass, L"",
			WS_POPUP | WS_VISIBLE,
			g_ov->screenX + x, g_ov->screenY + y, barW, barH,
			g_ov->hwnd, NULL, GetModuleHandle(NULL), NULL);
		if (g_ov->toolbar)
		{
			SetLayeredWindowAttributes(g_ov->toolbar, 0, 255, LWA_ALPHA);
			HRGN rgn = CreateRoundRectRgn(0, 0, barW + 1, barH + 1, 22, 22);
			SetWindowRgn(g_ov->toolbar, rgn, TRUE);
		}
	}
	else
	{
		SetWindowPos(g_ov->toolbar, HWND_TOPMOST,
			g_ov->screenX + x, g_ov->screenY + y, barW, barH, SWP_SHOWWINDOW);
		HRGN rgn = CreateRoundRectRgn(0, 0, barW + 1, barH + 1, 22, 22);
		SetWindowRgn(g_ov->toolbar, rgn, TRUE);
		InvalidateRect(g_ov->toolbar, NULL, FALSE);
	}
}

AnnKind ToolToAnn(DrawTool t)
{
	switch (t)
	{
	case kToolPen: return kAnnPen;
	case kToolRect: return kAnnRect;
	case kToolEllipse: return kAnnEllipse;
	case kToolArrow: return kAnnArrow;
	case kToolMosaic: return kAnnMosaic;
	case kToolHighlight: return kAnnHighlight;
	case kToolText: return kAnnText;
	case kToolStep: return kAnnStep;
	default: return kAnnRect;
	}
}

bool IsClickPlaceTool(DrawTool t)
{
	return t == kToolText || t == kToolStep || t == kToolColorPick;
}

LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (!g_ov)
	{
		return DefWindowProcW(hwnd, msg, wParam, lParam);
	}
	switch (msg)
	{
	case WM_PAINT:
		PaintOverlay(hwnd);
		return 0;
	case WM_ERASEBKGND:
		return 1;
	case WM_LBUTTONDOWN:
	{
		const int x = (short)LOWORD(lParam);
		const int y = (short)HIWORD(lParam);
		SetCapture(hwnd);
		if (!g_ov->selectionLocked)
		{
			g_ov->selecting = true;
			g_ov->dragging = true;
			g_ov->start.x = x;
			g_ov->start.y = y;
			RECT oldSel = g_ov->sel;
			g_ov->sel = NormRect(g_ov->start, g_ov->start);
			SetPixelHudVisible(false);
			InvalidateSelectionChange(oldSel, g_ov->sel);
			return 0;
		}
		if (!PtInSel(g_ov->sel, x, y))
		{
			return 0;
		}
		POINT p = ClampToSel(g_ov->sel, x, y);

		if (g_ov->tool == kToolColorPick)
		{
			COLORREF c = GetPixel(g_ov->screenDc, p.x, p.y);
			if (c != CLR_INVALID)
			{
				g_ov->color = c;
			}
			g_ov->tool = kToolRect;
			RefreshEditBar();
			return 0;
		}

		if (g_ov->tool == kToolStep)
		{
			Annotation a = {};
			a.kind = kAnnStep;
			a.color = g_ov->color;
			a.width = g_ov->penWidth;
			a.a = p;
			a.b = p;
			a.stepNo = ++g_ov->nextStep;
			g_ov->anns.push_back(a);
			InvalidateOverlayRect(AnnBounds(a));
			RefreshEditBar();
			return 0;
		}

		if (g_ov->tool == kToolText)
		{
			std::wstring text;
			if (!PromptAnnotationText(hwnd, text))
			{
				return 0;
			}
			Annotation a = {};
			a.kind = kAnnText;
			a.color = g_ov->color;
			a.width = g_ov->penWidth;
			a.a = p;
			a.b = p;
			a.text = text;
			g_ov->anns.push_back(a);
			InvalidateOverlayRect(AnnBounds(a));
			RefreshEditBar();
			return 0;
		}

		g_ov->drawing = true;
		g_ov->current = Annotation();
		g_ov->current.color = g_ov->color;
		g_ov->current.width = g_ov->penWidth;
		g_ov->current.a = p;
		g_ov->current.b = p;
		g_ov->current.kind = ToolToAnn(g_ov->tool);
		if (g_ov->tool == kToolPen)
		{
			g_ov->current.points.push_back(p);
		}
		return 0;
	}
	case WM_MOUSEMOVE:
	{
		const int x = (short)LOWORD(lParam);
		const int y = (short)HIWORD(lParam);
		UpdatePixelHud(x, y);
		if (!(wParam & MK_LBUTTON))
		{
			return 0;
		}
		if (g_ov->selecting && g_ov->dragging)
		{
			POINT cur = {x, y};
			RECT oldSel = g_ov->sel;
			g_ov->sel = NormRect(g_ov->start, cur);
			InvalidateSelectionChange(oldSel, g_ov->sel);
			return 0;
		}
		if (g_ov->drawing)
		{
			RECT oldAnn = AnnBounds(g_ov->current);
			POINT p = ClampToSel(g_ov->sel, x, y);
			if (g_ov->current.kind == kAnnPen)
			{
				g_ov->current.points.push_back(p);
			}
			else
			{
				g_ov->current.b = p;
			}
			InvalidateOverlayRect(UnionTwo(oldAnn, AnnBounds(g_ov->current)));
		}
		return 0;
	}
	case WM_LBUTTONUP:
	{
		ReleaseCapture();
		const int x = (short)LOWORD(lParam);
		const int y = (short)HIWORD(lParam);
		if (g_ov->selecting && g_ov->dragging)
		{
			POINT cur = {x, y};
			RECT oldSel = g_ov->sel;
			g_ov->sel = NormRect(g_ov->start, cur);
			g_ov->selecting = false;
			g_ov->dragging = false;
			if (g_ov->sel.right - g_ov->sel.left > 4 && g_ov->sel.bottom - g_ov->sel.top > 4)
			{
				g_ov->selectionLocked = true;
				g_ov->tool = kToolRect;
				SetPixelHudVisible(false);
				PlaceToolbar();
			}
			else
			{
				// 选区太小，回到待选状态，重新显示像素窗
				g_ov->sel.left = g_ov->sel.right = g_ov->sel.top = g_ov->sel.bottom = 0;
				SetPixelHudVisible(true);
				UpdatePixelHud(x, y);
			}
			InvalidateSelectionChange(oldSel, g_ov->sel);
			return 0;
		}
		if (g_ov->drawing)
		{
			RECT oldAnn = AnnBounds(g_ov->current);
			POINT p = ClampToSel(g_ov->sel, x, y);
			if (g_ov->current.kind == kAnnPen)
			{
				g_ov->current.points.push_back(p);
			}
			else
			{
				g_ov->current.b = p;
			}
			const RECT nb = NormRect(g_ov->current.a, g_ov->current.b);
			const bool tiny = (g_ov->current.kind != kAnnPen) &&
				(nb.right - nb.left < 3 || nb.bottom - nb.top < 3);
			if (!tiny)
			{
				g_ov->anns.push_back(g_ov->current);
			}
			g_ov->drawing = false;
			InvalidateOverlayRect(UnionTwo(oldAnn, AnnBounds(g_ov->current)));
			RefreshEditBar();
		}
		return 0;
	}
	case WM_LBUTTONDBLCLK:
	{
		const int x = (short)LOWORD(lParam);
		const int y = (short)HIWORD(lParam);
		if (g_ov->selectionLocked && PtInSel(g_ov->sel, x, y))
		{
			g_ov->drawing = false;
			FinishOverlay(true);
			return 0;
		}
		return 0;
	}
	case WM_RBUTTONUP:
		if (g_ov->annotateMode)
		{
			FinishOverlay(false);
			return 0;
		}
		if (g_ov->selectionLocked && !g_ov->drawing)
		{
			RECT oldSel = g_ov->sel;
			g_ov->selectionLocked = false;
			g_ov->anns.clear();
			g_ov->nextStep = 0;
			g_ov->sel.left = g_ov->sel.right = g_ov->sel.top = g_ov->sel.bottom = 0;
			DestroyToolbar();
			SetPixelHudVisible(true);
			POINT pt = {};
			GetCursorPos(&pt);
			UpdatePixelHud(pt.x - g_ov->screenX, pt.y - g_ov->screenY);
			InvalidateSelectionChange(oldSel, g_ov->sel);
		}
		else
		{
			FinishOverlay(false);
		}
		return 0;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			FinishOverlay(false);
			return 0;
		}
		if (wParam == VK_RETURN && g_ov->selectionLocked)
		{
			FinishOverlay(true);
			return 0;
		}
		if (wParam == 'Q' || wParam == 'q')
		{
			CopyTextToClipboard(g_ov->hwnd, FormatHoverColor(g_ov->hoverColor, g_ov->colorAsRgb));
			return 0;
		}
		if (wParam == 'Z' && (GetKeyState(VK_CONTROL) & 0x8000) && !g_ov->anns.empty())
		{
			RECT dirty = AnnBounds(g_ov->anns.back());
			g_ov->anns.pop_back();
			InvalidateOverlayRect(UnionTwo(dirty, InflateCopy(g_ov->sel, 4)));
			RefreshEditBar();
			return 0;
		}
		return 0;
	case WM_KEYUP:
		if (wParam == VK_SHIFT)
		{
			g_ov->colorAsRgb = !g_ov->colorAsRgb;
			if (g_ov->pixelHud)
			{
				InvalidateRect(g_ov->pixelHud, NULL, FALSE);
			}
			return 0;
		}
		return 0;
	case WM_DESTROY:
		if (g_ov)
		{
			g_ov->closed = true;
		}
		return 0;
	default:
		break;
	}
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

HBITMAP CaptureVirtualScreen()
{
	const int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
	const int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
	const int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	const int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	HDC hdc = GetDC(NULL);
	HDC mem = CreateCompatibleDC(hdc);
	HBITMAP bmp = CreateCompatibleBitmap(hdc, w, h);
	HGDIOBJ old = SelectObject(mem, bmp);
	BitBlt(mem, 0, 0, w, h, hdc, x, y, SRCCOPY);
	SelectObject(mem, old);
	DeleteDC(mem);
	ReleaseDC(NULL, hdc);
	return bmp;
}
}

bool RunRegionCaptureOverlay(std::wstring& outPath)
{
	OverlayState state = {};
	state.screenX = GetSystemMetrics(SM_XVIRTUALSCREEN);
	state.screenY = GetSystemMetrics(SM_YVIRTUALSCREEN);
	state.screenW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	state.screenH = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	state.screenBmp = CaptureVirtualScreen();
	state.tool = kToolRect;
	state.color = RGB(255, 77, 79);
	state.penWidth = 3;
	state.colorIndex = 0;
	state.nextStep = 0;
	state.hotBtn = 0;
	state.pressBtn = 0;
	state.showPixelHud = true;
	state.colorAsRgb = false;
	if (!state.screenBmp)
	{
		return false;
	}
	if (!InitOverlayBuffers(state))
	{
		DeleteObject(state.screenBmp);
		ReleaseOverlayBuffers(state);
		return false;
	}

	static bool registered = false;
	if (!registered)
	{
		WNDCLASSW wc = {};
		wc.style = CS_DBLCLKS;
		wc.lpfnWndProc = OverlayWndProc;
		wc.hInstance = GetModuleHandle(NULL);
		wc.lpszClassName = L"YoyoCaptureOverlay";
		wc.hCursor = LoadCursor(NULL, IDC_CROSS);
		wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
		RegisterClassW(&wc);
		registered = true;
	}

	g_ov = &state;
	state.hwnd = CreateWindowExW(
		WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
		L"YoyoCaptureOverlay",
		L"Capture",
		WS_POPUP | WS_VISIBLE,
		state.screenX, state.screenY, state.screenW, state.screenH,
		NULL, NULL, GetModuleHandle(NULL), NULL);
	if (!state.hwnd)
	{
		ReleaseOverlayBuffers(state);
		DeleteObject(state.screenBmp);
		g_ov = NULL;
		return false;
	}

	SetForegroundWindow(state.hwnd);
	SetFocus(state.hwnd);

	MSG msg;
	while (!state.closed)
	{
		const BOOL gm = GetMessageW(&msg, NULL, 0, 0);
		if (gm <= 0)
		{
			break;
		}
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	ReleaseOverlayBuffers(state);
	if (state.screenBmp)
	{
		DeleteObject(state.screenBmp);
		state.screenBmp = NULL;
	}
	g_ov = NULL;

	if (state.confirmed)
	{
		outPath = state.outPath;
		return true;
	}
	return false;
}

bool CaptureFullScreenAnnotated(const std::wstring& filePath)
{
	return CaptureWindowToBmp(NULL, filePath);
}

bool RunAnnotateOnImage(const std::wstring& inPath, std::wstring& outPath)
{
	HBITMAP loaded = (HBITMAP)LoadImageW(NULL, inPath.c_str(), IMAGE_BITMAP, 0, 0,
		LR_LOADFROMFILE | LR_CREATEDIBSECTION);
	if (!loaded)
	{
		return false;
	}
	BITMAP bm = {};
	GetObject(loaded, sizeof(bm), &bm);
	if (bm.bmWidth <= 0 || bm.bmHeight <= 0)
	{
		DeleteObject(loaded);
		return false;
	}

	const int vx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	const int vy = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	const int sx = GetSystemMetrics(SM_XVIRTUALSCREEN);
	const int sy = GetSystemMetrics(SM_YVIRTUALSCREEN);
	int wndW = bm.bmWidth;
	int wndH = bm.bmHeight;
	if (wndW > vx) wndW = vx;
	if (wndH > vy) wndH = vy;

	HDC screenDc = GetDC(NULL);
	HDC srcDc = CreateCompatibleDC(screenDc);
	HDC dstDc = CreateCompatibleDC(screenDc);
	HGDIOBJ oldSrc = SelectObject(srcDc, loaded);
	HBITMAP display = CreateCompatibleBitmap(screenDc, wndW, wndH);
	HGDIOBJ oldDst = SelectObject(dstDc, display);
	SetStretchBltMode(dstDc, HALFTONE);
	StretchBlt(dstDc, 0, 0, wndW, wndH, srcDc, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
	SelectObject(dstDc, oldDst);
	SelectObject(srcDc, oldSrc);
	DeleteDC(dstDc);
	DeleteDC(srcDc);
	ReleaseDC(NULL, screenDc);
	DeleteObject(loaded);

	OverlayState state = {};
	state.screenX = sx + (vx - wndW) / 2;
	state.screenY = sy + (vy - wndH) / 2;
	state.screenW = wndW;
	state.screenH = wndH;
	state.screenBmp = display;
	state.tool = kToolRect;
	state.color = RGB(255, 77, 79);
	state.penWidth = 3;
	state.colorIndex = 0;
	state.nextStep = 0;
	state.annotateMode = true;
	state.selectionLocked = true;
	state.showPixelHud = false;
	state.colorAsRgb = false;
	state.sel.left = 0;
	state.sel.top = 0;
	state.sel.right = wndW;
	state.sel.bottom = wndH;
	if (!InitOverlayBuffers(state))
	{
		DeleteObject(state.screenBmp);
		ReleaseOverlayBuffers(state);
		return false;
	}

	static bool registered = false;
	if (!registered)
	{
		WNDCLASSW wc = {};
		wc.style = CS_DBLCLKS;
		wc.lpfnWndProc = OverlayWndProc;
		wc.hInstance = GetModuleHandle(NULL);
		wc.lpszClassName = L"YoyoCaptureOverlay";
		wc.hCursor = LoadCursor(NULL, IDC_CROSS);
		wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
		RegisterClassW(&wc);
		registered = true;
	}

	g_ov = &state;
	state.hwnd = CreateWindowExW(
		WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
		L"YoyoCaptureOverlay",
		L"Annotate",
		WS_POPUP | WS_VISIBLE,
		state.screenX, state.screenY, state.screenW, state.screenH,
		NULL, NULL, GetModuleHandle(NULL), NULL);
	if (!state.hwnd)
	{
		ReleaseOverlayBuffers(state);
		DeleteObject(state.screenBmp);
		g_ov = NULL;
		return false;
	}

	PlaceToolbar();

	SetForegroundWindow(state.hwnd);
	SetFocus(state.hwnd);

	MSG msg;
	while (!state.closed)
	{
		const BOOL gm = GetMessageW(&msg, NULL, 0, 0);
		if (gm <= 0)
		{
			break;
		}
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	ReleaseOverlayBuffers(state);
	if (state.screenBmp)
	{
		DeleteObject(state.screenBmp);
		state.screenBmp = NULL;
	}
	g_ov = NULL;

	if (state.confirmed)
	{
		outPath = state.outPath;
		return true;
	}
	return false;
}
} // namespace spy
