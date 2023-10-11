import os
import subprocess
import tkinter as tk
from tkinter import filedialog, simpledialog, messagebox

def select_directory(title="Select a directory"):
    """Prompt the user to select a directory and return the chosen path."""
    directory = filedialog.askdirectory(title=title)
    if directory:
        return directory.replace("/", "\\")  # Ensure we use backslashes for Windows paths
    return None


def convert_videos(input_folder, output_folder, output_name, start_num):
    """Convert videos using ffmpeg commands."""
    for filename in os.listdir(input_folder):
        if filename.endswith(".mp4"):
            input_file = os.path.join(input_folder, filename)
            current_folder = os.path.join(output_folder, str(start_num))
            
            os.makedirs(current_folder, exist_ok=True)
            
            output_audio = os.path.join(current_folder, f"{output_name}.mp3")
            output_video = os.path.join(current_folder, f"{output_name}.mjpeg")

            print(f"Processing file: {input_file}")
            print(f"Output audio will be: {output_audio}")
            print(f"Output video will be: {output_video}")

            # Convert to MP3
            try:
                print("Converting to MP3...")
                cmd = f'ffmpeg -i "{input_file}" -ar 44100 -ac 1 -q:a 9 "{output_audio}"'
                subprocess.run(cmd, shell=True, check=True)
                print("MP3 conversion done.")
            except subprocess.CalledProcessError as e:
                print(f"Error during MP3 conversion: {e.output}")
                raise

            # Convert to MJPEG
            try:
                print("Converting to MJPEG...")
                cmd = f'ffmpeg -i "{input_file}" -vf "fps=24,scale=160:128:flags=lanczos" -q:v 9 "{output_video}"'
                subprocess.run(cmd, shell=True, check=True)
                print("MJPEG conversion done.")
            except subprocess.CalledProcessError as e:
                print(f"Error during MJPEG conversion: {e.output}")
                raise
            
            start_num += 1


def main():
    root = tk.Tk()
    root.withdraw()  # Hide the root window

    # Prompt user for required inputs
    input_folder = select_directory("Select Input Folder")
    if not input_folder:
        return

    output_folder = select_directory("Select Output Folder")
    if not output_folder:
        return

    output_name = simpledialog.askstring("Output Name", "Enter the output file name:")
    if not output_name:
        return

    start_num = simpledialog.askinteger("Starting Number", "Enter the starting folder number:")
    if not start_num:
        return

    try:
        convert_videos(input_folder, output_folder, output_name, start_num)
        messagebox.showinfo("Success", "Conversion completed successfully!")
    except Exception as e:
        messagebox.showerror("Error", f"An error occurred: {str(e)}")

if __name__ == "__main__":
    main()
