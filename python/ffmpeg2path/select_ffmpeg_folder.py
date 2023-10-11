import os
import tkinter as tk
from tkinter import filedialog, messagebox

root = tk.Tk()
root.withdraw()  # Hide the main window

folder_path = filedialog.askdirectory(title="Select the folder containing ffmpeg.exe")

# Check if ffmpeg.exe exists in the selected folder
if folder_path and os.path.exists(os.path.join(folder_path, "ffmpeg.exe")):
    with open("ffmpeg_path.txt", "w") as file:
        file.write(folder_path)
else:
    messagebox.showerror("Error", "The selected folder doesn't seem to contain ffmpeg.exe. Please choose the correct 'bin' folder.")
