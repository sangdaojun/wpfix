#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <tlhelp32.h>
#include <shellapi.h>

#define MAX_ENV_BUFFER 32767
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

// --- 模拟键盘输入注入 ---
void SendStringSecure(HWND hwnd, const char* str) {
    for (int i = 0; i < (int)strlen(str); i++) {
        PostMessage(hwnd, WM_CHAR, str[i], 0);
        Sleep(15); 
    }
}

// --- 暴力同步每个 CMD 窗口 ---
void force_sync_one(DWORD pid, const char* sync_bat_path) {
    if (pid == GetCurrentProcessId()) return;
    if (FreeConsole()) { 
        if (AttachConsole(pid)) {
            HWND hwnd = GetConsoleWindow();
            if (hwnd) {
                PostMessage(hwnd, WM_CHAR, '\r', 0);
                Sleep(50);
                char cmd[MAX_PATH + 50];
                sprintf(cmd, "\"%s\"\r\n", sync_bat_path);
                SendStringSecure(hwnd, cmd);
            }
            FreeConsole();
        }
    }
}

// --- 外科医生扫描逻辑 ---
void run_surgery(HKEY root, const char* subkey, const char* title) {
    HKEY hKey;
    char buffer[MAX_ENV_BUFFER] = {0};
    DWORD d_sz = MAX_ENV_BUFFER;
    printf("\n\033[33m=== %s SURGERY ===\033[0m\n", title);
    if (RegOpenKeyExA(root, subkey, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        printf("\033[31m[!] Access Denied. (Need Admin for System Path)\033[0m\n");
        return;
    }
    RegQueryValueExA(hKey, "Path", NULL, NULL, (LPBYTE)buffer, &d_sz);
    char* buf_copy = strdup(buffer);
    char* tok = strtok(buf_copy, ";");
    while (tok) {
        if (strlen(tok) > 0) {
            DWORD attr = GetFileAttributesA(tok);
            if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY))
                printf("\033[37m[!!] INVALID: %s\033[0m\n", tok);
            else {
                const char* color = (root == HKEY_LOCAL_MACHINE) ? "\033[36m" : "\033[32m";
                printf("%s[OK] VALID:   %s\033[0m\n", color, tok);
            }
        }
        tok = strtok(NULL, ";");
    }
    free(buf_copy); RegCloseKey(hKey);
}
int main(int argc, char* argv[]) {
    // 初始化 ANSI 颜色支持
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
	unsigned int originalCP;

	originalCP=GetConsoleOutputCP();

    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    if (argc < 2) {
        printf("Usage: pm <add|del|list|show|surgeon> [path]\n");
        return 0;
    }
   SetConsoleOutputCP(65001);
   SetConsoleCP(65001);
    int cmd_type = 0; // 1:add, 2:del, 3:list, 4:surgeon, 5:show
    char target[MAX_PATH] = {0};

    // --- 严谨的参数识别 ---
    if (strnicmp(argv[1], "add", 3) == 0) cmd_type = 1;
    else if (strnicmp(argv[1], "del", 3) == 0) cmd_type = 2;
    else if (strnicmp(argv[1], "list", 4) == 0) cmd_type = 3;
    else if (strnicmp(argv[1], "surgeon", 7) == 0) cmd_type = 4;
    else if (strnicmp(argv[1], "show", 4) == 0) cmd_type = 5;

    // 解析路径（处理连写如 add.）
    if (cmd_type == 1 || cmd_type == 2) {
        if (strlen(argv[1]) > 3) strcpy(target, argv[1] + 3);
        else if (argc >= 3) strcpy(target, argv[2]);
        if (strlen(target) == 0 || strcmp(target, ".") == 0) GetCurrentDirectoryA(MAX_PATH, target);
    }

    // 功能 5：Show (当前窗口内存状态)
    if (cmd_type == 5) {
        char* p = getenv("PATH");
        printf("\n--- CURRENT WINDOW PATH (Memory) ---\n");
        if(!p)	return 0;
        char* buf = strdup(p);
        char* tok = strtok(buf, ";");
        int cnt = 1;
        while (tok) { printf("[%02d] %s\n", cnt++, tok); tok = strtok(NULL, ";"); }
        free(buf); return 0;
    }

    // 功能 4：Surgeon (全域扫描)
    if (cmd_type == 4) {
        run_surgery(HKEY_CURRENT_USER, "Environment", "USER PATH");
        run_surgery(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment", "SYSTEM PATH");
        SetConsoleOutputCP(originalCP);
		SetConsoleCP(originalCP);
		return 0;
    }

    // --- 注册表操作准备 ---
    HKEY hKey;
    char current_usr[MAX_ENV_BUFFER] = {0}, final_usr[MAX_ENV_BUFFER] = {0};
    DWORD d_sz = MAX_ENV_BUFFER;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS) return 1;
    RegQueryValueExA(hKey, "Path", NULL, NULL, (LPBYTE)current_usr, &d_sz);

    // 功能 3：List (注册表用户路径)
    if (cmd_type == 3) {
        printf("\n--- REGISTRY USER PATH ---\n");
        char* buf = strdup(current_usr);
        char* tok = strtok(buf, ";");
        int cnt = 1;
        while (tok) { printf("[%02d] %s\n", cnt++, tok); tok = strtok(NULL, ";"); }
        free(buf); RegCloseKey(hKey); return 0;
    }

    // --- Add/Del 核心去重逻辑 ---
    char temp[MAX_ENV_BUFFER]; strcpy(temp, current_usr);
    char* tok = strtok(temp, ";");
    while (tok) {
        if (stricmp(tok, target) != 0 && strlen(tok) > 0) {
            if (strlen(final_usr) > 0) strcat(final_usr, ";");
            strcat(final_usr, tok);
        }
        tok = strtok(NULL, ";");
    }
    if (cmd_type == 1) {
        if (strlen(final_usr) > 0) strcat(final_usr, ";");
        strcat(final_usr, target);
    }
    RegSetValueExA(hKey, "Path", 0, REG_EXPAND_SZ, (LPBYTE)final_usr, (DWORD)strlen(final_usr) + 1);
    RegCloseKey(hKey);

    // --- 智能同步脚本生成 (当前目录) ---
    char sync_bat[MAX_PATH], work_dir[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, work_dir);
    sprintf(sync_bat, "%s\\pm_sync.bat", work_dir);
    FILE *f = fopen(sync_bat, "w");
    if (f) {
        fprintf(f, "@set \"_P=%%PATH%%\"\n");
        fprintf(f, "@set \"_P=%%_P:;%s=%%\"\n", target);
        fprintf(f, "@set \"_P=%%_P:%s;=%%\"\n", target);
        fprintf(f, "@set \"_P=%%_P:%s=%%\"\n", target);
        fprintf(f, "@set \"_P=%%_P:;;=;%%\"\n");
        if (cmd_type == 1) 
            fprintf(f, "@if \"%%_P:~-1%%\"==\";\" (set \"PATH=%%_P%%%s\") else (set \"PATH=%%_P%%;%s\")\n", target, target);
        else 
            fprintf(f, "@set \"PATH=%%_P%%\"\n");
        fprintf(f, "@set \"_P=\"\n@echo [pm] Sync Success.\n");
        fclose(f);
    }

    // 广播通知 + 暴力注入
    SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)"Environment", SMTO_ABORTIFHUNG, 100, NULL);
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe = {sizeof(pe)};
    if (Process32First(hSnap, &pe)) {
        do {
            if (stricmp(pe.szExeFile, "cmd.exe") == 0) force_sync_one(pe.th32ProcessID, sync_bat);
        } while (Process32Next(hSnap, &pe));
    }
    CloseHandle(hSnap);

    printf("\033[32m[Done]\033[0m Action completed. Syncing windows...\n");
    SetConsoleOutputCP(originalCP);
	SetConsoleCP(originalCP);
	return 0;
}