/* Settings UI. The background mapper does not initialize these resources. */
static BOOL compositedPaint;
static BOOL morePage;
static HPAINTBUFFER buttonBuffer;
static BOOL keyboardNavigation;
extern int initDrawing(void);
extern void closeDrawing(void);
extern void smoothRoundRect(HDC, int, int, int, int, int, COLORREF, COLORREF, int);
extern void smoothCircle(HDC, int, int, int, COLORREF);
extern void smoothArrow(HDC, int, int, int, COLORREF, int);
extern int testDrawing(void);

static void readPalette(void)
{
    DWORD light = 1, size = sizeof(light);
    RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"AppsUseLightTheme", RRF_RT_REG_DWORD, NULL, &light, &size);
    HIGHCONTRASTW contrast = {sizeof(contrast), 0, NULL};
    SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(contrast), &contrast, 0);
    highContrast = (contrast.dwFlags & HCF_HIGHCONTRASTON) != 0;
    darkTheme = themeChoice == 0 ? !light : themeChoice == 2;
    if (themePreview >= 0) darkTheme = themePreview != 0;
    baseColor = darkTheme ? RGB(32, 32, 32) : RGB(245, 245, 245);
    cardColor = darkTheme ? RGB(45, 45, 45) : RGB(255, 255, 255);
    borderColor = darkTheme ? RGB(68, 68, 68) : RGB(218, 221, 227);
    foregroundColor = darkTheme ? RGB(243, 243, 243) : RGB(30, 32, 36);
    mutedColor = darkTheme ? RGB(179, 179, 185) : RGB(105, 109, 119);
    accentColor = darkTheme ? RGB(126, 183, 255) : RGB(36, 92, 211);
    if (highContrast) {
        baseColor = cardColor = GetSysColor(COLOR_WINDOW);
        foregroundColor = GetSysColor(COLOR_WINDOWTEXT);
        mutedColor = foregroundColor;
        borderColor = foregroundColor;
        accentColor = GetSysColor(COLOR_HIGHLIGHT);
    }
    if (backgroundBrush) DeleteObject(backgroundBrush);
    if (whiteBrush) DeleteObject(whiteBrush);
    backgroundBrush = CreateSolidBrush(baseColor);
    whiteBrush = CreateSolidBrush(cardColor);
}
static void applyMaterial(void)
{
    BOOL dark = darkTheme;
    int roundCorners = 2;
    DwmSetWindowAttribute(window, 20, &dark, sizeof(dark));
    DwmSetWindowAttribute(window, 33, &roundCorners, sizeof(roundCorners));
    DWORD transparency = 1, size = sizeof(transparency);
    RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"EnableTransparency", RRF_RT_REG_DWORD, NULL, &transparency, &size);
    int material = highContrast || !transparency || solidPreview ? 1 : 2;
    HRESULT result = DwmSetWindowAttribute(window, 38, &material, sizeof(material));
    micaEnabled = SUCCEEDED(result) && material == 2;
    MARGINS margins = micaEnabled ? (MARGINS){-1, -1, -1, -1} : (MARGINS){0, 0, 0, 0};
    if (FAILED(DwmExtendFrameIntoClientArea(window, &margins))) micaEnabled = FALSE;
    if (textTheme) CloseThemeData(textTheme);
    textTheme = OpenThemeData(window, L"WINDOW");
    RedrawWindow(window, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_FRAME);
}
static HWND control(const WCHAR *kind, const WCHAR *text, DWORD style, int x, int y, int width, int height, int id)
{
    HWND child = CreateWindowExW(0, kind, text, WS_CHILD | WS_VISIBLE | style,
        px(x), px(y), px(width), px(height), window, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
    SendMessageW(child, WM_SETFONT, (WPARAM)bodyFont, TRUE);
    return child;
}
static void drawText(HDC dc, const WCHAR *text, int x, int y, int width, int height, HFONT font, COLORREF color)
{
    RECT rectangle = {px(x), px(y), px(x + width), px(y + height)};
    SelectObject(dc, font);
    SetTextColor(dc, color);
    SetBkMode(dc, TRANSPARENT);
    DWORD format = DT_LEFT | DT_WORDBREAK | DT_NOPREFIX;
    if (compositedPaint && textTheme) {
        DTTOPTS options = {0};
        options.dwSize = sizeof(options);
        options.dwFlags = DTT_COMPOSITED | DTT_TEXTCOLOR;
        options.crText = color;
        if (SUCCEEDED(DrawThemeTextEx(textTheme, dc, 0, 0, text, -1, format, &rectangle, &options))) return;
    }
    DrawTextW(dc, text, -1, &rectangle, format);
}
static void panel(HDC dc, int x, int y, int width, int height, COLORREF color, COLORREF border)
{
    smoothRoundRect(dc, px(x), px(y), px(width), px(height), px(10), color, border, 1);
}
static void paintButtonContent(DRAWITEMSTRUCT *item)
{
    HDC dc = item->hDC;
    RECT r = item->rcItem;
    BOOL disabled = (item->itemState & ODS_DISABLED) != 0;
    BOOL pressed = (item->itemState & ODS_SELECTED) != 0;
    BOOL onCard = item->CtlID == ID_CHINESE || item->CtlID == ID_ENGLISH || (item->CtlID >= ID_THEME_SYSTEM && item->CtlID <= ID_THEME_DARK) || item->CtlID == ID_UPDATE;
    FillRect(dc, &r, onCard ? whiteBrush : micaEnabled ? (HBRUSH)GetStockObject(BLACK_BRUSH) : backgroundBrush);
    if (buttonBuffer && (onCard || !micaEnabled)) BufferedPaintSetAlpha(buttonBuffer, NULL, 255);
    int width = r.right - r.left, height = r.bottom - r.top;
    if (item->CtlID == ID_STARTUP) {
        HBRUSH surface = CreateSolidBrush(cardColor);
        FillRect(dc, &r, surface); DeleteObject(surface);
        if (buttonBuffer) BufferedPaintSetAlpha(buttonBuffer, NULL, 255);
        COLORREF track = startupEnabled ? accentColor : (darkTheme ? RGB(94, 94, 99) : RGB(185, 190, 199));
        smoothRoundRect(dc, 1, (height - px(24)) / 2, width - 2, px(24), px(12), track, track, 0);
        int left = startupEnabled ? width - px(23) : px(5), top = (height - px(18)) / 2;
        smoothCircle(dc, left, top, px(18), RGB(255, 255, 255));
    } else {
        BOOL app = item->CtlID == ID_TARGET || item->CtlID == ID_CLAUDE;
        BOOL language = item->CtlID == ID_CHINESE || item->CtlID == ID_ENGLISH;
        BOOL theme = item->CtlID >= ID_THEME_SYSTEM && item->CtlID <= ID_THEME_DARK;
        BOOL selected = app ? ((item->CtlID == ID_CLAUDE) == selectedClaude) :
            (language && ((item->CtlID == ID_CHINESE) == chinese)) || (theme && (int)item->CtlID - ID_THEME_SYSTEM == themeChoice);
        BOOL primary = item->CtlID == ID_SAVE;
        COLORREF fillColor = primary ? accentColor : selected ? (darkTheme ? RGB(48, 66, 87) : RGB(232, 240, 254)) : cardColor;
        if (pressed && !primary) fillColor = darkTheme ? RGB(65, 68, 75) : RGB(224, 229, 238);
        if (disabled) fillColor = darkTheme ? RGB(43, 43, 45) : RGB(237, 238, 241);
        if (highContrast) fillColor = primary || selected ? GetSysColor(COLOR_HIGHLIGHT) : cardColor;
        COLORREF border = primary ? accentColor : selected && app ? accentColor : borderColor;
        smoothRoundRect(dc, 1, 1, width - 2, height - 2, px(app ? 10 : 7), fillColor, border, selected && app ? px(1) : 1);
        WCHAR text[48];
        GetWindowTextW(item->hwndItem, text, 48);
        if (item->CtlID == ID_MORE) lstrcpyW(text, L"•••");
        SelectObject(dc, app ? labelFont : bodyFont);
        SetBkMode(dc, TRANSPARENT);
        COLORREF textColor = disabled ? mutedColor : primary ? (darkTheme ? RGB(15, 32, 52) : RGB(255, 255, 255)) : foregroundColor;
        if (highContrast) textColor = disabled ? GetSysColor(COLOR_GRAYTEXT) :
            primary || selected ? GetSysColor(COLOR_HIGHLIGHTTEXT) : foregroundColor;
        SetTextColor(dc, textColor);
        if (item->CtlID == ID_BACK) {
            smoothArrow(dc, (width - px(18)) / 2, (height - px(18)) / 2, px(18), textColor, FALSE);
        } else if (item->CtlID == ID_PROJECT) {
            SIZE textSize;
            GetTextExtentPoint32W(dc, text, lstrlenW(text), &textSize);
            int left = (width - textSize.cx - px(26)) / 2;
            RECT label = {left, 0, left + textSize.cx, height};
            DrawTextW(dc, text, -1, &label, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            smoothArrow(dc, left + textSize.cx + px(8), (height - px(18)) / 2, px(18), textColor, TRUE);
        } else {
            DrawTextW(dc, text, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        }
    }
    if (keyboardNavigation && (item->itemState & ODS_FOCUS) && !(item->itemState & ODS_NOFOCUSRECT)) {
        smoothRoundRect(dc, r.left + px(5), r.top + px(5), width - px(10), height - px(10),
            px(5), CLR_INVALID, accentColor, 1);
    }
}
static void finishBuffer(HPAINTBUFFER buffer, int width, int height)
{
    RGBQUAD *pixels; int stride;
    if (SUCCEEDED(GetBufferedPaintBits(buffer, &pixels, &stride)))
        for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
            RGBQUAD *pixel = &pixels[y * stride + x];
            if (!pixel->rgbReserved && (pixel->rgbRed || pixel->rgbGreen || pixel->rgbBlue)) pixel->rgbReserved = 255;
        }
    EndBufferedPaint(buffer, TRUE);
}
static void paintButton(DRAWITEMSTRUCT *item)
{
    HDC dc;
    BP_PAINTPARAMS parameters = {0};
    parameters.cbSize = sizeof(parameters); parameters.dwFlags = BPPF_ERASE;
    HPAINTBUFFER buffer = BeginBufferedPaint(item->hDC, &item->rcItem, BPBF_TOPDOWNDIB, &parameters, &dc);
    DRAWITEMSTRUCT copy = *item;
    if (buffer) copy.hDC = dc;
    buttonBuffer = buffer;
    paintButtonContent(&copy);
    buttonBuffer = NULL;
    if (buffer) finishBuffer(buffer, item->rcItem.right, item->rcItem.bottom);
}
static void translateControls(void)
{
    SetWindowTextW(window, tr(TX_WINDOW));
    SetWindowTextW(startupButton, tr(TX_STARTUP));
    SetWindowTextW(GetDlgItem(window, ID_SAVE), tr(TX_SAVE));
    SetWindowTextW(GetDlgItem(window, ID_MORE), tr(TX_MORE));
    SetWindowTextW(GetDlgItem(window, ID_BACK), tr(TX_BACK));
    SetWindowTextW(GetDlgItem(window, ID_PROJECT), tr(TX_PROJECT));
    SetWindowTextW(GetDlgItem(window, ID_THEME_SYSTEM), tr(TX_SYSTEM));
    SetWindowTextW(GetDlgItem(window, ID_THEME_LIGHT), tr(TX_LIGHT));
    SetWindowTextW(GetDlgItem(window, ID_THEME_DARK), tr(TX_DARK));
    SetWindowTextW(GetDlgItem(window, ID_UPDATE), tr(updateState == 1 ? TX_CHECKING : TX_UPDATE));
    /* Leave the edit contents and unsaved target/startup choices intact. */
    setStatus(currentStatus, statusError);
    RedrawWindow(window, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
}
static void changeLanguage(BOOL useChinese)
{
    if (chinese == useChinese) return;
    chinese = useChinese;
    translateControls();
    if (!ensureConfigDirectory() || !saveLanguage(settingsPath, chinese)) setStatus(TX_LANGUAGE_FAILED, TRUE);
}
static void showPage(BOOL showMore)
{
    const int mainControls[] = {ID_TARGET, ID_CLAUDE, ID_SAVE, ID_MORE};
    const int moreControls[] = {ID_CHINESE, ID_ENGLISH, ID_BACK, ID_PROJECT, ID_STARTUP, ID_THEME_SYSTEM, ID_THEME_LIGHT, ID_THEME_DARK, ID_UPDATE};
    morePage = showMore;
    for (unsigned int i = 0; i < sizeof(mainControls) / sizeof(mainControls[0]); ++i)
        ShowWindow(GetDlgItem(window, mainControls[i]), showMore ? SW_HIDE : SW_SHOW);
    for (unsigned int i = 0; i < sizeof(moreControls) / sizeof(moreControls[0]); ++i)
        ShowWindow(GetDlgItem(window, moreControls[i]), showMore ? SW_SHOW : SW_HIDE);
    RECT size = {0, 0, px(500), px(showMore ? 540 : 246)};
    AdjustWindowRect(&size, (DWORD)GetWindowLongPtrW(window, GWL_STYLE), FALSE);
    SetWindowPos(window, NULL, 0, 0, size.right - size.left, size.bottom - size.top, SWP_NOMOVE | SWP_NOZORDER);
    SetFocus(GetDlgItem(window, showMore ? ID_BACK : ID_MORE));
    InvalidateRect(window, NULL, FALSE);
}
static void changeStartup(void)
{
    BOOL enable = !startupEnabled;
    if (enable && legacyRunning()) {
        MessageBoxW(window, tr(TX_LEGACY_STARTUP), tr(TX_WINDOW), MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (setStartup(STARTUP_KEY, enable) != ERROR_SUCCESS) {
        MessageBoxW(window, tr(TX_STARTUP_FAILED), tr(TX_WINDOW), MB_OK | MB_ICONWARNING);
        return;
    }
    startupEnabled = enable;
    InvalidateRect(startupButton, NULL, FALSE);
}
static void openPage(BOOL release)
{
    WCHAR application[MAX_PATH], command[MAX_PATH + 128];
    UINT count = GetWindowsDirectoryW(application, MAX_PATH);
    if (!count || count + 14 >= MAX_PATH) return;
    lstrcatW(application, L"\\explorer.exe");
    wsprintfW(command, L"\"%s\" https://github.com/Milorgs/CopilotKeyBinding%s", application,
        release ? L"/releases/latest" : L"");
    if (!createProcess(application, command)) MessageBoxW(window, tr(TX_LINK_FAILED), tr(TX_WINDOW), MB_OK | MB_ICONWARNING);
}
static void changeTheme(int choice)
{
    Settings settings = loadSettings(settingsPath);
    settings.theme = choice;
    if (!ensureConfigDirectory() || !saveSettings(settingsPath, &settings)) {
        MessageBoxW(window, tr(TX_THEME_FAILED), tr(TX_WINDOW), MB_OK | MB_ICONWARNING); return;
    }
    themeChoice = choice; themePreview = -1;
    readPalette(); applyMaterial();
}
static void checkUpdates(void)
{
    if (updateState == 1) return;
    updateState = 1;
    SetWindowTextW(GetDlgItem(window, ID_UPDATE), tr(TX_CHECKING));
    EnableWindow(GetDlgItem(window, ID_UPDATE), FALSE);
    InvalidateRect(window, NULL, FALSE);
    HANDLE thread = CreateThread(NULL, 0, updateThread, window, 0, NULL);
    if (thread) CloseHandle(thread);
    else PostMessageW(window, UPDATE_COMPLETE, 0, 0);
}
static void paintContent(HDC dc, const RECT *client)
{
    FillRect(dc, client, compositedPaint ? (HBRUSH)GetStockObject(BLACK_BRUSH) : backgroundBrush);
    if (morePage) {
        drawText(dc, tr(TX_MORE), 76, 20, 330, 36, titleFont, foregroundColor);
        panel(dc, 24, 90, 452, 64, cardColor, borderColor);
        drawText(dc, tr(TX_LANGUAGE), 40, 111, 180, 24, labelFont, foregroundColor);
        panel(dc, 24, 170, 452, 96, cardColor, borderColor);
        drawText(dc, tr(TX_THEME), 40, 185, 400, 24, labelFont, foregroundColor);
        panel(dc, 24, 282, 452, 74, cardColor, borderColor);
        drawText(dc, tr(TX_STARTUP), 40, 296, 348, 24, labelFont, foregroundColor);
        drawText(dc, tr(TX_STARTUP_HINT), 40, 325, 358, 20, smallFont, mutedColor);
        panel(dc, 24, 372, 452, 92, cardColor, borderColor);
        drawText(dc, tr(TX_VERSION), 40, 387, 260, 24, labelFont, foregroundColor);
        drawText(dc, APP_VERSION, 414, 387, 50, 24, bodyFont, mutedColor);
        TextId updateText = updateState == 1 ? TX_CHECKING : updateState == 2 ? TX_CURRENT :
            updateState == 3 ? TX_AVAILABLE : updateState == 4 ? TX_UPDATE_FAILED : TX_UPDATE_HINT;
        drawText(dc, tr(updateText), 40, 427, 285, 22, smallFont, updateState == 4 ? accentColor : mutedColor);
        return;
    }
    drawText(dc, tr(TX_TITLE), 24, 20, 310, 36, titleFont, foregroundColor);
    drawText(dc, tr(TX_SUBTITLE), 24, 62, 430, 23, smallFont, mutedColor);
}
static LRESULT CALLBACK windowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CREATE: {
        window = hwnd;
        Settings settings = loadSettings(settingsPath);
        chinese = settings.chinese; selectedClaude = settings.claude; themeChoice = settings.theme;
        readPalette();
        StartupState startup;
        startupEnabled = snapshotStartup(STARTUP_KEY, &startup) == ERROR_SUCCESS && startup.exists;
        targetBox = control(L"BUTTON", L"Codex", BS_OWNERDRAW | WS_TABSTOP, 24, 104, 218, 54, ID_TARGET);
        claudeButton = control(L"BUTTON", L"Claude", BS_OWNERDRAW | WS_TABSTOP, 258, 104, 218, 54, ID_CLAUDE);
        control(L"BUTTON", tr(TX_MORE), BS_OWNERDRAW | WS_TABSTOP, 440, 24, 36, 30, ID_MORE);
        control(L"BUTTON", tr(TX_BACK), BS_OWNERDRAW | WS_TABSTOP, 24, 24, 36, 30, ID_BACK);
        control(L"BUTTON", L"中文", BS_OWNERDRAW | WS_TABSTOP, 240, 106, 98, 32, ID_CHINESE);
        control(L"BUTTON", L"English", BS_OWNERDRAW | WS_TABSTOP, 350, 106, 110, 32, ID_ENGLISH);
        control(L"BUTTON", tr(TX_PROJECT), BS_OWNERDRAW | WS_TABSTOP, 24, 480, 452, 36, ID_PROJECT);
        startupButton = control(L"BUTTON", tr(TX_STARTUP), BS_OWNERDRAW | WS_TABSTOP, 414, 298, 46, 28, ID_STARTUP);
        control(L"BUTTON", tr(TX_SAVE), BS_OWNERDRAW | WS_TABSTOP, 24, 186, 452, 36, ID_SAVE);
        control(L"BUTTON", tr(TX_SYSTEM), BS_OWNERDRAW | WS_TABSTOP, 40, 222, 132, 30, ID_THEME_SYSTEM);
        control(L"BUTTON", tr(TX_LIGHT), BS_OWNERDRAW | WS_TABSTOP, 184, 222, 132, 30, ID_THEME_LIGHT);
        control(L"BUTTON", tr(TX_DARK), BS_OWNERDRAW | WS_TABSTOP, 328, 222, 132, 30, ID_THEME_DARK);
        control(L"BUTTON", tr(TX_UPDATE), BS_OWNERDRAW | WS_TABSTOP, 330, 418, 130, 30, ID_UPDATE);
        SetWindowTextW(window, tr(TX_WINDOW));
        showPage(FALSE);
        return 0;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_MORE: showPage(TRUE); return 0;
        case ID_BACK: showPage(FALSE); return 0;
        case ID_PROJECT: openPage(FALSE); return 0;
        case ID_TARGET:
        case ID_CLAUDE:
            selectedClaude = LOWORD(wParam) == ID_CLAUDE;
            InvalidateRect(targetBox, NULL, FALSE); InvalidateRect(claudeButton, NULL, FALSE);
            return 0;
        case ID_CHINESE: changeLanguage(TRUE); return 0;
        case ID_ENGLISH: changeLanguage(FALSE); return 0;
        case ID_THEME_SYSTEM: case ID_THEME_LIGHT: case ID_THEME_DARK:
            changeTheme(LOWORD(wParam) - ID_THEME_SYSTEM); return 0;
        case ID_UPDATE: checkUpdates(); return 0;
        case ID_STARTUP: changeStartup(); return 0;
        case ID_SAVE:
            if (!applySettings() || !ensureMapper()) MessageBoxW(window, tr(currentStatus), tr(TX_WINDOW), MB_OK | MB_ICONWARNING);
            else { SetWindowTextW(GetDlgItem(window, ID_SAVE), tr(TX_SAVE_DONE)); SetTimer(window, 2, 1600, NULL); }
            return 0;
        }
        break;
    case UPDATE_COMPLETE:
        updateState = wParam == 2 ? 3 : wParam == 1 ? 2 : 4;
        EnableWindow(GetDlgItem(window, ID_UPDATE), TRUE);
        SetWindowTextW(GetDlgItem(window, ID_UPDATE), tr(TX_UPDATE));
        InvalidateRect(window, NULL, FALSE);
        if (wParam == 2 && MessageBoxW(window, tr(TX_DOWNLOAD), tr(TX_AVAILABLE), MB_YESNO | MB_ICONINFORMATION) == IDYES) openPage(TRUE);
        return 0;
    case WM_TIMER:
        if (wParam == 2) { KillTimer(window, 2); SetWindowTextW(GetDlgItem(window, ID_SAVE), tr(TX_SAVE)); return 0; }
        return 0;
    case WM_THEMECHANGED:
    case WM_SETTINGCHANGE:
        if (targetBox) { readPalette(); applyMaterial(); }
        return 0;
    case WM_ERASEBKGND: return 1;
    case WM_DRAWITEM: paintButton((DRAWITEMSTRUCT *)lParam); return TRUE;
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC:
        SetTextColor((HDC)wParam, IsWindowEnabled((HWND)lParam) ? foregroundColor : mutedColor);
        SetBkColor((HDC)wParam, cardColor);
        return (LRESULT)whiteBrush;
    case WM_PAINT: {
        PAINTSTRUCT paint;
        HDC screen = BeginPaint(hwnd, &paint), dc;
        RECT client;
        GetClientRect(hwnd, &client);
        BP_PAINTPARAMS parameters = {0};
        parameters.cbSize = sizeof(parameters); parameters.dwFlags = BPPF_ERASE;
        HPAINTBUFFER buffer = BeginBufferedPaint(screen, &client, BPBF_TOPDOWNDIB, &parameters, &dc);
        if (!buffer) dc = screen;
        compositedPaint = buffer && micaEnabled;
        paintContent(dc, &client);
        if (buffer) {
            if (!micaEnabled) BufferedPaintSetAlpha(buffer, NULL, 255);
            finishBuffer(buffer, client.right, client.bottom);
        }
        compositedPaint = FALSE;
        EndPaint(hwnd, &paint);
        return 0;
    }
    case WM_CLOSE: DestroyWindow(hwnd); return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}
static int showSettings(HINSTANCE instance, int show)
{
    HANDLE mutex = CreateMutexW(NULL, FALSE, UI_MUTEX);
    if (!mutex) return 1;
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND existing = FindWindowW(APP_CLASS, NULL);
        if (existing) { ShowWindow(existing, SW_RESTORE); SetForegroundWindow(existing); }
        CloseHandle(mutex); return 0;
    }
    SetProcessDPIAware();
    HDC screen = GetDC(NULL);
    dpi = GetDeviceCaps(screen, LOGPIXELSX);
    ReleaseDC(NULL, screen);
    titleFont = CreateFontW(-px(25), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    bodyFont = CreateFontW(-px(14), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    smallFont = CreateFontW(-px(13), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    labelFont = CreateFontW(-px(15), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    if (!initDrawing()) { CloseHandle(mutex); return 1; }
    readPalette(); BufferedPaintInit();
    WNDCLASSW kind = {0};
    kind.lpfnWndProc = windowProcedure; kind.hInstance = instance;
    kind.hCursor = LoadCursorW(NULL, IDC_ARROW); kind.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(1));
    kind.lpszClassName = APP_CLASS;
    RegisterClassW(&kind);
    RECT rectangle = {0, 0, px(500), px(246)};
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&rectangle, style, FALSE);
    window = CreateWindowExW(0, APP_CLASS, L"Copilot Key Binding 2.2", style,
        CW_USEDEFAULT, CW_USEDEFAULT, rectangle.right - rectangle.left, rectangle.bottom - rectangle.top,
        NULL, NULL, instance, NULL);
    if (!window) { CloseHandle(mutex); return 1; }
    applyMaterial();
    ShowWindow(window, show); UpdateWindow(window);
    MSG message;
    while (GetMessageW(&message, NULL, 0, 0) > 0) {
        if (message.message == WM_KEYDOWN && message.wParam == VK_TAB) {
            keyboardNavigation = TRUE;
            RedrawWindow(window, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
        } else if (message.message == WM_LBUTTONDOWN || message.message == WM_RBUTTONDOWN ||
            message.message == WM_NCLBUTTONDOWN || message.message == WM_MOUSEWHEEL) {
            keyboardNavigation = FALSE;
            RedrawWindow(window, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
        }
        if (!IsDialogMessageW(window, &message)) { TranslateMessage(&message); DispatchMessageW(&message); }
    }
    if (textTheme) CloseThemeData(textTheme);
    BufferedPaintUnInit(); closeDrawing();
    DeleteObject(titleFont); DeleteObject(bodyFont); DeleteObject(smallFont); DeleteObject(labelFont);
    DeleteObject(backgroundBrush); DeleteObject(whiteBrush);
    CloseHandle(mutex);
    return 0;
}
