#ifndef IMAGE_PROCESSOR_H
#define IMAGE_PROCESSOR_H

#include <opencv2/opencv.hpp>

class ImageProcessor {
public:
    static cv::Mat loadImage(const std::string& path);
    static void saveImage(const cv::Mat& image, const std::string& path);
    static void showImage(const std::string& windowName, const cv::Mat& image);

    static cv::Mat applyBrightness(const cv::Mat& image, int value);
    static cv::Mat applyContrast(const cv::Mat& image, double factor);
    static cv::Mat applyBlur(const cv::Mat& image, int kernelSize);
    static cv::Mat applyEdgeDetection(const cv::Mat& image);
    static cv::Mat applyNoise(const cv::Mat& image, double amount);
    static cv::Mat applyConvolution(const cv::Mat& image, const cv::Mat& kernel);
    static cv::Mat blend(const cv::Mat& img1, const cv::Mat& img2, double alpha);
};

#endif // IMAGE_PROCESSOR_H