#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <commdlg.h>
#include <stdio.h>

#define APP_CLASS L"CopilotKeyBindingV2.Settings"
#define SHARED_MUTEX L"Local\\CodexCopilotKeyBinding"
#define V2_MUTEX L"Local\\CopilotKeyBindingV2.Worker"
#define UI_MUTEX L"Local\\CopilotKeyBindingV2.Settings"
#define STARTUP_KEY L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define STARTUP_VALUE L"CopilotKeyBindingV2"
#define LAUNCH_TARGET (WM_APP + 1)
#define ID_TARGET 101
#define ID_PATH 102
#define ID_BROWSE 103
#define ID_STARTUP 104
#define ID_SAVE 105
#define ID_START 106
#define ID_OPEN 107
#define ID_STOP 108
#define STOP_EVENT L"Local\\CopilotKeyBindingV2.Stop"

typedef struct { BOOL claude; WCHAR executable[MAX_PATH]; } Settings;
typedef struct { BOOL exists; DWORD type, bytes; BYTE data[2048]; } StartupState;
static const WCHAR codexTarget[] = L"shell:AppsFolder\\OpenAI.Codex_2p2nqsd0c76g0!App";
static const WCHAR claudeTarget[] = L"shell:AppsFolder\\Claude_pzs8sxrjxfjjc!Claude";
static WCHAR executablePath[MAX_PATH], settingsPath[MAX_PATH], configDirectory[MAX_PATH];
static HHOOK hook;
static DWORD workerThread;
static BOOL captured, startupEnabled;
static HWND window, targetBox, pathBox, browseButton, startupButton, startButton, stopButton;
static HFONT titleFont, bodyFont, smallFont, labelFont;
static HBRUSH backgroundBrush, whiteBrush;
static int dpi = 96;
static WCHAR statusText[256];
static BOOL statusError;
static BOOL expectedWorker;
static int transitionChecks;

static int px(int value) { return MulDiv(value, dpi, 96); }
static BOOL mutexExists(const WCHAR *name)
{
    HANDLE mutex = OpenMutexW(SYNCHRONIZE, FALSE, name);
    if (!mutex) return FALSE;
    CloseHandle(mutex);
    return TRUE;
}
static BOOL legacyRunning(void) { return mutexExists(SHARED_MUTEX) && !mutexExists(V2_MUTEX); }

