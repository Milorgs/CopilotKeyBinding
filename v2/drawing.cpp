#include <windows.h>
#include <cmath>

static HDC sourceDc;
static HBITMAP surface;
static HGDIOBJ originalBitmap;
static DWORD *surfaceBits;
static int surfaceWidth, surfaceHeight;

extern "C" int initDrawing(void)
{
    sourceDc = CreateCompatibleDC(nullptr);
    return sourceDc != nullptr;
}
extern "C" void closeDrawing(void)
{
    if (sourceDc && originalBitmap) SelectObject(sourceDc, originalBitmap);
    if (surface) DeleteObject(surface);
    if (sourceDc) DeleteDC(sourceDc);
    sourceDc = nullptr; surface = nullptr; originalBitmap = nullptr;
    surfaceBits = nullptr; surfaceWidth = surfaceHeight = 0;
}
static bool ensureSurface(int width, int height)
{
    if (surface && width <= surfaceWidth && height <= surfaceHeight) return true;
    int nextWidth = width > surfaceWidth ? width : surfaceWidth;
    int nextHeight = height > surfaceHeight ? height : surfaceHeight;
    BITMAPINFO info = {};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = nextWidth;
    info.bmiHeader.biHeight = -nextHeight;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    void *bits;
    HBITMAP next = CreateDIBSection(sourceDc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!next) return false;
    HGDIOBJ previous = SelectObject(sourceDc, next);
    if (!originalBitmap) originalBitmap = previous;
    if (surface) DeleteObject(surface);
    surface = next; surfaceBits = static_cast<DWORD *>(bits);
    surfaceWidth = nextWidth; surfaceHeight = nextHeight;
    return true;
}
static float maximum(float a, float b) { return a > b ? a : b; }
static float minimum(float a, float b) { return a < b ? a : b; }
struct Shape {
    float centerX, centerY, halfWidth, halfHeight, radius;
    bool circle;
    float distance(float x, float y) const {
        float dx = std::fabs(x - centerX), dy = std::fabs(y - centerY);
        if (circle) return std::sqrt(dx * dx + dy * dy) - radius;
        float qx = dx - (halfWidth - radius), qy = dy - (halfHeight - radius);
        if (qx <= 0.0f || qy <= 0.0f) return maximum(qx, qy) - radius;
        return std::sqrt(qx * qx + qy * qy) - radius;
    }
};
static DWORD opaque(COLORREF color)
{
    return 0xff000000u | (GetRValue(color) << 16) | (GetGValue(color) << 8) | GetBValue(color);
}
/* Only boundary pixels need 8x8 coverage samples. Interiors are filled directly
   into one reusable, native-resolution premultiplied-alpha DIB. */
