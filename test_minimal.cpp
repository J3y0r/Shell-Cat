#include <iostream>
#include "shell_cat/renderer.hpp"

int main() {
    std::cerr << "Before Renderer\n";
    shell_cat::Renderer renderer;
    std::cerr << "After Renderer init\n";
    renderer.clear();
    std::cerr << "After clear\n";
    renderer.present();
    std::cerr << "After present\n";
    return 0;
}
