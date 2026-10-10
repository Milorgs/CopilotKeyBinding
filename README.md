# Copilot Key Binding

将 Windows 电脑的 Copilot 实体键映射为打开 Codex。使用原生 Win32 C 实现，无需 .NET 或 AutoHotkey。

## 功能

- 拦截 `Win + Shift + F23`，松开时打开 Codex。
- 事件驱动，无定时轮询；应用启动在键盘钩子返回后执行。
- 单实例运行，长按不会反复启动。
- 无常驻窗口和托盘图标。
- 日志只记录程序启动和错误，不记录普通按键或输入文字。

## 使用

1. 在 Windows x64 上安装 Codex。
2. 下载发布版本中的 `CopilotKey.exe`，放到一个固定目录。
3. 双击运行，再按 Copilot 键。

程序面向发送 `Win + Shift + F23` 的 Copilot 键。其他键盘信号不在当前版本支持范围内。
默认打开 Microsoft Store 应用标识 `OpenAI.Codex_2p2nqsd0c76g0!App`。
如果该标识在应用更新后变化，需要修改源码里的 `target` 并重新构建。

### 登录后自动运行

按 `Win + R`，输入 `shell:startup`，把 `CopilotKey.exe` 的快捷方式放入打开的文件夹。

### 停用

在任务管理器中结束 `CopilotKey.exe`；如已设为自动运行，同时删除启动文件夹里的快捷方式。

### 排查启动问题

运行 `CopilotKey.exe --check` 可以直接请求打开 Codex，而不安装键盘钩子。
此命令成功只表示启动请求已发出；仍需确认 Codex 窗口是否出现。
运行目录里的 `status.log` 会记录 `READY_NATIVE`、`HOOK_FAILED_NATIVE` 或 `LAUNCH_ERROR_NATIVE`。

## 构建

Windows 上安装 MinGW-w64 GCC，然后在 PowerShell 中执行：

```powershell
.\build.ps1
# GCC 不在 PATH 时：
.\build.ps1 -Compiler 'D:\Program Files\mingw64\bin\gcc.exe'
```

输出为 `dist/CopilotKey.exe`，构建脚本同时输出 SHA-256。
GitHub Actions 会在提交后构建 Windows x64 可执行文件；推送 `v*` 标签时创建包含可执行文件的 Release。

## 占用

在开发者的一台 Windows 电脑上，启动 Codex 后的工作集约 5.5 MB，私有内存约 0.87 MB。
原 .NET/WinForms 版本工作集约 35 MB。空闲 3 秒观察期内没有测到 CPU 时间增加。
这些是单机测量值，不保证其他设备占用相同。

## 说明

这是独立的快捷键工具，不是 OpenAI 或 Microsoft 官方产品。
