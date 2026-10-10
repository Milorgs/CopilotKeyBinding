#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>

#define OPEN_CODEX (WM_APP + 1)
#define F23 0x86

static HHOOK keyboardHook;
static DWORD messageThread;
static BOOL captured;
static const WCHAR target[] = L"shell:AppsFolder\\OpenAI.Codex_2p2nqsd0c76g0!App";

/* Record lifecycle/errors only, never ordinary keyboard input. */
static void writeStatus(const char *event)
{
    WCHAR path[MAX_PATH];
    DWORD length = GetModuleFileNameW(NULL, path, MAX_PATH);
    if (!length || length >= MAX_PATH) return;
    while (length && path[length - 1] != L'\\') --length;
    if (!length || length + 11 >= MAX_PATH) return;
    lstrcpyW(path + length, L"status.log");
    HANDLE file = CreateFileW(path, FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return;
    SYSTEMTIME time;
    char line[160];
    DWORD written;
    GetLocalTime(&time);
    int size = wsprintfA(line, "%04u-%02u-%02uT%02u:%02u:%02u %s\r\n",
        time.wYear, time.wMonth, time.wDay, time.wHour,
        time.wMinute, time.wSecond, event);
    WriteFile(file, line, (DWORD)size, &written, NULL);
    CloseHandle(file);
}

static BOOL openCodex(void)
{
    WCHAR explorer[MAX_PATH], command[MAX_PATH + 128];
    UINT length = GetWindowsDirectoryW(explorer, MAX_PATH);
    if (!length || length + 14 >= MAX_PATH) return FALSE;
    lstrcatW(explorer, L"\\explorer.exe");
    wsprintfW(command, L"\"%s\" %s", explorer, target);
    STARTUPINFOW startup = {0};
    PROCESS_INFORMATION process = {0};
    startup.cb = sizeof(startup);
    if (!CreateProcessW(explorer, command, NULL, NULL, FALSE, 0,
            NULL, NULL, &startup, &process)) {
        writeStatus("LAUNCH_ERROR_NATIVE");
        return FALSE;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return TRUE;
}

static LRESULT CALLBACK onKey(int code, WPARAM message, LPARAM data)
{
    if (code == HC_ACTION) {
        const KBDLLHOOKSTRUCT *key = (const KBDLLHOOKSTRUCT *)data;
        if (key->vkCode == F23) {
            BOOL down = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
            BOOL up = message == WM_KEYUP || message == WM_SYSKEYUP;
            BOOL win = (GetAsyncKeyState(VK_LWIN) & 0x8000) ||
                       (GetAsyncKeyState(VK_RWIN) & 0x8000);
            BOOL shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
            if (down && (captured || (win && shift))) {
                captured = TRUE;
                return 1;
            }
            if (up && captured) {
                captured = FALSE;
                /* Prevent the swallowed shortcut from opening Start. */
                keybd_event(0xE8, 0, 0, 0);
                keybd_event(0xE8, 0, KEYEVENTF_KEYUP, 0);
                /* Launch outside the hook so it returns promptly. */
                PostThreadMessageW(messageThread, OPEN_CODEX, 0, 0);
                return 1;
            }
        }
    }
    return CallNextHookEx(keyboardHook, code, message, data);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous,
    PWSTR commandLine, int show)
{
    (void)previous;
    (void)show;
    if (lstrcmpW(commandLine, L"--check") == 0) return openCodex() ? 0 : 1;
    HANDLE mutex = CreateMutexW(NULL, FALSE, L"Local\\CodexCopilotKeyBinding");
    if (!mutex) return 1;
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(mutex);
        return 0;
    }
    messageThread = GetCurrentThreadId();
    MSG message;
    PeekMessageW(&message, NULL, WM_USER, WM_USER, PM_NOREMOVE);
    keyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, onKey, instance, 0);
    if (!keyboardHook) {
        writeStatus("HOOK_FAILED_NATIVE");
        CloseHandle(mutex);
        return 1;
    }
    writeStatus("READY_NATIVE");
    int result;
    while ((result = GetMessageW(&message, NULL, 0, 0)) > 0) {
        if (message.message == OPEN_CODEX) openCodex();
        else {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    UnhookWindowsHookEx(keyboardHook);
    CloseHandle(mutex);
    return result < 0 ? 1 : 0;
}
