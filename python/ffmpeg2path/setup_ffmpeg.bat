@echo off
setlocal enabledelayedexpansion

:: Backup the current PATH to path_backup.txt
echo %PATH% > path_backup.txt

:: Run the packaged Python script (.exe) to show the GUI dialog
select_ffmpeg_folder.exe

:: Check if ffmpeg_path.txt exists (meaning user selected a valid folder)
if not exist ffmpeg_path.txt (
    echo Failed to get a valid ffmpeg path from user.
    exit /b 1
)

:: Read the ffmpeg path from ffmpeg_path.txt
set /p FFMPEG_DIR=<ffmpeg_path.txt
del ffmpeg_path.txt

:: Use PowerShell to permanently set the PATH, bypassing the 1024-character limit of setx
powershell -command "[Environment]::SetEnvironmentVariable('PATH', [Environment]::GetEnvironmentVariable('PATH', 'User') + ';%FFMPEG_DIR%', 'User')"

echo ffmpeg has been added to your PATH.
pause
