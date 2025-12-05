# ===============================================
# Script de Publicación de Actualización
# Ejecutar este script para publicar una nueva versión
# ===============================================

param(
    [Parameter(Mandatory=$true)]
    [string]$NewVersion
)

$ErrorActionPreference = "Stop"

# Rutas
$RepoPath = "D:\HB Repository"
$ProductionPath = "D:\HelbreathServer-main - copia\Helbreath"
$UpdatesPath = "$RepoPath\updates"
$FilesPath = "$UpdatesPath\files"

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  Publicando Version $NewVersion" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""

# Crear carpetas si no existen
New-Item -Path $FilesPath -ItemType Directory -Force | Out-Null

# Archivos a incluir
$filesToUpdate = @("Game.exe", "HelbreathLauncher.exe")
$fileList = @()

Write-Host "Calculando hashes..." -ForegroundColor Yellow
foreach ($file in $filesToUpdate) {
    $sourcePath = "$ProductionPath\$file"
    if (Test-Path $sourcePath) {
        $hash = (Get-FileHash $sourcePath -Algorithm MD5).Hash.ToLower()
        $size = (Get-Item $sourcePath).Length
        
        $fileList += @{
            path = $file
            hash = $hash
            size = $size
        }
        
        Write-Host "  $file" -ForegroundColor White
        Write-Host "    Hash: $hash" -ForegroundColor Gray
        Write-Host "    Size: $size bytes" -ForegroundColor Gray
        
        # Copiar archivo a updates/files/
        Copy-Item $sourcePath "$FilesPath\$file" -Force
    } else {
        Write-Host "  ERROR: $file no encontrado en $ProductionPath" -ForegroundColor Red
        exit 1
    }
}

Write-Host ""
Write-Host "Generando archivos de actualizacion..." -ForegroundColor Yellow

# Crear version.txt
$NewVersion | Out-File -FilePath "$UpdatesPath\version.txt" -Encoding UTF8 -NoNewline
Write-Host "  version.txt creado" -ForegroundColor Green

# Crear patchlist.json
$patchlist = @{
    version = $NewVersion
    files = $fileList
}
$patchlist | ConvertTo-Json -Depth 3 | Out-File -FilePath "$UpdatesPath\patchlist.json" -Encoding UTF8
Write-Host "  patchlist.json creado" -ForegroundColor Green

Write-Host ""
Write-Host "Subiendo a GitHub..." -ForegroundColor Yellow

# Git commands
Set-Location $RepoPath
git add updates/
git commit -m "Update v$NewVersion - Game.exe y HelbreathLauncher.exe"
git push

Write-Host ""
Write-Host "==========================================" -ForegroundColor Green
Write-Host "  Version $NewVersion publicada!" -ForegroundColor Green
Write-Host "==========================================" -ForegroundColor Green
Write-Host ""
Write-Host "Los usuarios veran la actualizacion al abrir el Launcher." -ForegroundColor Cyan
