// nodes.h
#pragma once

#include <string>
#include <optional>
#include <opencv2/opencv.hpp>
#include "imgui.h"
#include <GLFW/glfw3.h>

enum class OperationType {
    Blur,
    Brightness,
    LoadImage,
    ProcessDisplay
};

struct Node {
    int id;
    OperationType type;
    std::string name;
    ImVec2 position;
    int inputSlotId = -1;
    int outputSlotId = -1;
    float width = 150.0f;

    // Parameters for adjustable operations
    std::optional<float> value;

    // For LoadImage node
    std::optional<std::string> imagePath;

    // Shared image data and OpenGL texture info
    std::optional<cv::Mat> loadedCvImage;
    GLuint textureId = 0;
    int imageWidth = 0;
    int imageHeight = 0;

    // Processing flags
    bool processingRequested = false;
};

struct Link {
    int id;
    int fromSlot;
    int toSlot;
};
