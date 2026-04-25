#pragma once
#include <string>
#include <vector>

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
    void draw_sprite(int x, int y, const std::vector<std::string>& lines);
    void draw_box(int x, int y, int w, int h);
    void fill_rect(int x, int y, int w, int h, char c);

private:
    int w_ = 0, h_ = 0;
    std::vector<std::string> buffer_;

    void get_terminal_size();
    void reset_buffer();
};

} // namespace shell_cat
