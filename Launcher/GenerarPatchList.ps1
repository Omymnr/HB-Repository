# GenerarPatchList.ps1
# Script para generar patchlist.json y version.txt para el sistema de actualizaciones

param(
    [Parameter(Mandatory=$true)]
    [string]$Version,
    
    [Parameter(Mandatory=$true)]
    [string]$FilesPath
)

Write-Host "============================================"
Write-Host "  Generador de Patchlist para Helbreath"
Write-Host "============================================"
Write-Host ""

# Verificar que la carpeta existe
if (-not (Test-Path $FilesPath)) {
    Write-Host "ERROR: La carpeta no existe: $FilesPath" -ForegroundColor Red
    exit 1
}

Write-Host "Version: $Version"
Write-Host "Carpeta: $FilesPath"
Write-Host ""

# Función para calcular MD5
function Get-FileMD5 {
    param([string]$Path)
    $md5 = New-Object System.Security.Cryptography.MD5CryptoServiceProvider
    $file = [System.IO.File]::OpenRead($Path)
    $hash = [System.BitConverter]::ToString($md5.ComputeHash($file)).Replace("-", "").ToLower()
    $file.Close()
    return $hash
}

# Obtener todos los archivos
$files = Get-ChildItem -Path $FilesPath -Recurse -File

Write-Host "Procesando $($files.Count) archivos..."
Write-Host ""

# Construir lista de archivos
$fileList = @()

foreach ($file in $files) {
    $relativePath = $file.FullName.Substring($FilesPath.Length + 1).Replace("\", "/")
    $hash = Get-FileMD5 -Path $file.FullName
    $size = $file.Length
    
    $fileList += @{
        path = $relativePath
        hash = $hash
        size = $size
    }
    
    Write-Host "  $relativePath - $hash"
}

# Crear objeto JSON
$patchlist = @{
    version = $Version
    files = $fileList
}

# Guardar patchlist.json
$jsonPath = Join-Path (Split-Path $FilesPath -Parent) "patchlist.json"
$patchlist | ConvertTo-Json -Depth 10 | Set-Content -Path $jsonPath -Encoding UTF8

# Guardar version.txt
$versionPath = Join-Path (Split-Path $FilesPath -Parent) "version.txt"
$Version | Set-Content -Path $versionPath -Encoding ASCII -NoNewline

Write-Host ""
Write-Host "============================================"
Write-Host "  Archivos generados:" -ForegroundColor Green
Write-Host "  - $jsonPath"
Write-Host "  - $versionPath"
Write-Host "============================================"
