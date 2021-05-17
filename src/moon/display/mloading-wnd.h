#ifndef LinuxLoadingScreen
#define LinuxLoadingScreen

#include <string>

namespace MoonWindow {
    void setupLoadingWindow(std::string status);
    void updateMessage(std::string msg);
    void setMaxAddons(int max);
    void addAddonCounter();
    void closeLoadingWindow();
}

#endif