$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$managedComponents = Join-Path $projectRoot "managed_components"
$stagingRoot = Join-Path $env:TEMP ("ps4-bluepad32-" + [guid]::NewGuid().ToString("N"))
$templateRevision = "2c881858b6d71cfb8d7fcd7b7e3315df0381dc8b"
$arduinoRevision = "94afccf35fb1e401facddbcf9e13bcf7c76a31d8"

function Get-SourceArchive {
    param(
        [string]$Url,
        [string]$Destination
    )

    & curl.exe --fail --location --silent --show-error --retry 3 --retry-delay 2 $Url --output $Destination
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to download source archive: $Url"
    }
}

try {
    New-Item -ItemType Directory -Path $stagingRoot -Force | Out-Null
    New-Item -ItemType Directory -Path $managedComponents -Force | Out-Null

    $templateArchive = Join-Path $stagingRoot "bluepad32-template.zip"
    $arduinoArchive = Join-Path $stagingRoot "arduino-core.zip"

    Get-SourceArchive `
        "https://codeload.github.com/ricardoquesada/esp-idf-arduino-bluepad32-template/zip/$templateRevision" `
        $templateArchive
    Get-SourceArchive `
        "https://codeload.github.com/espressif/arduino-esp32/zip/$arduinoRevision" `
        $arduinoArchive

    Expand-Archive -LiteralPath $templateArchive -DestinationPath $stagingRoot
    Expand-Archive -LiteralPath $arduinoArchive -DestinationPath $stagingRoot

    $templateRoot = Join-Path $stagingRoot "esp-idf-arduino-bluepad32-template-$templateRevision"
    $arduinoRoot = Join-Path $stagingRoot "arduino-esp32-$arduinoRevision"

    foreach ($component in @("bluepad32", "bluepad32_arduino", "cmd_nvs", "cmd_system")) {
        $destination = Join-Path $managedComponents $component
        if (Test-Path -LiteralPath $destination) {
            Remove-Item -LiteralPath $destination -Recurse -Force
        }

        Copy-Item `
            -LiteralPath (Join-Path $templateRoot "components\$component") `
            -Destination $destination `
            -Recurse -Force
    }

    $arduinoDestination = Join-Path $managedComponents "arduino"
    if (Test-Path -LiteralPath $arduinoDestination) {
        Remove-Item -LiteralPath $arduinoDestination -Recurse -Force
    }

    Copy-Item `
        -LiteralPath $arduinoRoot `
        -Destination $arduinoDestination `
        -Recurse -Force

    Write-Host "Bluepad32 and Arduino components are ready in managed_components."
}
finally {
    if (Test-Path -LiteralPath $stagingRoot) {
        Remove-Item -LiteralPath $stagingRoot -Recurse -Force
    }
}
