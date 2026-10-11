#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <winhttp.h>
#include <stdlib.h>
#include <ctype.h>
#define APP_VERSION L"2.2"
#define UPDATE_COMPLETE (WM_APP + 2)

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
#define ID_CLAUDE 109
#define ID_CHINESE 110
#define ID_ENGLISH 111
#define ID_MORE 112
#define ID_BACK 113
#define ID_PROJECT 114
#define ID_THEME_SYSTEM 115
#define ID_THEME_LIGHT 116
#define ID_THEME_DARK 117
#define ID_UPDATE 118
#define STOP_EVENT L"Local\\CopilotKeyBindingV2.Stop"

typedef struct { BOOL claude; WCHAR executable[MAX_PATH]; BOOL chinese; int theme; } Settings;
typedef struct { BOOL exists; DWORD type, bytes; BYTE data[2048]; } StartupState;
static const WCHAR codexTarget[] = L"shell:AppsFolder\\OpenAI.Codex_2p2nqsd0c76g0!App";
static const WCHAR claudeTarget[] = L"shell:AppsFolder\\Claude_pzs8sxrjxfjjc!Claude";
static WCHAR executablePath[MAX_PATH], settingsPath[MAX_PATH], configDirectory[MAX_PATH];
static HHOOK hook;
static DWORD workerThread;
static BOOL captured, startupEnabled;
static HWND window, targetBox, startupButton;
static HFONT titleFont, bodyFont, smallFont, labelFont;
static HBRUSH backgroundBrush, whiteBrush;
static int dpi = 96;
static WCHAR statusText[256];
static BOOL statusError;
static BOOL chinese, selectedClaude, darkTheme, micaEnabled, highContrast;
static int themePreview = -1;
static BOOL solidPreview;
static int themeChoice;
static int updateState; /* 0 idle, 1 checking, 2 current, 3 available, 4 failed */
static HWND claudeButton;
static COLORREF foregroundColor, mutedColor, cardColor, borderColor, accentColor, baseColor;
static HTHEME textTheme;

