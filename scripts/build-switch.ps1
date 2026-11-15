param(
 [Parameter(Mandatory=$true)][string]$Source,
 [Parameter(Mandatory=$true)][string]$BuildDir,
 [Parameter(Mandatory=$true)][string]$VuDir,
 [Parameter(Mandatory=$true)][string]$ShaderCpp,
 [string]$DevkitPro=$env:DEVKITPRO
)
$ErrorActionPreference='Stop'
if(-not $DevkitPro){throw 'Set DEVKITPRO or pass -DevkitPro'}
$env:DEVKITPRO=$DevkitPro
$env:DEVKITA64=Join-Path $DevkitPro 'devkitA64'
$env:PATH="$DevkitPro/tools/bin;$env:DEVKITA64/bin;"+$env:PATH
& cmake -S $Source -B $BuildDir -G Ninja "-DCMAKE_TOOLCHAIN_FILE=$DevkitPro/cmake/Switch.cmake" -DCMAKE_BUILD_TYPE=Release -DPS2X_SWITCH_DEKO3D=ON -DPS2X_GOW_LOW_RESERVED_MEMORY=ON -DPS2X_ENABLE_RUNTIME_LOGS=OFF -DPS2X_ENABLE_DEBUG_UI=OFF -DPS2X_ENABLE_FFMPEG=OFF -DPS2X_ENABLE_IOP_RPC_TRACE=OFF -DPS2X_ENABLE_RUNNER_PCH=ON -DPS2X_ENABLE_RUNNER_UNITY_BUILD=ON -DPS2X_STRICT_RETURN_DIAGNOSTICS=OFF -DPS2X_DEFAULT_BOOT_ELF=sdmc:/switch/gow/disc-gows16941/GOWS_169.41 "-DPS2X_VU1_PREGENERATED_DIR=$VuDir" "-DPS2X_DK_SHADERS=$ShaderCpp"
if($LASTEXITCODE -ne 0){throw 'Configure failed'}
& cmake --build $BuildDir --target ps2EntryRunner_nro --parallel 2
if($LASTEXITCODE -ne 0){throw 'Build failed'}
