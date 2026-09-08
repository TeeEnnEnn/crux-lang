# Crux installer for Windows PowerShell
# Usage: iwr https://raw.githubusercontent.com/TheophilusNenhanga/crux-lang/main/scripts/install/install.ps1 | iex
param(
  [string]$Version = "latest",
  [string]$InstallDir = "$env:USERPROFILE\.local\bin",
  [string]$StdlibDir = "$env:USERPROFILE\.local\share\crux\stdlib"
)

$ErrorActionPreference = "Stop"
$Repo = if ($env:GITHUB_REPOSITORY) { $env:GITHUB_REPOSITORY } else { "TheophilusNenhanga/crux-lang" }

$Arch = "amd64"
$Platform = "windows"
$Binary = "crux-${Platform}-${Arch}.exe"
$StdlibArchive = "crux-stdlib.zip"

# Allow overriding base URL for local testing (e.g. $env:CRUX_BASE_URL="http://127.0.0.1:8766")
if ($env:CRUX_BASE_URL) {
  $UrlBase = $env:CRUX_BASE_URL
} elseif ($Version -eq "latest") {
  $UrlBase = "https://github.com/$Repo/releases/latest/download"
} else {
  if (-not $Version.StartsWith("v")) { $Version = "v$Version" }
  $UrlBase = "https://github.com/$Repo/releases/download/$Version"
}

Write-Host "Installing Crux $Version for $Platform-$Arch..."

New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null
New-Item -ItemType Directory -Force -Path $StdlibDir | Out-Null
$TmpDir = Join-Path $env:TEMP "crux-install-$(Get-Random)"
New-Item -ItemType Directory -Path $TmpDir | Out-Null

try {
  $BinaryUrl = "$UrlBase/$Binary"
  Write-Host "Downloading $BinaryUrl..."
  Invoke-WebRequest -Uri $BinaryUrl -OutFile (Join-Path $TmpDir "crux.exe")
  Move-Item -Force (Join-Path $TmpDir "crux.exe") (Join-Path $InstallDir "crux.exe")
  Write-Host "Installed crux to $InstallDir\crux.exe"

  $StdlibUrl = "$UrlBase/$StdlibArchive"
  Write-Host "Downloading stdlib $StdlibUrl..."
  try {
    Invoke-WebRequest -Uri $StdlibUrl -OutFile (Join-Path $TmpDir $StdlibArchive)
    Expand-Archive -Force -Path (Join-Path $TmpDir $StdlibArchive) -DestinationPath $TmpDir
    $ExtractedStdlib = Join-Path $TmpDir "stdlib"
    if (Test-Path $ExtractedStdlib) {
      Copy-Item -Recurse -Force "$ExtractedStdlib\*" $StdlibDir
      Write-Host "Installed stdlib to $StdlibDir"
    }
  } catch {
    Write-Warning "Stdlib archive not found, skipping: $_"
  }

  Write-Host "Add $InstallDir to your PATH if not already."
  Write-Host "Crux $Version installed successfully."
} finally {
  if (Test-Path $TmpDir) { Remove-Item -Recurse -Force $TmpDir }
}