static void drawSurface(HDC destination, int x, int y, int width, int height,
    int radius, COLORREF fillColor, COLORREF borderColor, int borderWidth, bool circle)
{
    if (width <= 0 || height <= 0 || !ensureSurface(width, height)) return;
    float halfBorder = borderWidth / 2.0f;
    float inset = halfBorder + 0.5f;
    Shape shape = {width / 2.0f, height / 2.0f, width / 2.0f - inset, height / 2.0f - inset,
        static_cast<float>(radius), circle};
    shape.radius = minimum(shape.radius, minimum(shape.halfWidth, shape.halfHeight));
    const bool filled = fillColor != CLR_INVALID;
    const DWORD fillPixel = filled ? opaque(fillColor) : 0;
    const DWORD borderPixel = opaque(borderColor);
    for (int row = 0; row < height; ++row) {
        DWORD *pixels = surfaceBits + row * surfaceWidth;
        for (int column = 0; column < width; ++column) {
            float d = shape.distance(column + 0.5f, row + 0.5f);
            if (d < -halfBorder - 0.71f) { pixels[column] = fillPixel; continue; }
            if (d > halfBorder + 0.71f) { pixels[column] = 0; continue; }
            if (halfBorder > 0.71f && std::fabs(d) < halfBorder - 0.71f) {
                pixels[column] = borderPixel; continue;
            }
            unsigned int fillHits = 0, borderHits = 0;
            for (int sy = 0; sy < 8; ++sy) for (int sx = 0; sx < 8; ++sx) {
                float sample = shape.distance(column + (sx + 0.5f) / 8.0f, row + (sy + 0.5f) / 8.0f);
                if (borderWidth && std::fabs(sample) <= halfBorder) ++borderHits;
                else if (filled && sample <= 0.0f) ++fillHits;
            }
            unsigned int alpha = ((fillHits + borderHits) * 255 + 32) / 64;
            unsigned int red = (fillHits * GetRValue(fillColor) + borderHits * GetRValue(borderColor) + 32) / 64;
            unsigned int green = (fillHits * GetGValue(fillColor) + borderHits * GetGValue(borderColor) + 32) / 64;
            unsigned int blue = (fillHits * GetBValue(fillColor) + borderHits * GetBValue(borderColor) + 32) / 64;
            pixels[column] = (alpha << 24) | (red << 16) | (green << 8) | blue;
        }
    }
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    AlphaBlend(destination, x, y, width, height, sourceDc, 0, 0, width, height, blend);
    GdiFlush();
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
static float segmentDistance(float x, float y, float ax, float ay, float bx, float by)
{
    float dx = bx - ax, dy = by - ay;
    float t = maximum(0.0f, minimum(1.0f, ((x - ax) * dx + (y - ay) * dy) / (dx * dx + dy * dy)));
    dx = x - ax - t * dx; dy = y - ay - t * dy;
    return std::sqrt(dx * dx + dy * dy);
}
extern "C" void smoothArrow(HDC destination, int x, int y, int size, COLORREF color, int diagonal)
{
    if (size <= 0 || !ensureSurface(size, size)) return;
    const float segments[2][3][4] = {
        {{3,9,15,9}, {3,9,8,4}, {3,9,8,14}},
        {{4,14,14,4}, {6,4,14,4}, {14,4,14,12}}
    };
    for (int row = 0; row < size; ++row) for (int column = 0; column < size; ++column) {
        unsigned int hits = 0;
        for (int sy = 0; sy < 8; ++sy) for (int sx = 0; sx < 8; ++sx) {
            float sampleX = (column + (sx + 0.5f) / 8) * 18 / size;
            float sampleY = (row + (sy + 0.5f) / 8) * 18 / size;
            float distance = 100;
            for (int line = 0; line < 3; ++line) {
                const float *s = segments[diagonal ? 1 : 0][line];
                distance = minimum(distance, segmentDistance(sampleX, sampleY, s[0], s[1], s[2], s[3]));
            }
            if (distance <= 1.0f) ++hits;
        }
        surfaceBits[row * surfaceWidth + column] = (((hits * 255 + 32) / 64) << 24) |
            (((hits * GetRValue(color) + 32) / 64) << 16) |
            (((hits * GetGValue(color) + 32) / 64) << 8) | ((hits * GetBValue(color) + 32) / 64);
    }
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    AlphaBlend(destination, x, y, size, size, sourceDc, 0, 0, size, size, blend);
    GdiFlush();
}
extern "C" int testDrawing(void)
{
    DWORD before = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
    if (!initDrawing()) return 0;
    HDC target = CreateCompatibleDC(nullptr);
    BITMAPINFO info = {};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = 80; info.bmiHeader.biHeight = -64;
    info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32;
    void *memory;
    HBITMAP bitmap = CreateDIBSection(target, &info, DIB_RGB_COLORS, &memory, nullptr, 0);
    if (!target || !bitmap) {
        if (bitmap) DeleteObject(bitmap);
        if (target) DeleteDC(target);
        closeDrawing(); return 0;
    }
    HGDIOBJ old = SelectObject(target, bitmap);
    DWORD *pixels = static_cast<DWORD *>(memory);
    ZeroMemory(pixels, 80 * 64 * 4);
    smoothRoundRect(target, 0, 0, 60, 40, 10, RGB(255,255,255), RGB(30,90,210), 2);
    bool ok = pixels[0] == 0 && (pixels[20 * 80 + 30] >> 24) == 255;
    bool partial = false;
    for (int i = 0; i < 80 * 64; ++i) {
        unsigned int alpha = pixels[i] >> 24;
        if (alpha > 0 && alpha < 255) partial = true;
        if (((pixels[i] >> 16) & 255) > alpha || ((pixels[i] >> 8) & 255) > alpha ||
            (pixels[i] & 255) > alpha) ok = false;
    }
    ok = ok && partial;
    GdiFlush(); ZeroMemory(pixels, 80 * 64 * 4);
    smoothCircle(target, 0, 0, 18, RGB(255,255,255));
    for (int y = 0; y < 18; ++y) for (int x = 0; x < 18; ++x)
        if (pixels[y * 80 + x] != pixels[y * 80 + 17 - x] ||
            pixels[y * 80 + x] != pixels[(17 - y) * 80 + x]) ok = false;
    GdiFlush(); ZeroMemory(pixels, 80 * 64 * 4);
    smoothRoundRect(target, 0, 0, 76, 50, 8, RGB(45,45,45), RGB(70,70,70), 1);
    GdiFlush(); ZeroMemory(pixels, 80 * 64 * 4);
    smoothCircle(target, 0, 0, 18, RGB(255,255,255));
    if (pixels[9 * 80 + 9] != 0xffffffffu || pixels[30 * 80 + 30] != 0) ok = false;
    SelectObject(target, old); DeleteObject(bitmap); DeleteDC(target);
    closeDrawing();
    return ok && GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS) <= before;
}
