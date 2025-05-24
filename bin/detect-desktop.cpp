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
std::wofstream g_logFile;

// Sliding window for minimize-start events
static std::deque<uint64_t> g_minTimes;
static const uint64_t       MIN_WINDOW_MS = 1000;  // 1 second
// If **any** minimize-start occurs in that window, we treat it as Show Desktop
// (so threshold = 1)
static const size_t         MIN_COUNT = 1;

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------
std::wstring GetTimestamp() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t buf[64];
    swprintf(buf, 64, L"[%04d-%02d-%02d %02d:%02d:%02d] ",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond);
    return buf;
}

void Log(const std::wstring& msg) {
    std::wstring out = GetTimestamp() + msg;
    if (!g_quiet) {
        std::wcout << out << L"\n";
    }
    if (g_logEnabled && g_logFile.is_open()) {
        g_logFile << out << L"\n";
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

// Clean up on Ctrl+C
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
    // drop old timestamps
    while (!g_minTimes.empty() && now - g_minTimes.front() > MIN_WINDOW_MS) {
        g_minTimes.pop_front();
    }
}

void CALLBACK ForegroundChanged(HWINEVENTHOOK, DWORD, HWND hwnd, LONG, LONG, DWORD, DWORD) {
    bool nowDesktop = (hwnd == GetShellWindow());
    if (nowDesktop == g_desktopIsFg) {
        return;  // no state change
    }

    size_t burst = g_minTimes.size();
    // decide the message based on whether any minimize-start fired
    if (nowDesktop) {
        if (burst >= MIN_COUNT) {
            Log(L"*** Desktop is now FOREGROUND (Show Desktop)");
        }
        else {
            Log(L"*** Desktop is now FOREGROUND (shown)");
        }
    }
    else {
        Log(L"*** Desktop is now BACKGROUND (apps shown)");
    }

    g_desktopIsFg = nowDesktop;
    g_minTimes.clear();  // reset for next transition
}

// -----------------------------------------------------------------------------
// Entry point
// -----------------------------------------------------------------------------
int wmain(int argc, wchar_t* argv[]) {
    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::wstring a = argv[i];
        if (a == L"--quiet")      g_quiet = true;
        else if (a == L"--log")   g_logEnabled = true;
        else if (a == L"--help") { ShowHelp(); return 0; }
        else {
            std::wcerr << L"Unknown argument: " << a << L"\n";
            ShowHelp();
            return 1;
        }
    }

    // Open log file if requested
    if (g_logEnabled) {
        g_logFile.open(L"detect-desktop.log", std::ios::app);
        if (!g_logFile) {
            std::wcerr << L"Error: failed to open log file ("
                << GetLastError() << L")\n";
            return 1;
        }
    }

    // Setup Ctrl+C handler
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    // Get initial state
    HWND shell = GetShellWindow();
    if (!shell) {
        std::wcerr << L"Error: could not get desktop handle\n";
        return 1;
    }
    g_desktopIsFg = (GetForegroundWindow() == shell);

    Log(L"Initial state: Desktop is " +
        std::wstring(g_desktopIsFg ? L"FOREGROUND" : L"BACKGROUND"));
    Log(L"Listening for Show Desktop; press Ctrl+C to exit.");

    // Install hooks
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
        std::wcerr << L"Error: SetWinEventHook failed ("
            << GetLastError() << L")\n";
        return 1;
    }

    // Message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup (if ever reached)
    if (g_hMinHook)        UnhookWinEvent(g_hMinHook);
    if (g_hForegroundHook) UnhookWinEvent(g_hForegroundHook);
    if (g_logFile.is_open()) g_logFile.close();

    return 0;
}
