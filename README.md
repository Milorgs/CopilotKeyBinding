# Copilot Key Binding

Remap the Copilot key to Codex or Claude on Windows.

## Downloads

- [2.2](https://github.com/Milorgs/CopilotKeyBinding/releases/tag/v2.2): adds System, Light, and Dark appearance choices and a manual update check.
- [2.1](https://github.com/Milorgs/CopilotKeyBinding/releases/tag/v2.1): settings window, Codex/Claude selection, startup control, and Chinese/English support.
- [1.0](https://github.com/Milorgs/CopilotKeyBinding/releases/tag/v1.0): a small background-only mapper that opens Codex.

For 2.2, download [CopilotKey22.exe](https://github.com/Milorgs/CopilotKeyBinding/releases/download/v2.2/CopilotKey22.exe), keep it in a permanent folder, and run it. No installation, extraction, or companion files are needed. Select an app and click **Save**. Language, appearance, startup, and update checking are under **More**.

For 1.0, download `CopilotKey-windows-x64.zip` and run `CopilotKey.exe`. Add a shortcut to `shell:startup` if it should run at sign-in.

The keyboard must send `Win + Shift + F23`. The target application must already be installed. Before upgrading, exit the old mapper so that only one version runs at a time.

## Changes in 2.2

Theme choices apply immediately and survive restarts. Update checking queries the latest formal GitHub release when clicked and offers its download page if a newer version is available. Navigation arrows use smooth strokes with rounded ends. Existing settings and app selection are retained.

## Build

Install MinGW-w64 GCC/G++ and run:

```powershell
# 2.2 (v2/dist/CopilotKey22.exe)
.\v2\build.ps1
# 1.0
.\build.ps1
```

Both scripts accept `-Compiler` with the path to `gcc.exe` when it is not on PATH. Runtime dependencies are Windows system libraries only.

Version documentation: [1.0](docs/versions/1.0.md), [2.1](docs/versions/2.1.md), [2.2](docs/versions/2.2.md). Neither version records ordinary keys or typed text.

This is an independent utility, not an OpenAI, Anthropic, or Microsoft product.

---

# 中文说明

将 Windows 电脑的 Copilot 键映射为打开 Codex 或 Claude。

## 下载

- [2.2](https://github.com/Milorgs/CopilotKeyBinding/releases/tag/v2.2)：提供跟随系统、浅色、深色主题和手动检查更新。
- [2.1](https://github.com/Milorgs/CopilotKeyBinding/releases/tag/v2.1)：提供设置窗口、应用选择、开机自启动和中英文切换。
- [1.0](https://github.com/Milorgs/CopilotKeyBinding/releases/tag/v1.0)：轻量后台映射程序，按 Copilot 键打开 Codex。

2.2 只需下载 [CopilotKey22.exe](https://github.com/Milorgs/CopilotKeyBinding/releases/download/v2.2/CopilotKey22.exe)，放在固定目录后双击运行，无需安装、解压或附带其他文件。选择应用并点击“保存”；语言、外观、自启动和检查更新位于“更多”。

1.0 可下载 ZIP 后解压运行 `CopilotKey.exe`，或直接下载单独的 EXE。需要登录后自动运行时，将快捷方式放入 `shell:startup`。

键盘必须发送 `Win + Shift + F23`，目标应用需事先安装。升级前请先退出旧版映射程序，确保同一时间只运行一个版本。

## 2.2 更新内容

主题选择立即生效，重启后仍会保留。检查更新会按需查询 GitHub 最新正式版，有新版时可打开下载页面。返回和链接箭头采用圆头线条。已有设置和应用选择继续保留。

## 构建

安装 MinGW-w64 GCC/G++ 后，在项目目录执行：

```powershell
# 2.2，输出 v2/dist/CopilotKey22.exe
.\v2\build.ps1
# 1.0
.\build.ps1
```

GCC 不在 PATH 时，可通过 `-Compiler` 指定 `gcc.exe` 路径。运行只依赖 Windows 系统组件。

完整版本说明：[1.0](docs/versions/1.0.md)、[2.1](docs/versions/2.1.md)、[2.2](docs/versions/2.2.md)。程序不记录普通按键或输入文字。

这是独立工具，不是 OpenAI、Anthropic 或 Microsoft 的官方产品。