typedef enum {
    TX_TITLE, TX_SUBTITLE, TX_TARGET, TX_PATH, TX_PATH_HINT, TX_STARTUP, TX_STARTUP_HINT,
    TX_BROWSE, TX_OPEN, TX_STOP, TX_START, TX_SAVE, TX_WINDOW,
    TX_LEGACY, TX_ACTIVE, TX_STOPPED, TX_INVALID_PATH, TX_LEGACY_STARTUP,
    TX_SAVE_FAILED, TX_STARTUP_FAILED, TX_ROLLBACK_OK, TX_ROLLBACK_FAILED,
    TX_SAVED_LEGACY, TX_SAVED, TX_LAUNCHED, TX_LAUNCH_FAILED, TX_START_FAILED,
    TX_START_REQUESTED, TX_STOP_REQUESTED, TX_LANGUAGE_FAILED,
    TX_MORE, TX_LANGUAGE, TX_VERSION, TX_PROJECT, TX_BACK, TX_LINK_FAILED, TX_SAVE_DONE, TX_THEME, TX_SYSTEM, TX_LIGHT, TX_DARK, TX_THEME_FAILED, TX_UPDATE, TX_CHECKING, TX_CURRENT, TX_AVAILABLE, TX_UPDATE_FAILED, TX_DOWNLOAD, TX_UPDATE_HINT, TX_COUNT
} TextId;
static TextId currentStatus = TX_STOPPED;
static const WCHAR *const messages[2][TX_COUNT] = {
    {
        L"Copilot key", L"Choose what your key opens.", L"Open with",
        L"Claude executable (optional)", L"Leave blank to use the packaged app.",
        L"Run at sign-in", L"Keep the mapper running in the background",
        L"Browse", L"Open app", L"Stop", L"Start mapping", L"Save", L"Copilot Key Binding",
        L"",
        L"Mapping is running. Saved target changes apply on the next key press.",
        L"Mapping is stopped. Save your settings, then start when ready.",
        L"Choose an existing Claude .exe, or leave the path blank for the packaged app.",
        L"Another mapper is running. Stop it and disable its startup entry before enabling startup here.",
        L"Could not save settings. Check access to your settings folder and startup entry.",
        L"Could not change startup. The target setting has not been saved.",
        L"Could not save the target. The previous startup setting was restored.",
        L"Saving and startup rollback failed. Check Windows Startup Apps.",
        L"Settings saved. 1.0 is still running with its original mapping.",
        L"Settings saved. Closing this window leaves the mapper running.",
        L"Launch request sent. Check that the selected app opened.",
        L"Could not launch the app. Check the executable path or installation.",
        L"Could not start the background mapper.",
        L"Starting the mapper. You can close this window once it is running.",
        L"Stopping the mapper. The startup setting is unchanged.",
        L"Could not save the language selection.",
        L"More", L"Language", L"Version", L"GitHub project ↗", L"Back", L"Could not open the project page.", L"Saved",
        L"Appearance", L"System", L"Light", L"Dark", L"Could not save the theme.",
        L"Check updates", L"Checking…", L"You are up to date.", L"An update is available.", L"Could not check. Try again.", L"Open the download page?", L"Get the latest version."
    },
    {
        L"Copilot 按键", L"选择按下按键后打开的应用。", L"打开应用",
        L"Claude 程序路径（可选）", L"使用打包安装的 Claude 时留空即可。",
        L"开机自启动", L"登录 Windows 后在后台运行映射程序",
        L"浏览", L"打开应用", L"停止", L"启动映射", L"保存", L"Copilot 按键设置",
        L"",
        L"映射正在运行。保存后，下次按键将使用新的应用选择。",
        L"映射尚未运行。请先保存设置，再按需启动。",
        L"请选择存在的 Claude .exe 文件；使用打包安装版本时可留空。",
        L"另一映射程序正在运行，请先关闭它并停用其开机启动。",
        L"无法保存设置，请检查设置文件夹和启动项的访问权限。",
        L"无法更改启动项，应用选择尚未保存。",
        L"无法保存应用选择，已恢复原来的启动设置。",
        L"保存和启动项恢复均失败，请检查 Windows 启动应用设置。",
        L"设置已保存。1.0 仍使用原有映射运行。",
        L"设置已保存，关闭窗口不会停止后台映射。",
        L"已发送启动请求，请确认所选应用已打开。",
        L"无法启动应用，请检查程序路径或安装情况。",
        L"无法启动后台映射程序。",
        L"正在启动映射，启动后可以关闭此窗口。",
        L"正在停止映射，开机启动设置保持不变。",
        L"无法保存语言选择。",
        L"更多", L"界面语言", L"版本", L"GitHub 项目 ↗", L"返回", L"无法打开项目页面。", L"已保存",
        L"外观", L"跟随系统", L"浅色", L"深色", L"无法保存主题。",
        L"检查更新", L"正在检查…", L"当前已是最新版本。", L"有新版本可用。", L"检查失败，请重试。", L"打开新版本下载页面？", L"获取最新版本。"
    }
};
static const WCHAR *tr(TextId id) { return messages[chinese ? 1 : 0][id]; }

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
    GetPrivateProfileStringW(L"Interface", L"Language", L"", name, 24, path);
    settings.chinese = name[0] ? lstrcmpiW(name, L"zh-CN") == 0 : PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_CHINESE;
    settings.theme = GetPrivateProfileIntW(L"Interface", L"Theme", 0, path);
    if (settings.theme < 0 || settings.theme > 2) settings.theme = 0;
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
        !WritePrivateProfileStringW(L"Mapping", L"ClaudeExecutable", settings->executable, temporary) ||
        !WritePrivateProfileStringW(L"Interface", L"Language", settings->chinese ? L"zh-CN" : L"en", temporary) ||
        !WritePrivateProfileStringW(L"Interface", L"Theme", settings->theme == 1 ? L"1" : settings->theme == 2 ? L"2" : L"0", temporary)) {
        DeleteFileW(temporary);
        return FALSE;
    }
    WritePrivateProfileStringW(NULL, NULL, NULL, temporary);
    /* Packaged hosts may redirect the paths onto different storage volumes. */
    if (!MoveFileExW(temporary, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH | MOVEFILE_COPY_ALLOWED)) {
        DeleteFileW(temporary);
        return FALSE;
    }
    WritePrivateProfileStringW(NULL, NULL, NULL, path);
    return TRUE;
}
static BOOL saveLanguage(const WCHAR *path, BOOL useChinese)
{
    Settings existing = loadSettings(path);
    existing.chinese = useChinese;
    return saveSettings(path, &existing);
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
    UINT count = GetWindowsDirectoryW(application, MAX_PATH);
    if (!count || count + 14 >= MAX_PATH) return FALSE;
    lstrcatW(application, L"\\explorer.exe");
    wsprintfW(command, L"\"%s\" %s", application, settings->claude ? claudeTarget : codexTarget);
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

static void setStatus(TextId id, BOOL error)
{
    currentStatus = id;
    lstrcpynW(statusText, tr(id), 256);
    statusError = error;
    InvalidateRect(window, NULL, FALSE);
}
static Settings selectedSettings(void)
{
    Settings settings = loadSettings(settingsPath);
    settings.claude = selectedClaude;
    settings.chinese = chinese;
    return settings;
}
static BOOL applySettings(void)
{
    Settings settings = selectedSettings();
    if (!ensureConfigDirectory()) {
        setStatus(TX_SAVE_FAILED, TRUE);
        return FALSE;
    }
    if (!saveSettings(settingsPath, &settings)) {
        setStatus(TX_SAVE_FAILED, TRUE);
        return FALSE;
    }
    setStatus(legacyRunning() ? TX_SAVED_LEGACY : TX_SAVED, FALSE);
    return TRUE;
}
static BOOL ensureMapper(void)
{
    /* Never replace an existing mapper, including the user's version 1.0. */
    if (mutexExists(SHARED_MUTEX)) return TRUE;
    WCHAR command[MAX_PATH + 32];
    wsprintfW(command, L"\"%s\" --background", executablePath);
    if (!createProcess(executablePath, command)) {
        setStatus(TX_START_FAILED, TRUE);
        return FALSE;
    }
    return TRUE;
}
#include "update.inc"
#include "ui.inc"

static int selfTest(void)
{
    WCHAR temporary[MAX_PATH], file[MAX_PATH], keyPath[128];
    DWORD count = GetTempPathW(MAX_PATH, temporary);
    if (!count || count + 40 >= MAX_PATH) return 1;
    wsprintfW(file, L"%sCopilotKeyV2-%lu.ini", temporary, GetCurrentProcessId());
    wsprintfW(keyPath, L"Software\\Milorgs\\CopilotKeyBindingTests-%lu", GetCurrentProcessId());
    int failures = 0;
    if (!testDrawing()) ++failures;
    Settings original = {0};
    original.claude = TRUE;
    original.chinese = TRUE; original.theme = 2;
    lstrcpyW(original.executable, L"C:\\Program Files\\Claude 测试\\claude.exe");
    if (!saveSettings(file, &original)) ++failures;
    Settings loaded = loadSettings(file);
    if (loaded.theme != 2 || !loaded.claude || !loaded.chinese || lstrcmpW(original.executable, loaded.executable)) ++failures;
    if (!saveLanguage(file, FALSE)) ++failures;
    loaded = loadSettings(file);
    if (loaded.theme != 2 || !loaded.claude || loaded.chinese || lstrcmpW(original.executable, loaded.executable)) ++failures;
    original.claude = FALSE; original.executable[0] = 0;
    original.chinese = FALSE;
    if (!saveSettings(file, &original)) ++failures;
    loaded = loadSettings(file);
    if (loaded.claude || loaded.executable[0] || loaded.chinese) ++failures;
    if (!saveLanguage(file, TRUE)) ++failures;
    loaded = loadSettings(file);
    if (loaded.claude || loaded.executable[0] || !loaded.chinese) ++failures;
    for (int language = 0; language < 2; ++language)
        for (int text = 0; text < TX_COUNT; ++text)
            if (!messages[language][text] || (text != TX_LEGACY && !messages[language][text][0])) ++failures;
    for (int theme = 0; theme < 3; ++theme) {
        original.theme = theme;
        if (!saveSettings(file, &original) || loadSettings(file).theme != theme) ++failures;
    }
    WritePrivateProfileStringW(L"Interface", L"Theme", L"99", file);
    if (loadSettings(file).theme != 0) ++failures;
    if (compareRelease("v2.1") != 1 || compareRelease("v2.2") != 1 ||
        compareRelease("v2.3") != 2 || compareRelease("v2.10") != 2 ||
        compareRelease("v2.2.1") != 2 || compareRelease("v3.0") != 2 ||
        compareRelease("v2.3-beta") != 0 || compareRelease("broken") != 0) ++failures;
    char tag[48];
    if (!releaseTag("{\"body\":\"\\\"tag_name\\\":fake\",\"tag_name\":\"v2.3\"}", tag) || strcmp(tag, "v2.3")) ++failures;
    if (releaseTag("{\"tag_name\":\"v2.3", tag)) ++failures;
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
static int diagnoseSettings(void)
{
    BOOL directory = ensureConfigDirectory();
    DWORD directoryError = GetLastError();
    WCHAR path[MAX_PATH];
    wsprintfW(path, L"%s\\diagnostic-%lu.ini", configDirectory, GetCurrentProcessId());
    Settings settings = {0};
    BOOL saved = directory && saveSettings(path, &settings);
    DWORD saveError = GetLastError();
    wprintf(L"Directory=%s\nCreated=%d Error=%lu\nSaved=%d Error=%lu\n", configDirectory,
        directory, directoryError, saved, saveError);
    if (saved) DeleteFileW(path);
    return saved ? 0 : 1;
}
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR commandLine, int show)
{
    (void)previous;
    if (!preparePaths()) return 1;
    if (lstrcmpW(commandLine, L"--background") == 0) return runWorker(instance);
    if (lstrcmpW(commandLine, L"--self-test") == 0) return selfTest();
    if (lstrcmpW(commandLine, L"--check-update") == 0) { int result = fetchUpdate(); printf("Update result: %d\n", result); return result ? 0 : 1; }
    if (lstrcmpW(commandLine, L"--diagnose-settings") == 0) return diagnoseSettings();
    if (lstrcmpW(commandLine, L"--preview-light") == 0) themePreview = 0;
    if (lstrcmpW(commandLine, L"--preview-solid") == 0) solidPreview = TRUE;
    return showSettings(instance, show);
}
