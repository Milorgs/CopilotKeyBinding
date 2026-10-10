param([string]$Compiler = 'gcc.exe')
$ErrorActionPreference = 'Stop'
$taskCompiler = Get-Command $Compiler -ErrorAction Stop
$taskDist = Join-Path $PSScriptRoot 'dist'
New-Item -ItemType Directory -Path $taskDist -Force | Out-Null
$taskOutput = Join-Path $taskDist 'CopilotKey2.exe'
& $taskCompiler.Source -Os -s -Wall -Wextra -Werror -municode -mwindows `
    -o $taskOutput (Join-Path $PSScriptRoot 'CopilotKey2.c') -luser32 -lgdi32 -ladvapi32 -lcomdlg32
if ($LASTEXITCODE -ne 0) { throw "GCC failed with exit code $LASTEXITCODE" }
Get-FileHash -LiteralPath $taskOutput -Algorithm SHA256
