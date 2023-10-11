#!/bin/bash

# Define relative paths
DEPLOY_DIR="../deploy/Executables"
CONVERTVIDEOS_DIR="../python/convertVideos"
FFMPEG2PATH_DIR="../python/ffmpeg2path"

# Convert Python scripts to EXEs using PyInstaller
# pyinstaller --onefile "$CONVERTVIDEOS_DIR/convertvideos.py"
# pyinstaller --onefile "$FFMPEG2PATH_DIR/select_ffmpeg_folder.py"

# Copy the generated EXEs to the deploy directory
cp "$CONVERTVIDEOS_DIR/dist/convertVideos.exe" "$DEPLOY_DIR"
cp "$FFMPEG2PATH_DIR/dist/select_ffmpeg_folder.exe" "$DEPLOY_DIR"

# Copy the setup_ffmpeg.bat to the deploy directory
cp "$FFMPEG2PATH_DIR/setup_ffmpeg.bat" "$DEPLOY_DIR"

echo "Conversion and copy complete!"
