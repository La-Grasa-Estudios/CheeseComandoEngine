#pragma once
#include <vector>
#include <SDL.h>

struct Animation
{
    std::vector<SDL_Rect> frames;
    float currentFrameTime = 0.0f;
    int currentFrame = 0;
    float frameRate = 1.0f;
    bool loop = true;
    bool isPlaying = false;
};