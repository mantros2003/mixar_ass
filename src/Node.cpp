#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

enum class NodeType {
    Load,
    Save,
    Brightness,
    Contrast,
    Blur,
    EdgeDetection,
    Noise,
    Convolution,
    Blend
};

class Node {
    public:
    private:
        NodeType operType;
        bool imageLoaded;
        cv::Mat *img;
};