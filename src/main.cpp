#include "gui.h"
#include <iostream>

int main() {
    std::cout << "=== PuzzleHunter Main ===" << std::endl;
    
    PuzzleHunterGUI gui;
    
    if (!gui.Initialize()) {
        std::cerr << "Failed to initialize GUI." << std::endl;
        return 1;
    }

    std::cout << "Initialization complete. Entering main loop..." << std::endl;
    gui.Run();
    
    std::cout << "Main loop ended. Shutting down..." << std::endl;
    // ЭТА СТРОКА ГАРАНТИРУЕТ, ЧТО ПОТОКИ БУДУТ КОРРЕКТНО ЗАВЕРШЕНЫ ПЕРЕД ВЫХОДОМ
    gui.Shutdown();

    std::cout << "Exit." << std::endl;
    return 0;
}