<#
  update_status.ps1
  - Attempts to connect to a TCP host/port and writes `updates/status.json`.
  - Optional: if -CommitPush is provided, commits and pushes the updated file (requires git credentials on host).

  Usage examples:
    .\update_status.ps1 -Host 89.7.69.125 -Port 2500
    .\update_status.ps1 -Host 127.0.0.1 -Port 2500 -CommitPush
#>

param(
  [string]$Host = '89.7.69.125',
  [int]$Port = 2500,
  [switch]$CommitPush,
  [string]$RepoPath = "D:\HB Repository"
)

function Test-TcpPort {
  param($host, $port, $timeout = 3000)
  try {
    $client = New-Object System.Net.Sockets.TcpClient
    $ar = $client.BeginConnect($host, $port, $null, $null)
    $wait = $ar.AsyncWaitHandle.WaitOne($timeout)
    if (-not $wait) { $client.Close(); return $false }
    $client.EndConnect($ar)
    $client.Close()
    return $true
  } catch { return $false }
}

$online = Test-TcpPort -host $Host -port $Port -timeout 3000
$players = if ($online) { 0 } else { 0 }

$status = @{ online = $online; players = $players; message = (if ($online) { 'Servidor Online' } else { 'Servidor Offline' }) }
$json = $status | ConvertTo-Json -Depth 4

$outPath = Join-Path $RepoPath 'updates\status.json'
Set-Content -Path $outPath -Value $json -Encoding UTF8
Write-Host "Wrote $outPath -> online=$online"

if ($CommitPush) {
  Push-Location $RepoPath
  git add updates/status.json
  git commit -m "Auto: update status.json (online=$online)" 2>$null
  git push origin main
  Pop-Location
  Write-Host "Committed and pushed status.json"
}
