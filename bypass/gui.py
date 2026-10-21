#!/usr/bin/env python3
"""GUI wrapper for the Linkvertise / unlocknow.net bypass."""

import threading
import tkinter as tk
from tkinter import ttk, messagebox

from bypass import bypass


class BypassApp:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Link Bypass")
        self.root.geometry("600x300")
        self.root.configure(bg="#0a0a0f")
        self.root.resizable(False, False)

        self._build_ui()

    def _build_ui(self):
        bg = "#0a0a0f"
        fg = "#e8e8f0"
        accent = "#7c5cfc"
        entry_bg = "#1a1a26"
        border = "#2a2a3a"

        # Title
        tk.Label(
            self.root, text="Link Bypass", font=("Segoe UI", 18, "bold"),
            bg=bg, fg=accent,
        ).pack(pady=(20, 5))

        tk.Label(
            self.root, text="Paste an unlocknow.net or Linkvertise link",
            font=("Segoe UI", 10), bg=bg, fg="#8888a0",
        ).pack()

        # Input frame
        input_frame = tk.Frame(self.root, bg=bg)
        input_frame.pack(pady=15, padx=30, fill="x")

        self.url_var = tk.StringVar()
        self.entry = tk.Entry(
            input_frame, textvariable=self.url_var,
            font=("Consolas", 12), bg=entry_bg, fg=fg,
            insertbackground=fg, relief="flat", bd=8,
        )
        self.entry.pack(side="left", fill="x", expand=True)
        self.entry.bind("<Return>", lambda e: self._run_bypass())

        self.btn = tk.Button(
            input_frame, text="Bypass", font=("Segoe UI", 11, "bold"),
            bg=accent, fg="white", relief="flat", bd=0,
            padx=20, pady=8, cursor="hand2",
            command=self._run_bypass,
        )
        self.btn.pack(side="right", padx=(10, 0))

        # Status
        self.status_var = tk.StringVar(value="Ready")
        tk.Label(
            self.root, textvariable=self.status_var,
            font=("Segoe UI", 9), bg=bg, fg="#555566",
        ).pack()

        # Result frame
        result_frame = tk.Frame(self.root, bg=bg)
        result_frame.pack(pady=10, padx=30, fill="x")

        self.result_var = tk.StringVar()
        self.result_entry = tk.Entry(
            result_frame, textvariable=self.result_var,
            font=("Consolas", 11), bg=entry_bg, fg="#44cc88",
            insertbackground=fg, relief="flat", bd=8,
            state="readonly",
        )
        self.result_entry.pack(side="left", fill="x", expand=True)

        self.copy_btn = tk.Button(
            result_frame, text="Copy", font=("Segoe UI", 10),
            bg="#1a1a26", fg=fg, relief="flat", bd=0,
            padx=15, pady=6, cursor="hand2",
            command=self._copy_result,
        )
        self.copy_btn.pack(side="right", padx=(10, 0))

    def _run_bypass(self):
        url = self.url_var.get().strip()
        if not url:
            return
        if not url.startswith("http"):
            url = "https://" + url
            self.url_var.set(url)

        self.btn.configure(state="disabled", text="...")
        self.status_var.set("Bypassing...")
        self.result_var.set("")

        def worker():
            try:
                result = bypass(url)
                self.root.after(0, lambda: self._show_result(result))
            except Exception as e:
                self.root.after(0, lambda: self._show_error(str(e)))

        threading.Thread(target=worker, daemon=True).start()

    def _show_result(self, result):
        self.btn.configure(state="normal", text="Bypass")
        if result:
            self.result_var.set(result)
            self.status_var.set("Bypassed!")
        else:
            self.result_var.set("")
            self.status_var.set("Could not bypass — link structure may have changed")

    def _show_error(self, error):
        self.btn.configure(state="normal", text="Bypass")
        self.status_var.set(f"Error: {error}")

    def _copy_result(self):
        result = self.result_var.get()
        if result:
            self.root.clipboard_clear()
            self.root.clipboard_append(result)
            self.copy_btn.configure(text="Copied!")
            self.root.after(1500, lambda: self.copy_btn.configure(text="Copy"))

    def run(self):
        self.root.mainloop()


if __name__ == "__main__":
    app = BypassApp()
    app.run()
