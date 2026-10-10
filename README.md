# Copilot Key Binding

Open Codex with the Copilot key on Windows.

Written in C using Win32. No .NET or AutoHotkey required. It runs in the background without a window or tray icon, and opens Codex when the key is released. Holding the key does not launch it repeatedly.

## Install

1. Install Codex on Windows x64.
2. Download [CopilotKey-windows-x64.zip](https://github.com/Milorgs/CopilotKeyBinding/releases/latest) and extract it to a permanent folder.
3. Run `CopilotKey.exe`, then press the Copilot key.

The keyboard must send `Win + Shift + F23`. Other key sequences are not supported.

To run at sign-in, press `Win + R`, enter `shell:startup`, and add a shortcut to `CopilotKey.exe`.

To stop it, end `CopilotKey.exe` in Task Manager. Remove the startup shortcut if you no longer want it to run at sign-in.

## Build

Install MinGW-w64 GCC, then run:

```powershell
.\build.ps1
# Or specify the compiler path:
.\build.ps1 -Compiler 'C:\mingw64\bin\gcc.exe'
```

The executable is written to `dist/CopilotKey.exe`. The script also prints its SHA-256 hash.

GitHub Actions builds on pushes to `main`. Push a `v*` tag to build and publish a release.

## Notes

- The target app ID is `OpenAI.Codex_2p2nqsd0c76g0!App`, set in `CopilotKey.c`. Change it and rebuild if your Codex installation uses a different ID.
- `CopilotKey.exe --check` sends a launch request without enabling the keyboard hook. Check that Codex opens; a successful exit alone does not confirm this.
- Only one instance runs at a time. The program waits for events instead of polling.
- `status.log` records startup and launch or hook errors. Ordinary keys and typed text are not logged.
- Measured working set after opening Codex: about 5.5 MB, down from about 35 MB in the earlier .NET version. Private memory was about 0.87 MB. These figures are from one Windows PC; usage varies.

An independent utility, with no affiliation to OpenAI or Microsoft.
