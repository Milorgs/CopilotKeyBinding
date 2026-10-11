# Copilot Key Binding 2.1

A compact settings window for the Copilot key mapper, with Codex/Claude selection, startup control, and Chinese/English support.

## Changes in 2.1

- Windows 11 Mica background and system light/dark colors.
- Antialiased rounded controls and a smooth circular startup switch.
- Mouse clicks no longer add focus outlines; Tab navigation still shows keyboard focus.
- A Copilot-key-style icon, embedded at sizes from 16 to 256 pixels.
- Language, startup at sign-in, version information, and the GitHub link are in **More**.
- The main window has two app choices and a single **Save** button.
- The Claude executable-path field and persistent status panel have been removed.

Mica requires Windows 11 build 22621 or newer. A solid background is used when the API is unavailable, transparency is disabled, or high contrast is enabled. See [Microsoft's backdrop documentation](https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/ne-dwmapi-dwm_systembackdrop_type).

Version 1.0 remains available for a simple background-only mapper. Version 2.1 uses a separate executable, settings folder, and startup entry.

## Use

1. Download `CopilotKey21.exe` and keep it in a permanent folder.
2. Run `CopilotKey21.exe` to open settings.
3. Choose Codex or Claude.
4. Click **Save** to apply the choice. The background mapper starts if no other mapper is running.
5. Close the settings window when finished.

The EXE contains the settings window, background mapper, icons, and both languages. No installer, companion DLL, icon file, or runtime download is required. Settings and the status log are created automatically in your user profile; the EXE folder does not need to be writable. Keep the EXE at the same location after enabling startup. If you move it, open it at the new location and turn startup off and on again.

There are no separate launch, start, or stop buttons. To stop mapping, end the background `CopilotKey21.exe` process in Task Manager.

Open **More** with the `•••` button. Language and startup changes are saved immediately and separately from the selected app. Turning startup off affects the next sign-in, not the current background process. The initial language follows Windows when no preference is saved.

If 1.0 is running, **Save** stores the new settings but does not take over the key. Before switching versions, stop 1.0 and remove its startup shortcut yourself. Startup is blocked while another mapper is running.

## Supported apps

- Codex packaged app: `OpenAI.Codex_2p2nqsd0c76g0!App`.
- Claude packaged app: `Claude_pzs8sxrjxfjjc!Claude`.

This version uses packaged applications only.

The keyboard must send `Win + Shift + F23`. The target applications must already be installed.

## Settings

Target settings are stored in `%LOCALAPPDATA%\CopilotKeyBinding\v2\settings.ini`.
The startup entry is `CopilotKeyBindingV2` under the current user's Windows `Run` key and launches `CopilotKey21.exe --background`.

Opening settings does not enable mapping or startup. App choices take effect only after **Save**. Saved target changes apply on the next Copilot key press if the mapper is running.

The background mapper waits for events. It records startup and errors in its own `status.log`, without recording ordinary keys or typed text. The settings window performs only a short status check after a start or stop request; there is no recurring idle timer.

## Resource use

The renderer reuses a native-resolution bitmap and samples only curved boundary pixels. It does not load GDI+ or allocate large temporary supersampled images.

On one Windows PC, the main window used about 15.4 MB of working-set memory and 2.06 MB of private memory, compared with 16.7 MB and 2.76 MB before this change. Thirty forced More-page redraws used 0.17 seconds of CPU time, down from 0.59 seconds. Neither version showed an increase in CPU time during a three-second idle sample. Results vary between devices and runs.

## Build

Install MinGW-w64 GCC/G++, then run:

```powershell
.\v2\build.ps1
# Or pass the compiler location:
.\v2\build.ps1 -Compiler 'C:\mingw64\bin\gcc.exe'
```

Output: `v2/dist/CopilotKey21.exe`. Runtime dependencies are Windows system libraries only. The vector icon and its source credits are in `v2/assets`; `make-icon.ps1` rebuilds the icon.

The executable supports `--self-test` for settings persistence and startup-entry checks. These checks use a temporary INI file and an isolated test registry key, leaving the real startup entry untouched.

## Validation

Checked on Windows:

- Codex/Claude selection, language switching, More navigation, and startup controls render and respond.
- The packaged Claude target opens Claude.
- Language changes survive a restart and preserve the stored target.
- Startup enable, disable, and rollback preserve the expected entry contents in an isolated test key.
- The background process exits with code 2 when the 1.0 mapper is already running.

Only one version should own the key at a time. The 2.x keyboard hook has not been exercised alongside 1.0.
