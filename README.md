# Copilot Key Binding

Remap the Copilot key to Codex or Claude on Windows.

## Downloads

- [2.1](https://github.com/Milorgs/CopilotKeyBinding/releases/tag/v2.1): settings window, Codex/Claude selection, startup control, and Chinese/English support.
- [1.0](https://github.com/Milorgs/CopilotKeyBinding/releases/tag/v1.0): a small background-only mapper that opens Codex.

For 2.1, download [CopilotKey21.exe](https://github.com/Milorgs/CopilotKeyBinding/releases/download/v2.1/CopilotKey21.exe), keep it in a permanent folder, and run it. No installation, extraction, or companion files are needed. Select an app and click **Save**. Language and startup settings are under **More**.

For 1.0, download `CopilotKey-windows-x64.zip` and run `CopilotKey.exe`. Add a shortcut to `shell:startup` if it should run at sign-in.

The keyboard must send `Win + Shift + F23`. The target application must already be installed. Only one mapper can own the key at a time; 2.1 does not stop or replace a running 1.0 instance.

## Build

Install MinGW-w64 GCC/G++ and run:

```powershell
# 2.1
.\v2\build.ps1
# 1.0
.\build.ps1
```

Both scripts accept `-Compiler` with the path to `gcc.exe` when it is not on PATH. Runtime dependencies are Windows system libraries only.

See [2.1 documentation](v2/README.md) for configuration details. Neither version records ordinary keys or typed text.

This is an independent utility, not an OpenAI, Anthropic, or Microsoft product.
