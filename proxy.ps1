$IsAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $IsAdmin) {
    Write-Host "Requesting administrative privileges..."
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = "powershell.exe"
    $psi.Arguments = "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`""
    $psi.Verb = "runas"
    [System.Diagnostics.Process]::Start($psi) | Out-Null
    exit
}

try {
    $WslIP = (wsl hostname -I 2>$null).Trim().Split()[0]
    if (-not $WslIP) {
        throw "Could not get WSL IP. Make sure WSL is running."
    }
} catch {
    Write-Error "Failed to detect WSL IP: $_"
    exit 1
}

$Port = Read-Host "Enter port to forward (default 8080)"
if (-not $Port) { $Port = 8080 }
if ($Port -as [int] -eq $null) {
    Write-Error "Invalid port number."
    exit 1
}

$fwRuleName = "WSL-QEMU-port-$Port"

function Add-Proxy {
    param($Port, $WslIP, $fwName)
    Write-Host "`n[+] Setting up portproxy for $Port -> $WslIP`:$Port"
    & netsh interface portproxy add v4tov4 listenport=$Port listenaddress=0.0.0.0 connectport=$Port connectaddress=$WslIP
    & netsh advfirewall firewall add rule name="WSL QEMU" dir=in action=allow protocol=TCP localport=$Port

    Write-Host "   Portproxy + firewall rule created."
}

function Remove-Proxy {
    param($Port, $fwName)
    Write-Host "`n[-] Removing portproxy and firewall rule..."
    & netsh interface portproxy delete v4tov4 listenport=$Port listenaddress=0.0.0.0

    Write-Host "   Cleanup done."
}

try {
    Add-Proxy -Port $Port -WslIP $WslIP -fwName $fwRuleName

    Write-Host "`nNow Windows is forwarding:"
    Write-Host "  http://localhost:$Port  ->  WSL ($WslIP`:$Port)"
    Write-Host ""
    Write-Host "Press ENTER to remove proxy and exit..."
    Read-Host | Out-Null
}
finally {
    Remove-Proxy -Port $Port -fwName $fwRuleName
}
