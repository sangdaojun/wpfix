🛠️ wpfix (Windows Path Fixer)
wpfix is a high-performance, zero-dependency CLI tool written in pure C, designed to manage and repair the Windows PATH environment variable with surgical precision.

Unlike the standard setx command, wpfix ensures your PATH remains clean, deduplicated, and instantly synchronized across all active terminal windows without requiring a system reboot.

✨ Key Features
Zero Dependencies: A tiny standalone executable. No .NET, Python, or PowerShell overhead.

Surgeon Mode (Full System Scan): Scans both User and System paths.

🟢 Green: Valid User paths.

🔵 Cyan: Valid System paths.

⚪ White: Invalid/Broken paths (Optimized for dark-themed consoles).

Active Sync: Uses Windows Message Broadcasting and Remote Process Injection to update the %PATH% of all currently open cmd.exe windows in real-time.

Smart Sanitization: Automatically handles duplicate entries, trailing backslashes, and messy double semicolons (;;).

Robust Parsing: High tolerance for user input (e.g., handles wpfix add. or wpfix list\ gracefully).

🚀 Installation & Build
Since wpfix is written in pure C, you can compile it using the Tiny C Compiler (TCC) or any standard MinGW/MSVC compiler.

Build with TCC:
Bash

tcc wpfix.c -luser32 -ladvapi32 -lshell32 -o wpfix.exe
💻 Usage Guide
1. Adding a Path
Add the current directory or a specific path. It automatically removes existing duplicates before adding.

Bash

wpfix add .
wpfix add C:\Tools\my_bin
2. Removing a Path
Removes a specific directory from the User PATH registry.

Bash

wpfix del C:\Old\Path
3. Surgeon Mode (Diagnostics)
The "Surgeon" command performs a deep health check of your entire environment.

Bash

wpfix surgeon
Identifies broken links (folders that no longer exist) in White.

4. Viewing Paths
Show: Lists paths currently loaded in the active window's memory.

List: Lists paths saved in the Windows Registry (User).

Bash

wpfix show
wpfix list
🔧 Technical Mechanism
wpfix operates on three levels:

Registry Level: Modifies HKEY_CURRENT_USER\Environment directly for persistence.

Broadcast Level: Issues WM_SETTINGCHANGE to notify the OS of environment changes.

Process Level: Injects a temporary synchronization script into all cmd.exe pids to update their local process environment variables immediately.

📜 License
This project is licensed under the MIT License.