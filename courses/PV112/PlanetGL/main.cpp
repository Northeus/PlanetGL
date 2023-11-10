#define GLFW_INCLUDE_NONE

#include <string>
#include <vector>
#include "application.hpp"
#include "gui_manager.h"

int main(int argc, char** argv) {
    int initial_width = 1280;
    int initial_height = 720;

    std::vector<std::string> arguments(argv, argv + argc);

    ImGuiManager manager;
    manager.init(initial_width, initial_height, "PV112 Template", 4, 5);
    if(!manager.is_fail())
    {
        Application application(initial_width, initial_height, arguments);
        manager.run(application);
    }

    manager.terminate();
    return 0;
}
