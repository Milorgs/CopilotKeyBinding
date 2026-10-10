$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$taskAssets = Join-Path $PSScriptRoot 'assets'
New-Item -ItemType Directory -Path $taskAssets -Force | Out-Null

function New-RoundedPath([single]$x, [single]$y, [single]$width, [single]$height, [single]$radius) {
    $taskPath = New-Object System.Drawing.Drawing2D.GraphicsPath
    $taskDiameter = $radius * 2
    $taskPath.AddArc($x, $y, $taskDiameter, $taskDiameter, 180, 90)
    $taskPath.AddArc($x + $width - $taskDiameter, $y, $taskDiameter, $taskDiameter, 270, 90)
    $taskPath.AddArc($x + $width - $taskDiameter, $y + $height - $taskDiameter, $taskDiameter, $taskDiameter, 0, 90)
    $taskPath.AddArc($x, $y + $height - $taskDiameter, $taskDiameter, $taskDiameter, 90, 90)
    $taskPath.CloseFigure()
    return ,$taskPath
}

function New-SvgPath([string]$Data) {
    $taskTokens = [regex]::Matches($Data,'[A-Za-z]|[-+]?(?:\d*\.\d+|\d+)(?:[eE][-+]?\d+)?')
    $taskPath = New-Object System.Drawing.Drawing2D.GraphicsPath
    $taskPath.FillMode = [System.Drawing.Drawing2D.FillMode]::Winding
    $taskIndex = 0; $taskX = 0.0; $taskY = 0.0; $taskStartX = 0.0; $taskStartY = 0.0; $taskCommand = ''
    while ($taskIndex -lt $taskTokens.Count) {
        if ($taskTokens[$taskIndex].Value -match '^[A-Za-z]$') {
            $taskCommand = $taskTokens[$taskIndex].Value
            $taskIndex++
            if ($taskCommand.ToLowerInvariant() -eq 'z') {
                $taskPath.CloseFigure(); $taskX = $taskStartX; $taskY = $taskStartY; continue
            }
        }
        $taskKind = $taskCommand.ToLowerInvariant()
        $taskCount = @{m=2;l=2;c=6;h=1;v=1}[$taskKind]
        if (-not $taskCount) { throw "Unsupported SVG command: $taskCommand" }
        $taskNumbers = @()
        for ($taskNumber = 0; $taskNumber -lt $taskCount; $taskNumber++) {
            $taskNumbers += [single]::Parse($taskTokens[$taskIndex++].Value,[Globalization.CultureInfo]::InvariantCulture)
        }
        $taskRelative = $taskCommand -ceq $taskKind
        switch ($taskKind) {
            'm' {
                $taskX = $taskNumbers[0] + $(if ($taskRelative) { $taskX } else { 0 })
                $taskY = $taskNumbers[1] + $(if ($taskRelative) { $taskY } else { 0 })
                $taskStartX = $taskX; $taskStartY = $taskY
                $taskPath.StartFigure()
                $taskCommand = if ($taskRelative) { 'l' } else { 'L' }
            }
            'c' {
                $taskBaseX = if ($taskRelative) { $taskX } else { 0 }
                $taskBaseY = if ($taskRelative) { $taskY } else { 0 }
                $taskPath.AddBezier([single]$taskX,[single]$taskY,
                    [single]($taskBaseX+$taskNumbers[0]),[single]($taskBaseY+$taskNumbers[1]),
                    [single]($taskBaseX+$taskNumbers[2]),[single]($taskBaseY+$taskNumbers[3]),
                    [single]($taskBaseX+$taskNumbers[4]),[single]($taskBaseY+$taskNumbers[5]))
                $taskX = $taskBaseX+$taskNumbers[4]; $taskY = $taskBaseY+$taskNumbers[5]
            }
            'l' {
                $taskNextX = $taskNumbers[0] + $(if ($taskRelative) { $taskX } else { 0 })
                $taskNextY = $taskNumbers[1] + $(if ($taskRelative) { $taskY } else { 0 })
                $taskPath.AddLine([single]$taskX,[single]$taskY,[single]$taskNextX,[single]$taskNextY)
                $taskX = $taskNextX; $taskY = $taskNextY
            }
            'h' {
                $taskNextX = $taskNumbers[0] + $(if ($taskRelative) { $taskX } else { 0 })
                $taskPath.AddLine([single]$taskX,[single]$taskY,[single]$taskNextX,[single]$taskY)
                $taskX = $taskNextX
            }
            'v' {
                $taskNextY = $taskNumbers[0] + $(if ($taskRelative) { $taskY } else { 0 })
                $taskPath.AddLine([single]$taskX,[single]$taskY,[single]$taskX,[single]$taskNextY)
                $taskY = $taskNextY
            }
        }
    }
    return ,$taskPath
}

