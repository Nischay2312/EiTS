#!/bin/bash

# Define relative paths
ROOT_DIR=$(pwd)  # Capture the initial directory
DEPLOY_DIR="../deploy/Executables"
CONVERTVIDEOS_DIR="../python/convertVideos"
FFMPEG2PATH_DIR="../python/ffmpeg2path"

# Change directory to CONVERTVIDEOS_DIR and build the EXE
cd "$CONVERTVIDEOS_DIR"
pyinstaller --onefile "convertvideos.py"

# Go back to root directory
cd "$ROOT_DIR"

# Copy the generated EXE to the deploy directory
cp "$CONVERTVIDEOS_DIR/dist/convertVideos.exe" "$DEPLOY_DIR"

# Change directory to FFMPEG2PATH_DIR and build the EXE
cd "$FFMPEG2PATH_DIR"
pyinstaller --onefile "select_ffmpeg_folder.py"

# Go back to root directory
cd "$ROOT_DIR"

# Copy the generated EXE and the bat file to the deploy directory
cp "$FFMPEG2PATH_DIR/dist/select_ffmpeg_folder.exe" "$DEPLOY_DIR"
cp "$FFMPEG2PATH_DIR/setup_ffmpeg.bat" "$DEPLOY_DIR"

echo "Conversion and copy complete!"
