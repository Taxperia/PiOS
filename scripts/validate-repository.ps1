$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$product = Get-Content -Raw (Join-Path $root 'config/product.json') | ConvertFrom-Json
$required = 'id','name','version','channel','supportUrl','githubRepo','updateKeyring','accent','accentDark','wallpaper'
foreach ($key in $required) {
    if (-not $product.PSObject.Properties.Name.Contains($key)) {
        throw "product.json is missing '$key'"
    }
}
if ($product.accent -notmatch '^#[0-9A-Fa-f]{6}$') { throw 'Invalid accent color' }
if ($product.accentDark -notmatch '^#[0-9A-Fa-f]{6}$') { throw 'Invalid dark accent color' }

[xml](Get-Content -Raw (Join-Path $root 'data/labwc/rc.xml')) | Out-Null
Get-ChildItem -LiteralPath (Join-Path $root 'translations') -Filter '*.ts' | ForEach-Object {
    [xml](Get-Content -Raw -LiteralPath $_.FullName) | Out-Null
}

if ($product.version -notmatch '^\d+\.\d+\.\d+(?:-(alpha|beta|rc)\.\d+)?$' -or
    $product.channel -notin @('stable', 'alpha', 'beta', 'rc')) {
    throw 'Invalid product version or release channel'
}
if (($product.channel -eq 'stable' -and $product.version.Contains('-')) -or
    ($product.channel -ne 'stable' -and $product.version -notmatch "-$($product.channel)\.\d+$")) {
    throw 'The product version does not match its release channel'
}

$catalog = Get-Content -Raw (Join-Path $root 'data/store/catalog.json') | ConvertFrom-Json
if ($catalog.version -ne 1 -or $catalog.apps.Count -lt 1) {
    throw 'The store catalog is empty or has an unsupported version'
}
foreach ($app in $catalog.apps) {
    if ($app.source -notin @('flatpak', 'apt') -or [string]::IsNullOrWhiteSpace($app.id) -or
        [string]::IsNullOrWhiteSpace($app.name)) {
        throw 'The store catalog contains an invalid application entry'
    }
}

Add-Type -AssemblyName System.Drawing
$wallpaperPath = Join-Path $root 'artwork/wallpapers/fidan-mountain-dawn-4k.png'
$wallpaper = [System.Drawing.Image]::FromFile($wallpaperPath)
try {
    if ($wallpaper.Width -ne 3840 -or $wallpaper.Height -ne 2160) {
        throw "Wallpaper must be 3840x2160, found $($wallpaper.Width)x$($wallpaper.Height)"
    }
} finally {
    $wallpaper.Dispose()
}

$imageLayer = Get-Content -Raw (Join-Path $root 'image/layer/fidan-desktop.yaml')
if ($imageLayer -notmatch 'X-Env-Layer-Version: 0\.1\.0' -or
    $imageLayer -notmatch '\$\{@SRCROOT\}/packages/fidan-desktop_') {
    throw 'The rpi-image-gen layer metadata or local package paths are invalid'
}

$requiredFiles = @(
    'CMakeLists.txt',
    'src/shell/qml/Main.qml',
    'src/firstboot/qml/FirstBoot.qml',
    'src/greeter/qml/Greeter.qml',
    'src/store/qml/Store.qml',
    'src/taskmanager/qml/TaskManager.qml',
    'scripts/fidan-update',
    'image/config/fidan-pi5.yaml',
    'image/bdebstrap/customize',
    'packaging/debian/control',
    'artwork/wallpapers/fidan-mountain-dawn-4k.png'
)
foreach ($file in $requiredFiles) {
    if (-not (Test-Path -LiteralPath (Join-Path $root $file))) {
        throw "Required file is missing: $file"
    }
}

Write-Output "Repository structure and product metadata are valid."

