#include <windows.h>
#include <gdiplus.h>
#include <cstring>

using namespace Gdiplus;
static ULONG_PTR drawingToken;

extern "C" int initDrawing(void)
{
    GdiplusStartupInput input;
    return GdiplusStartup(&drawingToken, &input, nullptr) == Ok;
}
extern "C" void closeDrawing(void)
{
    if (drawingToken) GdiplusShutdown(drawingToken);
    drawingToken = 0;
}
static Color color(COLORREF value)
{
    return Color(255, GetRValue(value), GetGValue(value), GetBValue(value));
}
/* Render into a premultiplied-alpha surface at 4x resolution, then downsample.
   AlphaBlend keeps rounded edges transparent over the system Mica material. */
static void drawSurface(HDC destination, int x, int y, int width, int height,
    int radius, COLORREF fillColor, COLORREF borderColor, int borderWidth, bool circle)
{
    if (width <= 0 || height <= 0) return;
    Bitmap large(width * 4, height * 4, PixelFormat32bppPARGB);
    Graphics paint(&large);
    paint.Clear(Color(0, 0, 0, 0));
    paint.ScaleTransform(4.0f, 4.0f);
    paint.SetSmoothingMode(SmoothingModeAntiAlias);
    paint.SetPixelOffsetMode(PixelOffsetModeHighQuality);
    REAL inset = borderWidth ? borderWidth / 2.0f + 0.5f : 0.5f;
    REAL right = width - inset, bottom = height - inset;
    REAL diameter = static_cast<REAL>(radius * 2);
    if (diameter > right - inset) diameter = right - inset;
    if (diameter > bottom - inset) diameter = bottom - inset;
    GraphicsPath path;
    if (circle) path.AddEllipse(inset, inset, right - inset, bottom - inset);
    else {
        path.AddArc(inset, inset, diameter, diameter, 180.0f, 90.0f);
        path.AddArc(right - diameter, inset, diameter, diameter, 270.0f, 90.0f);
        path.AddArc(right - diameter, bottom - diameter, diameter, diameter, 0.0f, 90.0f);
        path.AddArc(inset, bottom - diameter, diameter, diameter, 90.0f, 90.0f);
        path.CloseFigure();
    }
    SolidBrush fill(color(fillColor));
    if (fillColor != CLR_INVALID) paint.FillPath(&fill, &path);
    if (borderWidth) {
        Pen pen(color(borderColor), static_cast<REAL>(borderWidth));
        paint.DrawPath(&pen, &path);
    }
    Bitmap output(width, height, PixelFormat32bppPARGB);
    Graphics scaled(&output);
    scaled.Clear(Color(0, 0, 0, 0));
    scaled.SetCompositingMode(CompositingModeSourceCopy);
    scaled.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    scaled.DrawImage(&large, Rect(0, 0, width, height), 0, 0, width * 4, height * 4, UnitPixel);
    Rect rectangle(0, 0, width, height);
    BitmapData pixels;
    if (output.LockBits(&rectangle, ImageLockModeRead, PixelFormat32bppPARGB, &pixels) != Ok) return;
    BITMAPINFO info = {};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    void *bits;
    HBITMAP bitmap = CreateDIBSection(destination, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (bitmap) {
        for (int row = 0; row < height; ++row)
            std::memcpy(static_cast<BYTE *>(bits) + row * width * 4,
                static_cast<BYTE *>(pixels.Scan0) + row * pixels.Stride, width * 4);
        HDC source = CreateCompatibleDC(destination);
        if (source) {
            HGDIOBJ old = SelectObject(source, bitmap);
            BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
            AlphaBlend(destination, x, y, width, height, source, 0, 0, width, height, blend);
            SelectObject(source, old); DeleteDC(source);
        }
        DeleteObject(bitmap);
    }
    output.UnlockBits(&pixels);
}
extern "C" void smoothRoundRect(HDC destination, int x, int y, int width, int height,
    int radius, COLORREF fillColor, COLORREF borderColor, int borderWidth)
{
    drawSurface(destination, x, y, width, height, radius, fillColor, borderColor, borderWidth, false);
}
extern "C" void smoothCircle(HDC destination, int x, int y, int diameter, COLORREF fillColor)
{
    drawSurface(destination, x, y, diameter, diameter, diameter / 2, fillColor, fillColor, 0, true);
}
