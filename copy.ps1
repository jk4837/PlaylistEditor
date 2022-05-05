Param(
    [Parameter(Mandatory=$false)]
    [Switch] $clean,

    [Parameter(Mandatory=$false)]
    [Switch] $log,

    [Parameter(Mandatory=$false)]
    [Switch] $useDebug,

    [Parameter(Mandatory=$false)]
    [Switch] $self=$true,

    [Parameter(Mandatory=$false)]
    [Switch] $all,

    [Parameter(Mandatory=$false)]
    [String] $custom="",

    [Parameter(Mandatory=$false)]
    [Switch] $file,

    [Parameter(Mandatory=$false)]
    [Switch] $help
)

if ($help -eq $true) {
    echo "`"Copy`" - Builds and copies your mod to your quest, and also starts Beat Saber with optional logging"
    echo "`n-- Arguments --`n"

    echo "-Clean `t`t Performs a clean build (equvilant to running `"Build -clean`")"
    echo "-UseDebug `t Copied the debug version of the mod to your quest"
    echo "-Log `t`t Logs Beat Saber using the `"Start-Logging`" command"

    echo "`n-- Logging Arguments --`n"

    & $PSScriptRoot/start-logging.ps1 -help -excludeHeader

    exit
}

& $PSScriptRoot/build.ps1 -clean:$clean

if ($LASTEXITCODE -ne 0) {
    echo "Failed to build, exiting..."
    exit $LASTEXITCODE
}

if ($useDebug -eq $true) {
    $fileName = Get-ChildItem lib*.so -Path "build/debug" -Name
} else {
    $fileName = Get-ChildItem lib*.so -Path "build/" -Name
}

& adb push build/$fileName /sdcard/Android/data/com.beatgames.beatsaber/files/mods/$fileName

& adb push ExtraFiles/Icons/DeleteIcon.png /sdcard/ModData/com.beatgames.beatsaber/Mods/playlisteditor/Icons/DeleteIcon.png
& adb push ExtraFiles/Icons/InsertIcon.png /sdcard/ModData/com.beatgames.beatsaber/Mods/playlisteditor/Icons/InsertIcon.png
& adb push ExtraFiles/Icons/MoveUpIcon.png /sdcard/ModData/com.beatgames.beatsaber/Mods/playlisteditor/Icons/MoveUpIcon.png
& adb push ExtraFiles/Icons/MoveDownIcon.png /sdcard/ModData/com.beatgames.beatsaber/Mods/playlisteditor/Icons/MoveDownIcon.png
& adb push ExtraFiles/Icons/RemoveIcon.png /sdcard/ModData/com.beatgames.beatsaber/Mods/playlisteditor/Icons/RemoveIcon.png
& adb push ExtraFiles/Icons/DeleteAndRemoveIcon.png /sdcard/ModData/com.beatgames.beatsaber/Mods/playlisteditor/Icons/DeleteAndRemoveIcon.png
& adb push ExtraFiles/Icons/EnterIcon.png /sdcard/ModData/com.beatgames.beatsaber/Mods/playlisteditor/Icons/EnterIcon.png
& adb push ExtraFiles/Icons/LockIcon.png /sdcard/ModData/com.beatgames.beatsaber/Mods/playlisteditor/Icons/LockIcon.png
& adb push ExtraFiles/Icons/UnlockIcon.png /sdcard/ModData/com.beatgames.beatsaber/Mods/playlisteditor/Icons/UnlockIcon.png
& adb push ExtraFiles/Icons/AddImageIcon.png /sdcard/ModData/com.beatgames.beatsaber/Mods/playlisteditor/Icons/AddImageIcon.png
& adb push ExtraFiles/Icons/DeleteImageIcon.png /sdcard/ModData/com.beatgames.beatsaber/Mods/playlisteditor/Icons/DeleteImageIcon.png
& adb push ExtraFiles/Icons/StartRecordIcon.png /sdcard/ModData/com.beatgames.beatsaber/Mods/playlisteditor/Icons/StartRecordIcon.png
& adb push ExtraFiles/Icons/StopRecordIcon.png /sdcard/ModData/com.beatgames.beatsaber/Mods/playlisteditor/Icons/StopRecordIcon.png

& $PSScriptRoot/restart-game.ps1

if ($log -eq $true) { & $PSScriptRoot/start-logging.ps1 -self:$self -all:$all -custom:$custom -file:$file }