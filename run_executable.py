import tkinter as tk, subprocess
from pathlib import Path

base = Path(__file__).parent
models = sorted((base / "cgproject/models").glob("*.mgf"))
exe = base / "built_binaries/Debug/cgproject.exe"

root = tk.Tk(); root.title("Select a model")
lb = tk.Listbox(root, width=40, height=15)
for m in models: 
	lb.insert("end", m.name)

lb.pack(padx=10, pady=10)

def launch():
    if lb.curselection():
        subprocess.Popen([str(exe), str(models[lb.curselection()[0]].resolve())])
        root.destroy()

tk.Button(root, text="Launch", command=launch).pack(pady=(0, 10))
root.mainloop()