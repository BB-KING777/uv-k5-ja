# COMポートの状態と、掴んでいるプロセスを調べる
# 使い方:  PowerShell で  .\check_com.ps1
# （Sysinternals の handle.exe が PATH にあれば占有プロセスまで特定します）

Write-Host "=== 現在存在する COM ポート ===" -ForegroundColor Cyan
Get-CimInstance Win32_PnPEntity |
    Where-Object { $_.Name -match '\(COM\d+\)' } |
    Select-Object Name, Status, DeviceID |
    Format-Table -AutoSize

Write-Host "=== 問題を抱えているデバイス ===" -ForegroundColor Cyan
Get-PnpDevice | Where-Object { $_.Status -ne 'OK' } |
    Select-Object Status, Class, FriendlyName | Format-Table -AutoSize

Write-Host "=== ポートを掴んでいそうなプロセス ===" -ForegroundColor Cyan
$suspects = 'chrome','msedge','firefox','python','putty','TeraTerm','ttermpro',
            'CHIRP','chirp','UVStudio','K5Viewer','Claude'
Get-Process | Where-Object { $suspects -contains $_.ProcessName } |
    Select-Object Id, ProcessName, MainWindowTitle | Format-Table -AutoSize

if (Get-Command handle.exe -ErrorAction SilentlyContinue) {
    Write-Host "=== handle.exe による占有調査 ===" -ForegroundColor Cyan
    handle.exe -a -nobanner COM19
} else {
    Write-Host "handle.exe が無いため占有プロセスの特定は省略しました。" -ForegroundColor DarkGray
    Write-Host "必要なら https://learn.microsoft.com/sysinternals/downloads/handle" -ForegroundColor DarkGray
}

Write-Host "`n=== USBシリアルの再認識（管理者権限が必要） ===" -ForegroundColor Cyan
Write-Host "pnputil /enum-devices /class Ports"
