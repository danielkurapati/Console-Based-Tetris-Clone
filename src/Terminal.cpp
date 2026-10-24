#include "Terminal.h"

#include <cstdio>

#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #define WIN32_LEAN_AND_MEAN
    #include <conio.h>
    #include <windows.h>
#else
    #include <sys/select.h>
    #include <termios.h>
    #include <unistd.h>
#endif

namespace {
#ifdef _WIN32
DWORD g_savedOutMode = 0;
bool g_haveOutMode = false;
#else
termios g_savedTermios;
bool g_haveTermios = false;

bool inputReady() {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    return select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0;
}
#endif
}

Terminal::Terminal() {
#ifdef _WIN32
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(out, &mode)) {
        g_savedOutMode = mode;
        g_haveOutMode = true;
        SetConsoleMode(out, mode | 0x0004);   // ENABLE_VIRTUAL_TERMINAL_PROCESSING
    }
#else
    if (tcgetattr(STDIN_FILENO, &g_savedTermios) == 0) {
        g_haveTermios = true;
        termios raw = g_savedTermios;
        raw.c_lflag &= ~static_cast<tcflag_t>(ICANON | ECHO | ISIG);  // Ctrl+C arrives as a key
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    }
#endif
    // alternate screen, clear, home, hide cursor
    write("\x1b[?1049h\x1b[2J\x1b[H\x1b[?25l");
}

Terminal::~Terminal() {
    // reset colors, show cursor, leave alternate screen
    write("\x1b[0m\x1b[?25h\x1b[?1049l");
#ifdef _WIN32
    if (g_haveOutMode) SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), g_savedOutMode);
#else
    if (g_haveTermios) tcsetattr(STDIN_FILENO, TCSANOW, &g_savedTermios);
#endif
}

void Terminal::write(const std::string& text) {
    std::fwrite(text.data(), 1, text.size(), stdout);
    std::fflush(stdout);
}

#ifdef _WIN32

int Terminal::readKey() {
    if (!_kbhit()) return KEY_NONE;
    int c = _getch();
    if (c == 0 || c == 0xE0) {
        int c2 = _getch();
        switch (c2) {
            case 72: return KEY_UP;
            case 80: return KEY_DOWN;
            case 75: return KEY_LEFT;
            case 77: return KEY_RIGHT;
            default: return KEY_NONE;
        }
    }
    return c;
}

#else

int Terminal::readKey() {
    if (!inputReady()) return KEY_NONE;
    unsigned char c = 0;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    if (n == 0) return KEY_EOF;
    if (n < 0) return KEY_NONE;

    if (c == 27) {                       // ESC, or the start of an arrow-key sequence
        if (inputReady()) {
            unsigned char s1 = 0;
            if (read(STDIN_FILENO, &s1, 1) == 1 && s1 == '[' && inputReady()) {
                unsigned char s2 = 0;
                if (read(STDIN_FILENO, &s2, 1) == 1) {
                    switch (s2) {
                        case 'A': return KEY_UP;
                        case 'B': return KEY_DOWN;
                        case 'C': return KEY_RIGHT;
                        case 'D': return KEY_LEFT;
                        default: break;
                    }
                }
            }
            return KEY_NONE;
        }
        return KEY_ESC;
    }
    return c;
}

#endif
