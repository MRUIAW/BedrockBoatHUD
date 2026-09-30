param(
    [string]$Tag = ''
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
$package = Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'tooth.json') | ConvertFrom-Json
$version = $package.version
$releaseConfig = Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'release-config.json') | ConvertFrom-Json
if ($releaseConfig.minecraftSeries -notmatch '^\d+\.\d+x?$') {
    throw 'Invalid Minecraft series label in release-config.json'
}
$releaseTitle = "v$version-mc$($releaseConfig.minecraftSeries)"
if ($version -notmatch '^\d+\.\d+\.\d+(-[0-9A-Za-z.-]+)?$') {
    throw "Invalid release version: $version"
}
if ($Tag -and $Tag -cne "v$version") {
    throw "Tag $Tag does not match package version v$version"
}
if ($package.format_version -ne 3 -or $package.tooth -cne 'github.com/MRUIAW/BedrockBoatHUD') {
    throw 'Unexpected lip manifest format or package identifier'
}
if ($package.variants.Count -ne 1 -or $package.variants[0].label -cne 'client') {
    throw 'Release must contain only the client variant'
}

$modRoot = Join-Path $projectRoot 'bin/BedrockBoatHUD'
$manifest = Get-Content -Raw -LiteralPath (Join-Path $modRoot 'manifest.json') | ConvertFrom-Json
if ($manifest.version -cne $version -or $manifest.name -cne 'BedrockBoatHUD' -or
    $manifest.platform -cne 'client' -or $manifest.entry -cne 'BedrockBoatHUD.dll') {
    throw 'Built mod manifest does not match the release. Rebuild first.'
}
$relativeFiles = @('BedrockBoatHUD.dll', 'BedrockBoatHUD.pdb', 'manifest.json', 'lang/en.json', 'lang/zh_CN.json')
foreach ($file in $relativeFiles) {
    $item = Get-Item -LiteralPath (Join-Path $modRoot $file)
    if ($item.Length -eq 0) { throw "Empty release file: $file" }
}
$english = Get-Content -Raw -LiteralPath (Join-Path $modRoot 'lang/en.json') | ConvertFrom-Json
$chinese = Get-Content -Raw -LiteralPath (Join-Path $modRoot 'lang/zh_CN.json') | ConvertFrom-Json
$englishKeys = @($english.PSObject.Properties.Name | Sort-Object)
$chineseKeys = @($chinese.PSObject.Properties.Name | Sort-Object)
if (Compare-Object $englishKeys $chineseKeys) { throw 'Language key sets differ' }

$changelog = Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'CHANGELOG.md')
$escapedVersion = [regex]::Escape($version)
$releaseSection = [regex]::Match(
    $changelog,
    "(?ms)^## \[$escapedVersion\] - \d{4}-\d{2}-\d{2}\r?\n(?<notes>.*?)(?=^## |^\[Unreleased\]:|\z)"
)
if (-not $releaseSection.Success) { throw "No dated changelog section for $version" }

$releaseRoot = Join-Path $projectRoot 'build/release'
$stagingRoot = Join-Path $projectRoot ("build/package-$version-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $releaseRoot -Force | Out-Null
$stagedMod = Join-Path $stagingRoot 'BedrockBoatHUD'
New-Item -ItemType Directory -Path (Join-Path $stagedMod 'lang') -Force | Out-Null
foreach ($file in $relativeFiles) {
    Copy-Item -LiteralPath (Join-Path $modRoot $file) -Destination (Join-Path $stagedMod $file)
}
$documents = @(
    'README.md', 'README.zh-CN.md', 'CHANGELOG.md', 'LICENSE', 'tooth.json', 'release-config.json', 'pack_icon.png', 'AGENTS.md',
    'TELEMETRY_GUIDE.md', 'DEVELOPMENT_STATUS.md', 'COMBINED_BOATHUD_FEATURE_SPEC.md',
    'LEVILAMINA_BOATHUD_FEASIBILITY.md', 'PROJECT_IMPLEMENTATION_LOGIC.md'
)
foreach ($file in $documents) {
    Copy-Item -LiteralPath (Join-Path $projectRoot $file) -Destination (Join-Path $stagingRoot $file)
}
Copy-Item -LiteralPath (Join-Path $projectRoot 'examples') -Destination (Join-Path $stagingRoot 'examples') -Recurse

$archiveName = 'BedrockBoatHUD-client-windows-x64.zip'
$archivePath = Join-Path $releaseRoot $archiveName
$assetUrl = $package.variants[0].assets[0].urls[0].Replace('{{tooth}}', $package.tooth).Replace('{{version}}', $version)
if ($assetUrl -cne "https://github.com/MRUIAW/BedrockBoatHUD/releases/download/v$version/$archiveName") {
    throw 'lip asset URL does not match the release archive'
}
$placement = $package.variants[0].assets[0].placements[0]
if ($placement.src -cne 'BedrockBoatHUD/' -or $placement.dest -cne 'mods/BedrockBoatHUD/') {
    throw 'lip placement does not match the archive layout'
}
foreach ($directory in @('config', 'data')) {
    if ($package.variants[0].preserve_files -cnotcontains "mods/BedrockBoatHUD/$directory/**") {
        throw "lip does not preserve the installed $directory directory"
    }
}
Compress-Archive -Path (Join-Path $stagingRoot '*') -DestinationPath $archivePath -Force

Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [System.IO.Compression.ZipFile]::OpenRead($archivePath)
try {
    $entries = @($zip.Entries | ForEach-Object { $_.FullName.Replace('\', '/') })
    foreach ($file in $relativeFiles) {
        if ($entries -cnotcontains "BedrockBoatHUD/$file") { throw "Archive missing $file" }
    }
    if ($entries | Where-Object { $_ -match '^BedrockBoatHUD/(config|data)/' }) {
        throw 'Archive contains private runtime data'
    }
} finally {
    $zip.Dispose()
}
$hash = (Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash.ToLowerInvariant()
[System.IO.File]::WriteAllText((Join-Path $releaseRoot 'SHA256SUMS.txt'), "$hash  $archiveName`n")
[System.IO.File]::WriteAllText((Join-Path $releaseRoot 'release-title.txt'), $releaseTitle + "`n")
$sourceCommit = (& git -C $projectRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Cannot determine release source commit' }
[System.IO.File]::WriteAllText(
    (Join-Path $releaseRoot 'release-notes.md'),
    $releaseSection.Groups['notes'].Value.Trim() + "`n`nSource commit: [$sourceCommit](https://github.com/MRUIAW/BedrockBoatHUD/commit/$sourceCommit)`n"
)
Copy-Item -LiteralPath (Join-Path $projectRoot 'tooth.json') -Destination (Join-Path $releaseRoot 'tooth.json') -Force
Write-Output "Validated release: v$version"
Write-Output "Release title: $releaseTitle"
Write-Output "Archive: $archivePath"
Write-Output "SHA256: $hash"
