#include "ImageProcessor.h"

cv::Mat ImageProcessor::loadImage(const std::string& path) {
    return cv::imread(path);
}

void ImageProcessor::saveImage(const cv::Mat& image, const std::string& path) {
    cv::imwrite(path, image);
}

void ImageProcessor::showImage(const std::string& windowName, const cv::Mat& image) {
    cv::imshow(windowName, image);
    cv::waitKey(0);
}

cv::Mat ImageProcessor::applyBrightness(const cv::Mat& image, int value) {
    cv::Mat result;
    image.convertTo(result, -1, 1, value);  // alpha = 1, beta = value
    return result;
}

cv::Mat ImageProcessor::applyContrast(const cv::Mat& image, double factor) {
    cv::Mat result;
    image.convertTo(result, -1, factor, 0);  // alpha = factor, beta = 0
    return result;
}

cv::Mat ImageProcessor::applyBlur(const cv::Mat& image, int kernelSize) {
    cv::Mat result;
    kernelSize = (kernelSize / 2) * 2 + 1;
    cv::GaussianBlur(image, result, cv::Size(kernelSize, kernelSize), 0);
    return result;
}

cv::Mat ImageProcessor::blend(const cv::Mat &img1, const cv::Mat &img2, double alpha) {
    cv::Mat res;
    cv::addWeighted(img1, alpha, img2, 1 - alpha, 0, res);
    return res;
}
