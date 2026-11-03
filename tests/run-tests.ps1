# Runs bin\qsysver.exe over the fixtures and checks the results.
#   fixtures\valid\<version>.qsys   header-only files; the name is the expected version
#   fixtures\invalid\*.qsys         must be rejected
# With -Shell, also reads System.Comment / System.Software.ProductVersion through the
# Windows Shell, which checks the registered handler end to end.
param([switch]$Shell)

# Not 'Stop': Windows PowerShell 5.1 turns qsysver's expected stderr into a terminating error.
$ErrorActionPreference = 'Continue'
$root = Split-Path $PSScriptRoot
$exe = Join-Path $root 'bin\qsysver.exe'
if (-not (Test-Path $exe)) { throw "Build first: $exe not found" }

$failed = 0
function Check($ok, $msg) {
    if ($ok) { Write-Host "PASS  $msg" } else { Write-Host "FAIL  $msg" -ForegroundColor Red; $script:failed++ }
}

foreach ($f in Get-ChildItem "$PSScriptRoot\fixtures\valid\*.qsys") {
    $expected = $f.BaseName
    $out = & $exe $f.FullName 2>&1
    $got = ("$out" -split "`t")[0]
    Check ($LASTEXITCODE -eq 0 -and $got -eq $expected) "qsysver $($f.Name) -> $got"
}

foreach ($f in Get-ChildItem "$PSScriptRoot\fixtures\invalid\*.qsys") {
    & $exe $f.FullName 2>$null | Out-Null
    Check ($LASTEXITCODE -eq 1) "qsysver rejects $($f.Name)"
}

if ($Shell) {
    $sh = New-Object -ComObject Shell.Application
    foreach ($f in Get-ChildItem "$PSScriptRoot\fixtures\valid\*.qsys") {
        $item = $sh.NameSpace($f.DirectoryName).ParseName($f.Name)
        $comment = $item.ExtendedProperty('System.Comment')
        $version = $item.ExtendedProperty('System.Software.ProductVersion')
        Check ($comment -eq "Q-SYS Designer $($f.BaseName)" -and $version -eq $f.BaseName) "shell $($f.Name) -> '$comment' / '$version'"
    }
    foreach ($f in Get-ChildItem "$PSScriptRoot\fixtures\invalid\*.qsys") {
        $item = $sh.NameSpace($f.DirectoryName).ParseName($f.Name)
        Check ([string]::IsNullOrEmpty($item.ExtendedProperty('System.Comment'))) "shell $($f.Name) has no comment"
    }
}

if ($failed) { Write-Host "$failed failed" -ForegroundColor Red; exit 1 }
Write-Host 'All passed'
