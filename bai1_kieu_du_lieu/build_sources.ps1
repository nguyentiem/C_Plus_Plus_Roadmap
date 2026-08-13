$files = Get-ChildItem -Path . -Recurse -File |
    Where-Object { $_.Extension -in '.cpp', '.cc', '.cxx' } |
    ForEach-Object { [System.IO.Path]::GetRelativePath((Get-Location).Path, $_.FullName).Replace('\\', '/') }

if (-not $files) {
    Write-Error 'No C++ source files found.'
    exit 1
}

$files -join ' '
