#pragma once

#include <string>

enum Key : int {
    KEY_EOF   = -1,
    KEY_NONE  = 0,
    KEY_CTRL_C = 3,
    KEY_ENTER = 13,
    KEY_ESC   = 27,
    KEY_LEFT  = 1001,
    KEY_RIGHT = 1002,
    KEY_UP    = 1003,
    KEY_DOWN  = 1004
};

class Terminal {
public:
    Terminal();
    ~Terminal();
    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;

    int readKey();

    static void write(const std::string& text);
};
