# Copilot Key Binding 2.1 preview

A compact settings window for the Copilot key mapper, with Codex/Claude selection, startup control, and Chinese/English support.

## Changes in 2.1

- Windows 11 Mica background and system light/dark colors.
- Supersampled rounded controls and a smooth circular startup switch.
- Mouse clicks no longer add focus outlines; Tab navigation still shows keyboard focus.
- A Copilot-key-style icon, embedded at sizes from 16 to 256 pixels.
- Language selection, version information, and the GitHub link are in **More**.
- The Claude executable-path field and persistent status panel have been removed.

Mica requires Windows 11 build 22621 or newer. A solid background is used when the API is unavailable, transparency is disabled, or high contrast is enabled. See [Microsoft's backdrop documentation](https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/ne-dwmapi-dwm_systembackdrop_type).

Version 1.0 remains available for a simple background-only mapper. This preview uses a separate executable, settings folder, and startup entry.

## Use

1. Extract the 2.1 download to a permanent folder.
2. Run `CopilotKey21.exe` to open settings.
3. Choose Codex or Claude.
4. Set **Run at sign-in** as needed, then click **Save**.
5. Click **Start mapping** to run the background mapper. Close the settings window when finished.

**Open app** checks the selected launch target without changing the Copilot key mapping or saving settings.
**Stop** stops only the 2.x background mapper. It does not change the startup setting.

Open **More** with the `•••` button to switch languages or visit the project. Language changes are immediate and saved separately, without applying unsaved app or startup choices. The initial language follows Windows when no preference is saved.

If 1.0 is running, settings can be opened but the new mapper will not take over the key. Before switching versions, stop 1.0 and remove its startup shortcut yourself. Startup is blocked while another mapper is running. Version 1.0 remains the latest stable release.

## Supported apps

- Codex packaged app: `OpenAI.Codex_2p2nqsd0c76g0!App`.
- Claude packaged app: `Claude_pzs8sxrjxfjjc!Claude`.

This version uses packaged applications only. The 2.0 preview is still available for older Claude installations that need a custom executable path.

The keyboard must send `Win + Shift + F23`. The target applications must already be installed. A launch request through Explorer does not prove the app opened; use **Open app** to check your installation.

## Settings

Target settings are stored in `%LOCALAPPDATA%\CopilotKeyBinding\v2\settings.ini`.
The startup entry is `CopilotKeyBindingV2` under the current user's Windows `Run` key and launches `CopilotKey21.exe --background`.

Opening settings does not enable mapping or startup. App and startup choices take effect only after **Save**. Saved target changes apply on the next Copilot key press if the mapper is running.

The background mapper waits for events. It records startup and errors in its own `status.log`, without recording ordinary keys or typed text. The settings window performs only a short status check after a start or stop request; there is no recurring idle timer.

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
