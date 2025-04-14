#include <string>
#include <opencv2/opencv.hpp>

struct LoadSettings {
    public:
        LoadSettings(const std::string& path): loadPath(path) {}

        std::string getPath() const {
            return loadPath;
        }

    private:
        const std::string loadPath;
};

struct SaveSettings {
    public:
        SaveSettings(std::string& path): savePath(path) {}

        std::string getPath() const {
            return savePath;
        }

    private:
        std::string savePath;
};

struct BrightnessSettings {
    public:
        int getBrightnessVal(BrightnessSettings &s) {
            return s.pct;
        }

        bool setBrightnessVal(BrightnessSettings &s, int newVal) {
            if (newVal < -100 || newVal > 100) return false;

            s.pct = newVal;
            return true;
        }
        
    private:
        int pct = 0;
};

struct ContrastSettings {
    public:
        double getValue() const {
            return factor;
        }
    
        bool setValue(double newVal) {
            if (newVal < 0.0 || newVal > 3.0) return false;
            factor = newVal;
            return true;
        }
    
    private:
        double factor = 1.0;
};

struct BlurSettings {
    public:
        int getKernelSize() const {
            return kernelSize;
        }
    
        bool setKernelSize(int k) {
            if (k <= 0 || k % 2 == 0) return false; // must be positive odd
            kernelSize = k;
            return true;
        }
    
    private:
        int kernelSize = 3;
};

struct NoiseSettings {
    public:
        double getAmount() const {
            return amount;
        }
    
        bool setAmount(double a) {
            if (a < 0.0 || a > 1.0) return false;
            amount = a;
            return true;
        }
    
    private:
        double amount = 0.05; // 5% noise by default
};

struct ConvolutionSettings {
    public:
        const cv::Mat& getKernel() const {
            return kernel;
        }
    
        void setKernel(const cv::Mat& k) {
            kernel = k.clone();
        }
    
    private:
        cv::Mat kernel;  // user-defined kernel
};    