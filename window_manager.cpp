#include "window_manager.hpp"

WindowManager::~WindowManager()
{
    for (Window* window : windows)
    {
        delete window;
    }
}

void WindowManager::refresh()
{
    for (int i = 0; i < windows.size(); i++)
    {
        windows.at(i)->refresh();
    }
}
