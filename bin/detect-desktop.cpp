#include <windows.h>
#include <iostream>
#include <string>
#include <fstream>
#include <deque>

// -----------------------------------------------------------------------------
// Globals and configuration
// -----------------------------------------------------------------------------
HWINEVENTHOOK g_hForegroundHook = nullptr;
HWINEVENTHOOK g_hMinHook = nullptr;

bool           g_desktopIsFg = false;
bool           g_quiet = false;
bool           g_logEnabled = false;
bool           g_lastShowDesktop = false; 

std::wofstream g_logFile;

static std::deque<uint64_t> g_minTimes;
static const uint64_t       MIN_WINDOW_MS = 500;
static const size_t         MIN_COUNT = 1;

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
std::string to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(
        CP_UTF8, 0,
        wstr.c_str(), (int)wstr.size(),
        nullptr, 0,
        nullptr, nullptr
    );
    std::string strTo;
    strTo.resize(size_needed);
    WideCharToMultiByte(
        CP_UTF8, 0,
        wstr.c_str(), (int)wstr.size(),
        &strTo[0], size_needed,
        nullptr, nullptr
    );
    return strTo;
}

std::wstring GetTimestamp() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t buf[64];
    swprintf(buf, 64, L"[%04d-%02d-%02d %02d:%02d:%02d] ",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond);
    return std::wstring(buf);
}

void Log(const std::wstring& msg) {
    std::ios::sync_with_stdio(false);
    std::wstring wline = GetTimestamp() + msg;
    std::string  line = to_utf8(wline + L"\n");

    if (!g_quiet) {
        std::cout << line;
        std::cout.flush();
    }
    if (g_logEnabled && g_logFile.is_open()) {
        g_logFile << wline << L"\n";
        g_logFile.flush();
    }
}

void ShowHelp() {
    std::wcout << L"Usage: detect-desktop.exe [--quiet] [--log] [--help]\n"
        << L"  --quiet   Suppress console output\n"
        << L"  --log     Append output to detect-desktop.log\n"
        << L"  --help    Show this help message\n\n"
        << L"Example:\n"
        << L"  detect-desktop.exe --log\n";
}

BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT) {
        Log(L"Exiting on Ctrl+C...");
        if (g_hForegroundHook) UnhookWinEvent(g_hForegroundHook);
        if (g_hMinHook)        UnhookWinEvent(g_hMinHook);
        if (g_logFile.is_open()) g_logFile.close();
        ExitProcess(0);
    }
    return TRUE;
}

// -----------------------------------------------------------------------------
// Event callbacks
// -----------------------------------------------------------------------------
void CALLBACK MinimizeStarted(HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD) {
    uint64_t now = GetTickCount64();
    g_minTimes.push_back(now);
    while (!g_minTimes.empty() && now - g_minTimes.front() > MIN_WINDOW_MS) {
        g_minTimes.pop_front();
    }
}

void CALLBACK ForegroundChanged(HWINEVENTHOOK, DWORD, HWND hwnd, LONG, LONG, DWORD, DWORD) {
    bool nowDesktop = (hwnd == GetShellWindow());
    if (nowDesktop == g_desktopIsFg) return;

    uint64_t now = GetTickCount64();
    while (!g_minTimes.empty() && now - g_minTimes.front() > MIN_WINDOW_MS) {
        g_minTimes.pop_front();
    }

    size_t burst = g_minTimes.size();

    if (nowDesktop) {
        if (burst >= MIN_COUNT) {
            Log(L"*** Desktop is now FOREGROUND (Show Desktop)");
            g_lastShowDesktop = true;
        }
        else {
            Log(L"*** Desktop is now FOREGROUND (shown)");
            g_lastShowDesktop = false;
        }
    }
    else {
        if (g_lastShowDesktop) {
            Log(L"*** Desktop is now BACKGROUND (apps restored via desktopMode)");
        }
        else {
            Log(L"*** Desktop is now BACKGROUND (apps shown)");
        }
        g_lastShowDesktop = false;
    }

    g_desktopIsFg = nowDesktop;
    g_minTimes.clear();
}

// -----------------------------------------------------------------------------
// Entry point
// -----------------------------------------------------------------------------
int wmain(int argc, wchar_t* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::wstring a = argv[i];
        if (a == L"--quiet")      g_quiet = true;
        else if (a == L"--log")        g_logEnabled = true;
        else if (a == L"--help") { ShowHelp(); return 0; }
        else {
            std::wcerr << L"Unknown argument: " << a << L"\n";
            ShowHelp();
            return 1;
        }
    }

    if (g_logEnabled) {
        g_logFile.open(L"detect-desktop.log", std::ios::app);
        if (!g_logFile) {
            std::wcerr << L"Error: failed to open log file (" << GetLastError() << L")\n";
            return 1;
        }
    }

    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    HWND shell = GetShellWindow();
    if (!shell) {
        std::wcerr << L"Error: could not get desktop handle\n";
        return 1;
    }
    g_desktopIsFg = (GetForegroundWindow() == shell);

    Log(L"Initial state: Desktop is " + std::wstring(g_desktopIsFg ? L"FOREGROUND" : L"BACKGROUND"));
    Log(L"Listening for Show Desktop; press Ctrl+C to exit.");

    g_hMinHook = SetWinEventHook(
        EVENT_SYSTEM_MINIMIZESTART, EVENT_SYSTEM_MINIMIZESTART,
        nullptr, MinimizeStarted,
        0, 0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
    );
    g_hForegroundHook = SetWinEventHook(
        EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
        nullptr, ForegroundChanged,
        0, 0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
    );

    if (!g_hMinHook || !g_hForegroundHook) {
        std::wcerr << L"Error: SetWinEventHook failed (" << GetLastError() << L")\n";
        return 1;
    }

    g_minTimes.clear();

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    if (g_hMinHook)        UnhookWinEvent(g_hMinHook);
    if (g_hForegroundHook) UnhookWinEvent(g_hForegroundHook);
    if (g_logFile.is_open()) g_logFile.close();

    return 0;
}
