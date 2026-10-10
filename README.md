# Copilot Key Binding

I wanted the Copilot key on my Windows PC to open Codex, so I made this small utility to give the key a use that fits my workflow.

The first version used .NET and WinForms. I later switched it to native Win32 C to reduce its background memory use. It runs without .NET or AutoHotkey.

## What it does

- Captures `Win + Shift + F23` and opens Codex when I release the key.
- Waits for keyboard events without polling and launches the app outside the keyboard hook.
- Runs as a single instance; holding the key does not repeatedly launch Codex.
- Runs without a window or tray icon.
- Logs startup and errors only, without recording ordinary keys or typed text.

## Getting started

1. Install Codex on Windows x64.
2. Download `CopilotKey-windows-x64.zip` from [Releases](https://github.com/Milorgs/CopilotKeyBinding/releases) and extract it to a permanent folder.
3. Run `CopilotKey.exe`, then press the Copilot key.

I built this for keyboards whose Copilot key sends `Win + Shift + F23`. Other key sequences are not supported by this version.

The current launch target is the Microsoft Store app ID `OpenAI.Codex_2p2nqsd0c76g0!App`. If that ID changes in a future app update, update `target` in the source and rebuild.

### Start automatically when signing in

Press `Win + R`, enter `shell:startup`, and place a shortcut to `CopilotKey.exe` in the folder that opens.

### Stop the mapper

End `CopilotKey.exe` in Task Manager. If you added a startup shortcut, remove it to prevent the mapper from starting at your next sign-in.

### Troubleshooting

Run `CopilotKey.exe --check` to request a Codex launch without installing the keyboard hook. A successful exit means the launch request was sent; check that Codex actually opens.

The `status.log` file beside the executable records `READY_NATIVE`, `HOOK_FAILED_NATIVE`, or `LAUNCH_ERROR_NATIVE`.

## Building from source

Install MinGW-w64 GCC on Windows, then run this in PowerShell:

```powershell
.\build.ps1
# If GCC is not on PATH, pass its location:
.\build.ps1 -Compiler 'C:\mingw64\bin\gcc.exe'
```

The script produces `dist/CopilotKey.exe` and prints its SHA-256 hash.
GitHub Actions builds a Windows x64 executable on pushes to `main`. Pushing a `v*` tag also publishes a release with the executable.

## Memory use on my PC

After launching Codex, I measured a working set of about **5.5 MB** and private memory of about **0.87 MB**. The earlier .NET/WinForms version used about **35 MB** of working-set memory.

I also observed no increase in CPU time during a three-second idle sample. These measurements come from my own Windows PC; usage can differ on other devices.

## About this project

This is my personal shortcut utility. It is not an official OpenAI or Microsoft product.

— Milorgs
