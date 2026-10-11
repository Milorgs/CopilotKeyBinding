param([string]$Compiler = 'gcc.exe')
$ErrorActionPreference = 'Stop'
$taskCompiler = Get-Command $Compiler -ErrorAction Stop
$taskDist = Join-Path $PSScriptRoot 'dist'
New-Item -ItemType Directory -Path $taskDist -Force | Out-Null
$taskOutput = Join-Path $taskDist 'CopilotKey22.exe'
$taskCompilerDirectory = Split-Path $taskCompiler.Source
$taskCpp = Join-Path $taskCompilerDirectory 'g++.exe'
$taskWindres = Join-Path $taskCompilerDirectory 'windres.exe'
& $taskCpp -Os -std=c++17 -fno-exceptions -fno-rtti -Wall -Wextra -Werror -c `
    (Join-Path $PSScriptRoot 'drawing.cpp') -o (Join-Path $taskDist 'drawing.o')
if ($LASTEXITCODE -ne 0) { throw 'Drawing helper compilation failed.' }
& $taskWindres -I $PSScriptRoot -i (Join-Path $PSScriptRoot 'app.rc') -o (Join-Path $taskDist 'app.o')
if ($LASTEXITCODE -ne 0) { throw 'Icon resource compilation failed.' }
& $taskCompiler.Source -Os -s -Wall -Wextra -Werror -municode -mwindows `
    -o $taskOutput (Join-Path $PSScriptRoot 'CopilotKey2.c') (Join-Path $taskDist 'drawing.o') (Join-Path $taskDist 'app.o') `
    -luser32 -lgdi32 -ladvapi32 -ldwmapi -luxtheme -lmsimg32 -lwinhttp
if ($LASTEXITCODE -ne 0) { throw "GCC failed with exit code $LASTEXITCODE" }
Get-FileHash -LiteralPath $taskOutput -Algorithm SHA256
