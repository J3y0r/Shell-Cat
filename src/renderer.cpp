#include "shell_cat/renderer.hpp"
#include <algorithm>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace shell_cat {

Renderer::Renderer() {
    init();
}

Renderer::~Renderer() {
    shutdown();
}

void Renderer::init() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO ci;
    GetConsoleCursorInfo(hOut, &ci);
    ci.bVisible = FALSE;
    SetConsoleCursorInfo(hOut, &ci);
#else
    std::cout << "\033[?25l";
#endif
    get_terminal_size();
    reset_buffer();
}

void Renderer::shutdown() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO ci;
    GetConsoleCursorInfo(hOut, &ci);
    ci.bVisible = TRUE;
    SetConsoleCursorInfo(hOut, &ci);
#else
    std::cout << "\033[?25h";
#endif
}

void Renderer::get_terminal_size() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
        w_ = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        h_ = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    } else {
        w_ = 80; h_ = 25;
    }
#else
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        w_ = ws.ws_col;
        h_ = ws.ws_row;
    } else {
        w_ = 80; h_ = 25;
    }
#endif
    if (w_ < 1) w_ = 80;
    if (h_ < 1) h_ = 25;
}

void Renderer::reset_buffer() {
    buffer_.resize(h_);
    for (int y = 0; y < h_; ++y) {
        buffer_[y] = std::string(w_, ' ');
    }
}

void Renderer::clear() {
    get_terminal_size();
    reset_buffer();

#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coord = {0, 0};
    DWORD count;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hOut, &csbi);
    DWORD cells = csbi.dwSize.X * csbi.dwSize.Y;
    FillConsoleOutputCharacter(hOut, ' ', cells, coord, &count);
    SetConsoleCursorPosition(hOut, coord);
#else
    std::cout << "\033[2J\033[H";
#endif
}

void Renderer::draw_text(int x, int y, const std::string& text) {
    if (y < 0 || y >= h_) return;
    int start = std::max(0, x);
    int end = std::min(w_, x + static_cast<int>(text.size()));
    for (int i = start; i < end; ++i) {
        int ti = i - x;
        if (ti >= 0 && ti < static_cast<int>(text.size())) {
            buffer_[y][i] = text[ti];
        }
    }
}

void Renderer::draw_sprite(int x, int y, const std::vector<std::string>& lines) {
    for (size_t i = 0; i < lines.size(); ++i) {
        int py = y + static_cast<int>(i);
        if (py < 0 || py >= h_) continue;
        const std::string& line = lines[i];
        int start = std::max(0, x);
        int end = std::min(w_, x + static_cast<int>(line.size()));
        for (int j = start; j < end; ++j) {
            int tj = j - x;
            if (tj >= 0 && tj < static_cast<int>(line.size())) {
                char c = line[tj];
                if (c != ' ') {
                    buffer_[py][j] = c;
                }
            }
        }
    }
}

void Renderer::draw_box(int x, int y, int w, int h) {
    if (w < 2 || h < 2) return;
    int x1 = x, y1 = y;
    int x2 = x + w - 1, y2 = y + h - 1;

    for (int iy = y1; iy <= y2; ++iy) {
        if (iy < 0 || iy >= h_) continue;
        for (int ix = x1; ix <= x2; ++ix) {
            if (ix < 0 || ix >= w_) continue;
            char c = ' ';
            if (ix == x1 || ix == x2) {
                c = (iy == y1 || iy == y2) ? '+' : '|';
            } else if (iy == y1 || iy == y2) {
                c = '-';
            }
            buffer_[iy][ix] = c;
        }
    }
}

void Renderer::fill_rect(int x, int y, int w, int h, char c) {
    for (int iy = y; iy < y + h; ++iy) {
        if (iy < 0 || iy >= h_) continue;
        for (int ix = x; ix < x + w; ++ix) {
            if (ix < 0 || ix >= w_) continue;
            buffer_[iy][ix] = c;
        }
    }
}

void Renderer::present() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coord = {0, 0};
    SetConsoleCursorPosition(hOut, coord);
#else
    std::cout << "\033[H";
#endif

    for (int y = 0; y < h_; ++y) {
        std::cout << buffer_[y];
        if (y < h_ - 1) std::cout << '\n';
    }
    std::cout << std::flush;
}

} // namespace shell_cat
