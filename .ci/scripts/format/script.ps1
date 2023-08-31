# SPDX-FileCopyrightText: 2019 yuzu Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

param (
    [string]$ClangPath
)

Set-PSDebug -Trace 1

# Set good encoding for input
$OutputEncoding = [Console]::InputEncoding = [Console]::OutputEncoding = New-Object System.Text.UTF8Encoding
# Set good encoding for clang-output
$PSDefaultParameterValues['*:Encoding'] = 'utf8'

$src_trailing=Get-ChildItem -Path src -Exclude "*.png","*.jpg","*.jar" -Recurse | Select-String -Pattern '\s$' -List | Select-Object -Property Path, LineNumber, Line
$files_trailing=Get-ChildItem -Path "*.txt", "*.md", "Doxyfile", ".gitignore", ".gitmodules", ".ci*", "dist/*.desktop", "dist/*.svg", "dist/*.xml" | Select-String -Pattern '\s$' -List | Select-Object -Property Path, LineNumber, Line

if ($src_trailing -or $files_trailing) {
    Write-Output $src_trailing
    Write-Output $files_trailing
    Write-Output "`nTrailing whitespace found, aborting"
    exit 1
}

# Set default clang-format
if ($ClangPath) {
    if (Test-Path $ClangPath) {
        $CLANG_FORMAT=$ClangPath
    } else {
        Write-Error 'Invalid path given as ClangPath'
        exit 1
    }
} else {
    if (-not $CLANG_FORMAT) {
        $CLANG_FORMAT='build/externals/clang-format-15.exe'
    }
}
Invoke-expression "$CLANG_FORMAT --version"

if ("$TRAVIS_EVENT_TYPE" -eq 'pull_request') {
    # Get list of every file modified in this pull request
    $files_to_lint="$(git diff --name-only --diff-filter=ACMRTUXB $TRAVIS_COMMIT_RANGE | grep '^src/[^.]*[.]\(cpp\|h\)$')"
} else {
    # Check everything for branch pushes
    $files_to_lint=Get-ChildItem -Path src -Include '*.cpp','*.h' -Recurse
}

# Turn off tracing for this because it's too verbose
Set-PSDebug -Trace 0

foreach  ($f in $files_to_lint) {
    $orig=Get-Content $f | %{$i = 1} { New-Object psobject -prop @{LineNum=$i;Text=$_}; $i++}
    $formated=(Invoke-expression "$CLANG_FORMAT -style=file '$f'") | %{$i = 1} { New-Object psobject -prop @{LineNum=$i;Text=$_}; $i++}
    $diff=Compare-Object $orig $formated -Property Text -PassThru

    if ($diff) {
        Write-Output "!!! $f not compliant to coding style, here is the fix:"
        Write-Output $diff | Format-Table
        $fail=1
    }
}

Set-PSDebug -Trace 0

if ($fail -eq 1) {
    exit 1
}
