Get-ChildItem "C:\Program Files (x86)\EMS Technologies\LUT Server\Run\*.dll" | ForEach-Object {
    try {
        $bytes = [System.IO.File]::ReadAllBytes($_.FullName)
        $peOffset = [System.BitConverter]::ToInt32($bytes, 0x3C)
        $machine  = [System.BitConverter]::ToUInt16($bytes, $peOffset + 4)
        $arch = switch ($machine) {
            0x8664 { "x64" }
            0x014c { "x86 (32-bit)" }
            default { "unknown 0x{0:X4}" -f $machine }
        }
    } catch { $arch = "read error" }
    [PSCustomObject]@{ Name = $_.Name; Arch = $arch }
} | Sort-Object Arch | Format-Table -AutoSize