static BOOL preparePaths(void)
{
    DWORD count = GetModuleFileNameW(NULL, executablePath, MAX_PATH);
    if (!count || count >= MAX_PATH) return FALSE;
    count = GetEnvironmentVariableW(L"LOCALAPPDATA", configDirectory, MAX_PATH);
    if (!count || count + 44 >= MAX_PATH) return FALSE;
    lstrcatW(configDirectory, L"\\CopilotKeyBinding\\v2");
    lstrcpyW(settingsPath, configDirectory);
    lstrcatW(settingsPath, L"\\settings.ini");
    return TRUE;
}
static BOOL ensureConfigDirectory(void)
{
    WCHAR parent[MAX_PATH];
    lstrcpyW(parent, configDirectory);
    WCHAR *last = wcsrchr(parent, L'\\');
    if (!last) return FALSE;
    *last = L'\0';
    if (!CreateDirectoryW(parent, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) return FALSE;
    return CreateDirectoryW(configDirectory, NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}
static Settings loadSettings(const WCHAR *path)
{
    Settings settings = {0};
    WCHAR name[24];
    GetPrivateProfileStringW(L"Mapping", L"Target", L"Codex", name, 24, path);
    settings.claude = lstrcmpiW(name, L"Claude") == 0;
    GetPrivateProfileStringW(L"Mapping", L"ClaudeExecutable", L"", settings.executable, MAX_PATH, path);
    return settings;
}
static BOOL saveSettings(const WCHAR *path, const Settings *settings)
{
    WCHAR temporary[MAX_PATH];
    if (lstrlenW(path) + 5 >= MAX_PATH) return FALSE;
    lstrcpyW(temporary, path);
    lstrcatW(temporary, L".tmp");
    HANDLE file = CreateFileW(temporary, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return FALSE;
    const WORD unicodeHeader = 0xFEFF;
    DWORD written;
    BOOL headerSaved = WriteFile(file, &unicodeHeader, sizeof(unicodeHeader), &written, NULL) && written == sizeof(unicodeHeader);
    CloseHandle(file);
    if (!headerSaved) { DeleteFileW(temporary); return FALSE; }
    if (!WritePrivateProfileStringW(L"Mapping", L"Target", settings->claude ? L"Claude" : L"Codex", temporary) ||
        !WritePrivateProfileStringW(L"Mapping", L"ClaudeExecutable", settings->executable, temporary)) {
        DeleteFileW(temporary);
        return FALSE;
    }
    WritePrivateProfileStringW(NULL, NULL, NULL, temporary);
    if (!MoveFileExW(temporary, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(temporary);
        return FALSE;
    }
    WritePrivateProfileStringW(NULL, NULL, NULL, path);
    return TRUE;
}
static LONG snapshotStartup(const WCHAR *keyPath, StartupState *state)
{
    HKEY key;
    ZeroMemory(state, sizeof(*state));
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, keyPath, 0, KEY_QUERY_VALUE, &key);
    if (result == ERROR_FILE_NOT_FOUND) return ERROR_SUCCESS;
    if (result != ERROR_SUCCESS) return result;
    state->bytes = sizeof(state->data);
    result = RegQueryValueExW(key, STARTUP_VALUE, NULL, &state->type, state->data, &state->bytes);
    RegCloseKey(key);
    if (result == ERROR_FILE_NOT_FOUND) return ERROR_SUCCESS;
    state->exists = result == ERROR_SUCCESS;
    return result;
}
static LONG setStartup(const WCHAR *keyPath, BOOL enabled)
{
    HKEY key;
    LONG result = RegCreateKeyExW(HKEY_CURRENT_USER, keyPath, 0, NULL, 0, KEY_SET_VALUE, NULL, &key, NULL);
    if (result != ERROR_SUCCESS) return result;
    if (enabled) {
        WCHAR command[MAX_PATH + 32];
        wsprintfW(command, L"\"%s\" --background", executablePath);
        result = RegSetValueExW(key, STARTUP_VALUE, 0, REG_SZ, (const BYTE *)command,
            ((DWORD)lstrlenW(command) + 1) * sizeof(WCHAR));
    } else {
        result = RegDeleteValueW(key, STARTUP_VALUE);
        if (result == ERROR_FILE_NOT_FOUND) result = ERROR_SUCCESS;
    }
    RegCloseKey(key);
    return result;
}
static LONG restoreStartup(const WCHAR *keyPath, const StartupState *state)
{
    if (!state->exists) return setStartup(keyPath, FALSE);
    HKEY key;
    LONG result = RegCreateKeyExW(HKEY_CURRENT_USER, keyPath, 0, NULL, 0, KEY_SET_VALUE, NULL, &key, NULL);
    if (result != ERROR_SUCCESS) return result;
    result = RegSetValueExW(key, STARTUP_VALUE, 0, state->type, state->data, state->bytes);
    RegCloseKey(key);
    return result;
}
static BOOL createProcess(const WCHAR *application, WCHAR *command)
{
    STARTUPINFOW startup = {0};
    PROCESS_INFORMATION process = {0};
    startup.cb = sizeof(startup);
    if (!CreateProcessW(application, command, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &process)) return FALSE;
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return TRUE;
}
static BOOL launch(const Settings *settings)
{
    WCHAR application[MAX_PATH], command[MAX_PATH + 160];
    if (settings->claude && settings->executable[0]) {
        DWORD attributes = GetFileAttributesW(settings->executable);
        if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
            SetLastError(ERROR_FILE_NOT_FOUND);
            return FALSE;
        }
        lstrcpyW(application, settings->executable);
        wsprintfW(command, L"\"%s\"", application);
    } else {
        UINT count = GetWindowsDirectoryW(application, MAX_PATH);
        if (!count || count + 14 >= MAX_PATH) return FALSE;
        lstrcatW(application, L"\\explorer.exe");
        wsprintfW(command, L"\"%s\" %s", application, settings->claude ? claudeTarget : codexTarget);
    }
    return createProcess(application, command);
}
static void workerLog(const char *event)
{
    if (!ensureConfigDirectory()) return;
    WCHAR path[MAX_PATH];
    lstrcpyW(path, configDirectory);
    lstrcatW(path, L"\\status.log");
    HANDLE file = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return;
    char line[160];
    SYSTEMTIME time;
    DWORD written;
    GetLocalTime(&time);
    int count = wsprintfA(line, "%04u-%02u-%02uT%02u:%02u:%02u %s\r\n", time.wYear,
        time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, event);
    WriteFile(file, line, (DWORD)count, &written, NULL);
    CloseHandle(file);
}
static LRESULT CALLBACK onKey(int code, WPARAM message, LPARAM data)
{
    if (code == HC_ACTION && ((const KBDLLHOOKSTRUCT *)data)->vkCode == 0x86) {
        BOOL down = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
        BOOL up = message == WM_KEYUP || message == WM_SYSKEYUP;
        BOOL win = (GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000);
        BOOL shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        if (down && (captured || (win && shift))) { captured = TRUE; return 1; }
        if (up && captured) {
            captured = FALSE;
            keybd_event(0xE8, 0, 0, 0);
            keybd_event(0xE8, 0, KEYEVENTF_KEYUP, 0);
            PostThreadMessageW(workerThread, LAUNCH_TARGET, 0, 0);
            return 1;
        }
    }
    return CallNextHookEx(hook, code, message, data);
}
static int runWorker(HINSTANCE instance)
{
    HANDLE shared = CreateMutexW(NULL, FALSE, SHARED_MUTEX);
    if (!shared) return 1;
    if (GetLastError() == ERROR_ALREADY_EXISTS) { CloseHandle(shared); return 2; }
    HANDLE marker = CreateMutexW(NULL, FALSE, V2_MUTEX);
    HANDLE stop = CreateEventW(NULL, TRUE, FALSE, STOP_EVENT);
    if (!marker || !stop) {
        if (marker) CloseHandle(marker);
        if (stop) CloseHandle(stop);
        CloseHandle(shared);
        return 1;
    }
    ResetEvent(stop);
    workerThread = GetCurrentThreadId();
    MSG message;
    PeekMessageW(&message, NULL, WM_USER, WM_USER, PM_NOREMOVE);
    hook = SetWindowsHookExW(WH_KEYBOARD_LL, onKey, instance, 0);
    if (!hook) {
        workerLog("HOOK_FAILED_V2");
        CloseHandle(stop); CloseHandle(marker); CloseHandle(shared);
        return 1;
    }
    workerLog("READY_V2");
    int result = 0;
    BOOL running = TRUE;
    while (running) {
        DWORD wait = MsgWaitForMultipleObjects(1, &stop, FALSE, INFINITE, QS_ALLINPUT);
        if (wait == WAIT_OBJECT_0) break;
        if (wait == WAIT_FAILED) { result = 1; break; }
        while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) { running = FALSE; break; }
            if (message.message == LAUNCH_TARGET) {
                Settings settings = loadSettings(settingsPath);
                if (!launch(&settings)) workerLog("LAUNCH_ERROR_V2");
            } else { TranslateMessage(&message); DispatchMessageW(&message); }
        }
    }
    UnhookWindowsHookEx(hook);
    CloseHandle(stop); CloseHandle(marker); CloseHandle(shared);
    return result;
}