$taskCanvas = New-Object System.Drawing.Bitmap(1024, 1024, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$taskGraphics = [System.Drawing.Graphics]::FromImage($taskCanvas)
$taskGraphics.Clear([System.Drawing.Color]::Transparent)
$taskGraphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$taskGraphics.ScaleTransform(4, 4)
$taskBasePath = New-RoundedPath 12 12 232 232 55
$taskBase = New-Object System.Drawing.Drawing2D.LinearGradientBrush([System.Drawing.Rectangle]::new(12,12,232,232), [System.Drawing.Color]::FromArgb(37,99,235), [System.Drawing.Color]::FromArgb(67,56,202), [single]90)
$taskGraphics.FillPath($taskBase,$taskBasePath)
$taskSvg = New-Object System.Xml.XmlDocument
$taskSvg.XmlResolver = $null
$taskSvg.Load((Join-Path $taskAssets 'copilot-key-glyph.svg'))
$taskGlyphData = $taskSvg.GetElementsByTagName('path')[0].GetAttribute('d')
$taskGlyph = New-SvgPath $taskGlyphData
$taskTransform = New-Object System.Drawing.Drawing2D.Matrix
$taskTransform.Translate(54,64)
$taskTransform.Scale([single]0.112,[single]0.112)
$taskGlyph.Transform($taskTransform)
$taskWhite = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::White)
$taskGraphics.FillPath($taskWhite,$taskGlyph)
$taskVector = @"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 256 256">
 <defs><linearGradient id="base" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#2563eb"/><stop offset="1" stop-color="#4338ca"/></linearGradient></defs>
 <rect x="12" y="12" width="232" height="232" rx="55" fill="url(#base)"/>
 <path transform="translate(54 64) scale(.112)" fill="white" d="$taskGlyphData"/>
</svg>
"@
$taskVector | Set-Content -LiteralPath (Join-Path $taskAssets 'app.svg') -Encoding utf8
$taskGraphics.Dispose()

$taskImages = @()
foreach ($taskSize in @(16,24,32,48,64,128,256)) {
    $taskImage = New-Object System.Drawing.Bitmap($taskSize,$taskSize,[System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $taskScale = [System.Drawing.Graphics]::FromImage($taskImage)
    $taskScale.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $taskScale.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $taskScale.DrawImage($taskCanvas,[System.Drawing.Rectangle]::new(0,0,$taskSize,$taskSize),0,0,1024,1024,[System.Drawing.GraphicsUnit]::Pixel)
    $taskScale.Dispose()
    if ($taskSize -eq 256) { $taskImage.Save((Join-Path $taskAssets 'app.png'),[System.Drawing.Imaging.ImageFormat]::Png) }
    $taskMemory = New-Object System.IO.MemoryStream
    $taskImage.Save($taskMemory,[System.Drawing.Imaging.ImageFormat]::Png)
    $taskImages += [pscustomobject]@{Size=$taskSize;Data=$taskMemory.ToArray()}
    $taskMemory.Dispose(); $taskImage.Dispose()
}
$taskIconStream = [System.IO.File]::Create((Join-Path $taskAssets 'app.ico'))
$taskWriter = New-Object System.IO.BinaryWriter($taskIconStream)
$taskWriter.Write([uint16]0); $taskWriter.Write([uint16]1); $taskWriter.Write([uint16]$taskImages.Count)
$taskOffset = 6 + 16 * $taskImages.Count
foreach ($taskImage in $taskImages) {
    $taskDimension = if ($taskImage.Size -eq 256) { 0 } else { $taskImage.Size }
    $taskWriter.Write([byte]$taskDimension); $taskWriter.Write([byte]$taskDimension)
    $taskWriter.Write([byte]0); $taskWriter.Write([byte]0)
    $taskWriter.Write([uint16]1); $taskWriter.Write([uint16]32)
    $taskWriter.Write([uint32]$taskImage.Data.Length); $taskWriter.Write([uint32]$taskOffset)
    $taskOffset += $taskImage.Data.Length
}
foreach ($taskImage in $taskImages) { $taskWriter.Write([byte[]]$taskImage.Data) }
$taskWriter.Dispose(); $taskCanvas.Dispose()
foreach ($taskObject in @($taskBasePath,$taskBase,$taskGlyph,$taskTransform,$taskWhite)) { $taskObject.Dispose() }
Write-Output 'Created app.ico (16–256 px) and app.png from the vector design.'
