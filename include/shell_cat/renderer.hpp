#pragma once
#include <string>
#include <vector>

#include "shell_cat/theme.hpp"

namespace shell_cat {

class Renderer {
public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void init();
    void shutdown();

    void clear();
    void present();

    int width() const { return w_; }
    int height() const { return h_; }

    void draw_text(int x, int y, const std::string& text);
    void draw_text(int x, int y, const std::string& text, ColorRole role);
    void draw_text_color(int x, int y, const std::string& text, int color_code);
    void draw_sprite(int x, int y, const std::vector<std::string>& lines);
    void draw_sprite(int x, int y, const std::vector<std::string>& lines, ColorRole role);
    void draw_sprite_color(int x, int y, const std::vector<std::string>& lines, int color_code);
    void draw_box(int x, int y, int w, int h);
    void draw_box(int x, int y, int w, int h, ColorRole role);
    void draw_box_color(int x, int y, int w, int h, int color_code);
    void fill_rect(int x, int y, int w, int h, char c);
    void fill_rect(int x, int y, int w, int h, char c, ColorRole role);
    void fill_rect_color(int x, int y, int w, int h, char c, int color_code);

    void set_theme(const Theme& theme);
    const Theme& theme() const { return theme_; }

private:
    struct Cell {
        char ch = ' ';
        ColorRole role = ColorRole::Default;
        int color = -1;
    };

    int w_ = 0, h_ = 0;
    std::vector<std::vector<Cell>> buffer_;
    Theme theme_;

    void get_terminal_size();
    void reset_buffer();
    int color_code(ColorRole role) const;
};

} // namespace shell_cat
