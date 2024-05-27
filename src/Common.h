#pragma once

#include <string>

struct InitArgs
{
    void* window;
    std::string canvasId;
    uint32_t width;
    uint32_t height;
};