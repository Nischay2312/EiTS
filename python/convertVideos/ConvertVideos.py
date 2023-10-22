import os
import subprocess
import tkinter as tk
from tkinter import ttk
from tkinter import filedialog, StringVar, IntVar, messagebox
import threading

class VideoConverterApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Video Converter")

        # Configure the grid for dynamic resizing
        self.root.grid_rowconfigure(7, weight=1)
        self.root.grid_columnconfigure(1, weight=1)

        # Variables
        self.input_folder_var = StringVar()
        self.output_folder_var = StringVar()
        self.output_name_var = StringVar()
        self.start_num_var = IntVar()
        self.quality_var = IntVar()
        self.quality_var.set(6)  # Default value

        # Layout
        tk.Label(root, text="Input Folder:").grid(row=0, column=0, sticky=tk.W, padx=10, pady=5)
        tk.Entry(root, textvariable=self.input_folder_var, width=40).grid(row=0, column=1, padx=10, pady=5)
        tk.Button(root, text="Browse", command=self.browse_input).grid(row=0, column=2, padx=10, pady=5)

        tk.Label(root, text="Output Folder:").grid(row=1, column=0, sticky=tk.W, padx=10, pady=5)
        tk.Entry(root, textvariable=self.output_folder_var, width=40).grid(row=1, column=1, padx=10, pady=5)
        tk.Button(root, text="Browse", command=self.browse_output).grid(row=1, column=2, padx=10, pady=5)

        tk.Label(root, text="Output Name:").grid(row=2, column=0, sticky=tk.W, padx=10, pady=5)
        tk.Entry(root, textvariable=self.output_name_var, width=40).grid(row=2, column=1, padx=10, pady=5)

        tk.Label(root, text="Starting Number:").grid(row=3, column=0, sticky=tk.W, padx=10, pady=5)
        tk.Entry(root, textvariable=self.start_num_var, width=40).grid(row=3, column=1, padx=10, pady=5)

        # New Layout Entry for Quality
        tk.Label(root, text="Quality (0-31):").grid(row=4, column=0, sticky=tk.W, padx=10, pady=5)
        tk.Entry(root, textvariable=self.quality_var, width=40).grid(row=4, column=1, padx=10, pady=5)

        # Adjusting the grid positioning of other elements to accommodate the new entry
        tk.Button(root, text="Convert", command=self.start_conversion_thread, width=20).grid(row=5, column=0, padx=10, pady=20)
        tk.Button(root, text="Repeat", command=self.reset_gui, width=10).grid(row=5, column=1, padx=10, pady=20)
        tk.Button(root, text="Exit", command=root.quit, width=10).grid(row=5, column=2, padx=10, pady=20)

        tk.Label(root, text="CONSOLE OUTPUT", font=("Arial", 12, "bold"), anchor="w").grid(row=6, column=0, columnspan=3, padx=10, pady=5, sticky="w")
        
        self.progress = ttk.Progressbar(root, orient=tk.HORIZONTAL, length=300, mode='determinate')
        self.progress.grid(row=7, column=0, columnspan=3, padx=(10, 0), pady=20, sticky= "ew")

        self.console = tk.Text(root, height=10, width=50)
        self.console.grid(row=8, column=0, columnspan=3, padx=10, pady=10, sticky='nsew')

        self.console_scrollbar = tk.Scrollbar(root, command=self.console.yview)
        self.console_scrollbar.grid(row=8, column=3, sticky='nsew')
        self.console['yscrollcommand'] = self.console_scrollbar.set
                
        tk.Button(root, text="Clear Console", command=self.clear_console, width=15).grid(row=9, column=2, padx=10, pady=5)
        

    def start_conversion_thread(self):
        thread = threading.Thread(target=self.convert_videos)
        thread.start()

    def browse_input(self):
        folder = filedialog.askdirectory()
        if folder:
            self.input_folder_var.set(folder.replace("/", "\\"))

    def browse_output(self):
        folder = filedialog.askdirectory()
        if folder:
            self.output_folder_var.set(folder.replace("/", "\\"))

    def clear_console(self):
        self.console.delete(1.0, tk.END)

    def convert_videos(self):
        input_folder = self.input_folder_var.get()
        output_folder = self.output_folder_var.get()
        output_name = self.output_name_var.get()
        start_num = self.start_num_var.get()
        quality = str(self.quality_var.get())

        self.progress["maximum"] = len(os.listdir(input_folder))
        self.progress["value"] = 0

        if not all([input_folder, output_folder, output_name, start_num]):
            tk.messagebox.showwarning("Warning", "All fields are required!")
            return

        for filename in os.listdir(input_folder):
            if filename.endswith(".mp4"):
                input_file = os.path.join(input_folder, filename)
                current_folder = os.path.join(output_folder, str(start_num))
                
                os.makedirs(current_folder, exist_ok=True)
                
                output_audio = os.path.join(current_folder, f"{output_name}.mp3")
                output_video = os.path.join(current_folder, f"{output_name}.mjpeg")

                try:
                    process = subprocess.Popen(['ffmpeg', '-i', input_file, '-y', '-ar', '44100', '-ac', '1', '-q:a', '9', output_audio], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
                    for line in process.stdout:
                        self.console.insert(tk.END, line)
                        self.console.see(tk.END)  # Auto-scrolling
                    
                        process = subprocess.Popen(['ffmpeg', '-i', input_file, '-y', '-vf', 'fps=24,scale=160:128:flags=lanczos', '-q:v', quality, output_video], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
                        self.console.insert(tk.END, line)
                        self.console.see(tk.END)  # Auto-scrolling

                except Exception as e:
                    tk.messagebox.showerror("Error", f"Error during conversion of {filename}: {e}")
                    return
                
                start_num += 1
            self.progress["value"] += 1
            self.root.update_idletasks()

        tk.messagebox.showinfo("Success", "Conversion completed successfully!")

    def reset_gui(self):
        self.input_folder_var.set('')
        self.output_folder_var.set('')
        self.output_name_var.set('')
        self.start_num_var.set('')

if __name__ == "__main__":
    root = tk.Tk()
    app = VideoConverterApp(root)
    root.mainloop()