static void setStatus(const WCHAR *text, BOOL error)
{
    lstrcpynW(statusText, text, 256);
    statusError = error;
    InvalidateRect(window, NULL, FALSE);
}
static void refreshStatus(void)
{
    BOOL legacy = legacyRunning(), active = mutexExists(V2_MUTEX);
    EnableWindow(startButton, !legacy && !active);
    EnableWindow(stopButton, active);
    if (legacy) setStatus(L"Version 1.0 is running. This window only configures 2.0; the current mapping stays active.", FALSE);
    else if (active) setStatus(L"Version 2.0 mapping is running. Saved target changes apply on the next key press.", FALSE);
    else setStatus(L"Version 2.0 mapping is stopped. Save your settings, then start mapping when ready.", FALSE);
}
static Settings selectedSettings(void)
{
    Settings settings = {0};
    settings.claude = SendMessageW(targetBox, CB_GETCURSEL, 0, 0) == 1;
    GetWindowTextW(pathBox, settings.executable, MAX_PATH);
    return settings;
}
static void refreshTarget(void)
{
    BOOL claude = SendMessageW(targetBox, CB_GETCURSEL, 0, 0) == 1;
    EnableWindow(pathBox, claude);
    EnableWindow(browseButton, claude);
}
static BOOL applySettings(void)
{
    Settings settings = selectedSettings();
    if (settings.claude && settings.executable[0]) {
        DWORD attributes = GetFileAttributesW(settings.executable);
        int length = lstrlenW(settings.executable);
        if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY) ||
            length < 4 || lstrcmpiW(settings.executable + length - 4, L".exe") != 0) {
            setStatus(L"Choose an existing Claude .exe, or leave the path blank for the packaged app.", TRUE);
            return FALSE;
        }
    }
    if (startupEnabled && legacyRunning()) {
        setStatus(L"Keep 1.0 as your startup version for now. Stop it and remove its startup shortcut before enabling 2.0 at sign-in.", TRUE);
        return FALSE;
    }
    StartupState before;
    if (snapshotStartup(STARTUP_KEY, &before) != ERROR_SUCCESS || !ensureConfigDirectory()) {
        setStatus(L"Settings could not be saved. Check access to your user settings folder and startup entry.", TRUE);
        return FALSE;
    }
    if (setStartup(STARTUP_KEY, startupEnabled) != ERROR_SUCCESS) {
        setStatus(L"The startup setting could not be changed. No target settings were saved.", TRUE);
        return FALSE;
    }
    if (!saveSettings(settingsPath, &settings)) {
        LONG rollback = restoreStartup(STARTUP_KEY, &before);
        setStatus(rollback == ERROR_SUCCESS ? L"The target could not be saved. The previous startup setting was restored." :
            L"Saving failed and the startup setting could not be restored. Check Windows Startup Apps.", TRUE);
        return FALSE;
    }
    setStatus(legacyRunning() ? L"2.0 settings saved. Version 1.0 is still running and has not been changed." :
        L"Settings saved. Closing this window does not stop the background mapper.", FALSE);
    return TRUE;
}
static void browseExecutable(void)
{
    WCHAR path[MAX_PATH] = L"";
    OPENFILENAMEW dialog = {0};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = window;
    dialog.lpstrFilter = L"Applications (*.exe)\0*.exe\0\0";
    dialog.lpstrFile = path;
    dialog.nMaxFile = MAX_PATH;
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    dialog.lpstrTitle = L"Choose Claude executable";
    if (GetOpenFileNameW(&dialog)) SetWindowTextW(pathBox, path);
}
static HWND control(const WCHAR *kind, const WCHAR *text, DWORD style, int x, int y, int width, int height, int id)
{
    HWND child = CreateWindowExW(kind[0] == L'E' ? WS_EX_CLIENTEDGE : 0, kind, text,
        WS_CHILD | WS_VISIBLE | style, px(x), px(y), px(width), px(height), window,
        (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
    SendMessageW(child, WM_SETFONT, (WPARAM)bodyFont, TRUE);
    return child;
}
static void drawText(HDC dc, const WCHAR *text, int x, int y, int width, int height, HFONT font, COLORREF color)
{
    RECT rectangle = {px(x), px(y), px(x + width), px(y + height)};
    SelectObject(dc, font);
    SetTextColor(dc, color);
    SetBkMode(dc, TRANSPARENT);
    DrawTextW(dc, text, -1, &rectangle, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);
}
static void paintButton(DRAWITEMSTRUCT *item)
{
    HDC dc = item->hDC;
    RECT r = item->rcItem;
    BOOL disabled = (item->itemState & ODS_DISABLED) != 0;
    BOOL pressed = (item->itemState & ODS_SELECTED) != 0;
    if (item->CtlID == ID_STARTUP) {
        FillRect(dc, &r, backgroundBrush);
        int w = r.right - r.left, h = r.bottom - r.top;
        HBRUSH fill = CreateSolidBrush(startupEnabled ? RGB(41, 97, 225) : RGB(192, 199, 211));
        HGDIOBJ oldBrush = SelectObject(dc, fill), oldPen = SelectObject(dc, GetStockObject(NULL_PEN));
        RoundRect(dc, 2, (h - px(24)) / 2, w - 2, (h + px(24)) / 2, px(24), px(24));
        SelectObject(dc, whiteBrush);
        int left = startupEnabled ? w - px(23) : px(5), top = (h - px(18)) / 2;
        Ellipse(dc, left, top, left + px(18), top + px(18));
        SelectObject(dc, oldBrush); SelectObject(dc, oldPen); DeleteObject(fill);
    } else {
        BOOL primary = item->CtlID == ID_SAVE;
        COLORREF color = disabled ? RGB(232, 235, 240) : primary ? (pressed ? RGB(25, 65, 160) : RGB(41, 97, 225)) :
            (pressed ? RGB(224, 228, 236) : RGB(255, 255, 255));
        HBRUSH fill = CreateSolidBrush(color);
        HPEN border = CreatePen(PS_SOLID, 1, primary ? color : RGB(210, 216, 226));
        HGDIOBJ oldBrush = SelectObject(dc, fill), oldPen = SelectObject(dc, border);
        RoundRect(dc, r.left, r.top, r.right, r.bottom, px(10), px(10));
        WCHAR text[48];
        GetWindowTextW(item->hwndItem, text, 48);
        SelectObject(dc, bodyFont);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, disabled ? RGB(150, 158, 173) : primary ? RGB(255, 255, 255) : RGB(39, 49, 65));
        DrawTextW(dc, text, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        SelectObject(dc, oldBrush); SelectObject(dc, oldPen); DeleteObject(fill); DeleteObject(border);
    }
    if (item->itemState & ODS_FOCUS) { InflateRect(&r, -px(4), -px(4)); DrawFocusRect(dc, &r); }
}
static LRESULT CALLBACK windowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CREATE: {
        window = hwnd;
        Settings settings = loadSettings(settingsPath);
        StartupState startup;
        startupEnabled = snapshotStartup(STARTUP_KEY, &startup) == ERROR_SUCCESS && startup.exists;
        targetBox = control(L"COMBOBOX", L"Open application", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 24, 112, 428, 160, ID_TARGET);
        SendMessageW(targetBox, CB_ADDSTRING, 0, (LPARAM)L"Codex");
        SendMessageW(targetBox, CB_ADDSTRING, 0, (LPARAM)L"Claude");
        SendMessageW(targetBox, CB_SETCURSEL, settings.claude ? 1 : 0, 0);
        SendMessageW(targetBox, CB_SETITEMHEIGHT, (WPARAM)-1, px(24));
        pathBox = control(L"EDIT", settings.executable, ES_AUTOHSCROLL | WS_TABSTOP, 24, 172, 338, 30, ID_PATH);
        SendMessageW(pathBox, EM_SETLIMITTEXT, MAX_PATH - 1, 0);
        browseButton = control(L"BUTTON", L"Browse", BS_OWNERDRAW | WS_TABSTOP, 372, 172, 80, 30, ID_BROWSE);
        startupButton = control(L"BUTTON", L"Run at sign-in", BS_OWNERDRAW | WS_TABSTOP, 404, 246, 48, 30, ID_STARTUP);
        control(L"BUTTON", L"Open app", BS_OWNERDRAW | WS_TABSTOP, 24, 388, 90, 34, ID_OPEN);
        stopButton = control(L"BUTTON", L"Stop", BS_OWNERDRAW | WS_TABSTOP, 124, 388, 62, 34, ID_STOP);
        startButton = control(L"BUTTON", L"Start mapping", BS_OWNERDRAW | WS_TABSTOP, 196, 388, 126, 34, ID_START);
        control(L"BUTTON", L"Save", BS_OWNERDRAW | WS_TABSTOP, 332, 388, 120, 34, ID_SAVE);
        refreshTarget(); refreshStatus();
        return 0;
    }
    case WM_ACTIVATE:
        if (LOWORD(wParam) != WA_INACTIVE && targetBox) refreshStatus();
        return 0;
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_TARGET && HIWORD(wParam) == CBN_SELCHANGE) { refreshTarget(); return 0; }
        switch (LOWORD(wParam)) {
        case ID_BROWSE: browseExecutable(); return 0;
        case ID_STARTUP: startupEnabled = !startupEnabled; InvalidateRect(startupButton, NULL, FALSE); return 0;
        case ID_SAVE: applySettings(); return 0;
        case ID_OPEN: {
            Settings settings = selectedSettings();
            BOOL launched = launch(&settings);
            setStatus(launched ? L"Launch request sent. Check that the selected app opened." :
                L"The app could not be launched. Check the executable path or app installation.", !launched);
            return 0;
        }
        case ID_START: {
            if (legacyRunning()) { refreshStatus(); return 0; }
            if (!applySettings()) return 0;
            WCHAR command[MAX_PATH + 32];
            wsprintfW(command, L"\"%s\" --background", executablePath);
            if (!createProcess(executablePath, command)) setStatus(L"The background mapper could not start.", TRUE);
            else {
                EnableWindow(startButton, FALSE);
                expectedWorker = TRUE; transitionChecks = 10;
                SetTimer(window, 1, 150, NULL);
                setStatus(L"Start requested. Version 2.0 will remain in the background after this window closes.", FALSE);
            }
            return 0;
        }
        case ID_STOP: {
            HANDLE event = OpenEventW(EVENT_MODIFY_STATE, FALSE, STOP_EVENT);
            if (event) {
                SetEvent(event); CloseHandle(event);
                expectedWorker = FALSE; transitionChecks = 10;
                SetTimer(window, 1, 150, NULL);
                setStatus(L"Stop requested for version 2.0. The startup setting is unchanged.", FALSE);
            }
            else refreshStatus();
            return 0;
        }
        }
        break;
    case WM_TIMER:
        if (wParam == 1 && (mutexExists(V2_MUTEX) == expectedWorker || --transitionChecks <= 0)) {
            KillTimer(window, 1);
            refreshStatus();
        }
        return 0;
    case WM_DRAWITEM: paintButton((DRAWITEMSTRUCT *)lParam); return TRUE;
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLOREDIT:
        SetBkColor((HDC)wParam, RGB(255, 255, 255));
        return (LRESULT)whiteBrush;
    case WM_PAINT: {
        PAINTSTRUCT paint;
        HDC dc = BeginPaint(hwnd, &paint);
        RECT client;
        GetClientRect(hwnd, &client);
        FillRect(dc, &client, backgroundBrush);
        drawText(dc, L"Copilot key", 24, 22, 350, 36, titleFont, RGB(28, 36, 50));
        drawText(dc, L"Settings  /  Version 2.0", 24, 60, 400, 22, smallFont, RGB(105, 117, 136));
        drawText(dc, L"Open when pressed", 24, 90, 400, 22, labelFont, RGB(47, 59, 78));
        drawText(dc, L"Claude executable (optional)", 24, 150, 400, 22, smallFont, RGB(91, 104, 123));
        drawText(dc, L"Leave blank to use the packaged Claude app.", 24, 209, 428, 22, smallFont, RGB(119, 130, 148));
        drawText(dc, L"Run at sign-in", 24, 245, 360, 24, labelFont, RGB(47, 59, 78));
        drawText(dc, L"Start the 2.0 mapper in the background", 24, 270, 370, 21, smallFont, RGB(105, 117, 136));
        RECT card = {px(24), px(306), px(452), px(371)};
        HBRUSH fill = CreateSolidBrush(statusError ? RGB(255, 237, 237) : RGB(232, 238, 249));
        HGDIOBJ oldBrush = SelectObject(dc, fill), oldPen = SelectObject(dc, GetStockObject(NULL_PEN));
        RoundRect(dc, card.left, card.top, card.right, card.bottom, px(12), px(12));
        SelectObject(dc, oldBrush); SelectObject(dc, oldPen); DeleteObject(fill);
        drawText(dc, statusText, 38, 316, 400, 49, smallFont, statusError ? RGB(163, 42, 47) : RGB(65, 86, 124));
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
        CloseHandle(mutex);
        return 0;
    }
    SetProcessDPIAware();
    HDC screen = GetDC(NULL);
    dpi = GetDeviceCaps(screen, LOGPIXELSX);
    ReleaseDC(NULL, screen);
    titleFont = CreateFontW(-px(27), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    bodyFont = CreateFontW(-px(14), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    smallFont = CreateFontW(-px(13), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    labelFont = CreateFontW(-px(14), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    backgroundBrush = CreateSolidBrush(RGB(246, 248, 252));
    whiteBrush = CreateSolidBrush(RGB(255, 255, 255));
    WNDCLASSW kind = {0};
    kind.lpfnWndProc = windowProcedure;
    kind.hInstance = instance;
    kind.hCursor = LoadCursorW(NULL, IDC_ARROW);
    kind.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    kind.hbrBackground = backgroundBrush;
    kind.lpszClassName = APP_CLASS;
    RegisterClassW(&kind);
    RECT rectangle = {0, 0, px(476), px(444)};
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&rectangle, style, FALSE);
    window = CreateWindowExW(0, APP_CLASS, L"Copilot Key Binding 2.0", style,
        CW_USEDEFAULT, CW_USEDEFAULT, rectangle.right - rectangle.left, rectangle.bottom - rectangle.top,
        NULL, NULL, instance, NULL);
    if (!window) { CloseHandle(mutex); return 1; }
    ShowWindow(window, show); UpdateWindow(window);
    MSG message;
    while (GetMessageW(&message, NULL, 0, 0) > 0) {
        if (!IsDialogMessageW(window, &message)) { TranslateMessage(&message); DispatchMessageW(&message); }
    }
    DeleteObject(titleFont); DeleteObject(bodyFont); DeleteObject(smallFont); DeleteObject(labelFont);
    DeleteObject(backgroundBrush); DeleteObject(whiteBrush);
    CloseHandle(mutex);
    return 0;
}

static int selfTest(void)
{
    WCHAR temporary[MAX_PATH], file[MAX_PATH], keyPath[128];
    DWORD count = GetTempPathW(MAX_PATH, temporary);
    if (!count || count + 40 >= MAX_PATH) return 1;
    wsprintfW(file, L"%sCopilotKeyV2-%lu.ini", temporary, GetCurrentProcessId());
    wsprintfW(keyPath, L"Software\\Milorgs\\CopilotKeyBindingTests-%lu", GetCurrentProcessId());
    int failures = 0;
    Settings original = {0};
    original.claude = TRUE;
    lstrcpyW(original.executable, L"C:\\Program Files\\Claude 测试\\claude.exe");
    if (!saveSettings(file, &original)) ++failures;
    Settings loaded = loadSettings(file);
    if (!loaded.claude || lstrcmpW(original.executable, loaded.executable)) ++failures;
    original.claude = FALSE; original.executable[0] = 0;
    if (!saveSettings(file, &original)) ++failures;
    loaded = loadSettings(file);
    if (loaded.claude || loaded.executable[0]) ++failures;
    StartupState missing, present, removed;
    if (snapshotStartup(keyPath, &missing) != ERROR_SUCCESS || missing.exists) ++failures;
    if (setStartup(keyPath, TRUE) != ERROR_SUCCESS) ++failures;
    if (snapshotStartup(keyPath, &present) != ERROR_SUCCESS || !present.exists || present.type != REG_SZ) ++failures;
    WCHAR expected[MAX_PATH + 32];
    wsprintfW(expected, L"\"%s\" --background", executablePath);
    if (present.exists && lstrcmpW((const WCHAR *)present.data, expected)) ++failures;
    if (setStartup(keyPath, FALSE) != ERROR_SUCCESS) ++failures;
    if (snapshotStartup(keyPath, &removed) != ERROR_SUCCESS || removed.exists) ++failures;
    if (restoreStartup(keyPath, &present) != ERROR_SUCCESS) ++failures;
    if (snapshotStartup(keyPath, &removed) != ERROR_SUCCESS || !removed.exists || removed.bytes != present.bytes ||
        memcmp(removed.data, present.data, present.bytes)) ++failures;
    if (restoreStartup(keyPath, &missing) != ERROR_SUCCESS) ++failures;
    DeleteFileW(file);
    RegDeleteKeyW(HKEY_CURRENT_USER, keyPath);
    printf("Settings persistence and isolated startup registry checks: %s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR commandLine, int show)
{
    (void)previous;
    if (!preparePaths()) return 1;
    if (lstrcmpW(commandLine, L"--background") == 0) return runWorker(instance);
    if (lstrcmpW(commandLine, L"--self-test") == 0) return selfTest();
    return showSettings(instance, show);
}
