# Copilot Key Binding 2.0 preview

A small settings window for the Copilot key mapper. Choose Codex or Claude and turn startup at sign-in on or off.

Version 1.0 remains available for a simple background-only mapper. This preview uses a separate executable, settings folder, and startup entry.

## Use

1. Extract the 2.0 download to a permanent folder.
2. Run `CopilotKey2.exe` to open settings.
3. Choose Codex or Claude. For an older Claude installation, select its `.exe` with **Browse**; leave the path blank for the packaged app.
4. Set **Run at sign-in** as needed, then click **Save**.
5. Click **Start mapping** to run the background mapper. Close the settings window when finished.

**Open app** checks the selected launch target without changing the Copilot key mapping or saving settings.
**Stop** stops only the 2.0 background mapper. It does not change the startup setting.

If 1.0 is running, 2.0 settings can be opened but the new mapper will not take over the key. Before switching versions, stop 1.0 and remove its startup shortcut yourself. The 2.0 startup switch is blocked while 1.0 is running.

## Supported apps

- Codex packaged app: `OpenAI.Codex_2p2nqsd0c76g0!App`.
- Claude packaged app: `Claude_pzs8sxrjxfjjc!Claude`.
- Claude executable: an optional `.exe` path selected in settings.

The keyboard must send `Win + Shift + F23`. The target applications must already be installed. A launch request through Explorer does not prove the app opened; use **Open app** to check your installation.

## Settings

Target settings are stored in `%LOCALAPPDATA%\CopilotKeyBinding\v2\settings.ini`.
The startup entry is `CopilotKeyBindingV2` under the current user's Windows `Run` key and launches `CopilotKey2.exe --background`.

Opening settings does not enable mapping or startup. Changes take effect only after **Save**. Saved target changes apply on the next Copilot key press if the 2.0 mapper is running.

The background mapper waits for events. It records startup and errors in its own `status.log`, without recording ordinary keys or typed text. The settings window performs only a short status check after a start or stop request; there is no recurring idle timer.

## Build

Install MinGW-w64 GCC, then run:

```powershell
.\v2\build.ps1
# Or pass the compiler location:
.\v2\build.ps1 -Compiler 'C:\mingw64\bin\gcc.exe'
```

Output: `v2/dist/CopilotKey2.exe`.

The executable supports `--self-test` for settings persistence and startup-entry checks. These checks use a temporary INI file and an isolated test registry key, leaving the real startup entry untouched.

## Validation

Checked on Windows:

- Codex/Claude selection and the startup switch render and respond.
- The packaged Claude target opens Claude.
- Settings preserve a Unicode executable path.
- Startup enable, disable, and rollback preserve the expected entry contents in an isolated test key.
- The background process exits with code 2 when the 1.0 mapper is already running.

The 2.0 keyboard hook has not been exercised alongside 1.0, since only one version should own the key at a time.
