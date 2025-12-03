#include "framework.h"
#include "Quick.h"
 
#include <shellapi.h>
#include <filesystem>
#include <functional>
#include <winuser.h>
#include <locale>
#include <shlobj.h>
#include <shlwapi.h>
#include <string>
#include <windowsx.h>
#include <windows.h>
#include <iostream>
#include <comdef.h>
#include <queue>
#include <mutex>
#include <atomic>
#include <memory>
#include <algorithm>
#include <thread>
#include <chrono>
#include <vector>
 

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/cuda.hpp>
#include <opencv2/core/ocl.hpp>

// CUDA-specific headers (if available in your OpenCV build)
 #include <opencv2/cudawarping.hpp>
 #include <opencv2/cudaimgproc.hpp>
 #include <opencv2/cudafilters.hpp>
#include <opencv2/cudaarithm.hpp>
#include <opencv2/world.hpp>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/highgui.hpp>


#define SHCHG_ASSOCCHANGED 0x08000000
 

#pragma comment(lib, "opencv_world4130.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "msimg32.lib")

/////////////make file folder go dark mode
#pragma comment(linker, "/manifestdependency:\"\n\
type='win32' \n\
name='Microsoft.Windows.Common-Controls' \n\
version='6.0.0.0' \n\
processorArchitecture='*' \n\
publicKeyToken='6595b64144ccf1df' \n\
language='*' \n\
\"")
////////////////////////////////////////////

#define MAX_LOADSTRING 100

// IDs for controls
#define ID_BTN_EXIT 1001
#define ID_BTN_FOLDER 1002
#define ID_CHK_RESIZE 2001
#define ID_CHK_COLOR 2002
#define ID_CHK_DENOISE 2003

// Global Variables:
HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

// Global output folder
WCHAR g_outputFolder[MAX_PATH] = L"";
 


// Icon positions and sizes
#define ICON_SIZE 32
#define ICON_MARGIN 20


constexpr int WINDOW_WIDTH = 900;
constexpr int WINDOW_HEIGHT = 550;

const int PANEL_Y = 130;

// GPU Toggle - right side, below format buttons
constexpr int GPU_TOGGLE_X = 650;
constexpr int GPU_TOGGLE_Y = PANEL_Y + 28;
constexpr int GPU_TOGGLE_W = 150;   // matches text width
constexpr int GPU_TOGGLE_H = 30;

const int FMT_BTN_W = 60;
const int FMT_BTN_H = 30;
// Folder and Exit icons - aligned to the right
constexpr int FOLDER_ICON_X = WINDOW_WIDTH - ICON_MARGIN - ICON_SIZE * 2 - 10;  // 2 icons + spacing
constexpr int EXIT_ICON_X = WINDOW_WIDTH - ICON_MARGIN - ICON_SIZE;
bool g_hoverClose = false;

const wchar_t* formats[] = { L"JPG", L"PNG", L"WEBP", L"TIF", L"BMP", L"GIF" };
const int FMT_X = 380;
const int FMT_Y = PANEL_Y + 25;
const int FMT_X2 = 380;
const int FMT_Y2 = PANEL_Y + 25; 
const int FMT_ROW2_Y = FMT_Y2 + FMT_BTN_H + 10;


// Result overlay data — stored when conversion finishes
 size_t   g_resultImageCount = 0;
 int      g_resultFailedCount = 0;
 double   g_resultDuration = 0.0;
 bool     g_resultGpuUsed = false;
 bool     g_showResult = false;
  clock_t  g_resultTime = 0;
 clock_t RESULT_DURATION = 5000; // 5 seconds

std::atomic<int> g_lastFailedCount = 0;
  bool g_hoverOutputFolder = false;  // Add this with your other hover vars


  constexpr int MINIMIZE_ICON_X = 7;
  constexpr int MINIMIZE_ICON_Y = 7;
  constexpr int MINIMIZE_ICON_SIZE = 36;
  bool g_hoverMinimize = false;  // Add with your other ho 
   HICON g_hAppIcon = nullptr;
 
  clock_t g_minimizeAnimTime = 0;
  int     g_minimizeAnimFrame = 0; // 0=/  1=-  2=\  3=|
  constexpr int MINIMIZE_ANIM_INTERVAL = 120;  // ms per frame

  bool g_hoverTitle = false;
  bool g_clickTitle = false;
  bool g_inAnimeMode = false;
  bool web = false;


  // ============================================
//   Add progress tracking globals
// ============================================
  std::atomic<bool> g_isConverting(false);
  std::atomic<int> g_conversionProgress(0);
  std::atomic<int> g_conversionTotal(0);
  std::wstring g_conversionStatus = L"";
  std::mutex g_statusMutex;
  clock_t g_progressAnimTime = 0;
  int g_progressAnimOffset = 0;
  void DrawGradientRect(HDC hdc, int x, int y, int w, int h,
      COLORREF color1, COLORREF color2, bool horizontal );

 
  void PlayGodTierAnimeEnding(HDC hdc, HWND hWnd);
  


enum class OutputFormat {
    JPG,
    PNG,
    WEBP,
    TIF,
    BMP,
    GIF  // Add these new formats
};

// Application State
struct AppState {
    int quality = 95;
    OutputFormat format = OutputFormat::JPG;
    bool useGpu = true;
    std::wstring outputFolder;
    bool isDraggingSlider = false;

    bool doResize = false;
    bool doColorConvert = false;
    bool doDenoise = false;
    
    int resizePercent = 100;
    
    // Color conversion options
    enum ColorMode {
        AUTO_DETECT = 0,
        BGR_3CHANNEL = 1,
        GRAYSCALE = 2,
        RGBA_4CHANNEL = 3
    };
    ColorMode colorMode = AUTO_DETECT;
    
    // Denoise options
    int denoiseStrength = 3;  // 1-10 scale
    int denoiseTemplateWindowSize = 7;
    int denoiseSearchWindowSize = 21;
    
    std::wstring resizeInput = L"100";
    bool resizeInputActive = false;
};

AppState g_state ;
HINSTANCE g_hInst = nullptr;
HWND g_hWnd = nullptr;

// Forward declarations
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void SelectOutputFolder(HWND hwndOwner);
void EnableDarkMode();
void InitializePathsAndFolders();
//void ProcessFiles(HWND hWnd, const std::vector<std::wstring>& paths);
std::string getAvailableGpuBackend();
void DrawRoundedRect(HDC hdc, int x, int y, int w, int h, int r, COLORREF color);

 

/////////////////////////////////////////////////////////////////////////////////
// GPU Device Manager (detects CUDA or OpenCL)
////////////////////////////////////////////////////////////////////////////////
class GPUDevice {
private:
    bool available;
    bool useCuda;
    std::string deviceName;

public:
    GPUDevice() : available(false), useCuda(false), deviceName("") {
        // Try CUDA first
        try {
            int cudaCount = 0;
            try { cudaCount = cv::cuda::getCudaEnabledDeviceCount(); }
            catch (...) { cudaCount = 0; }
            if (cudaCount > 0) {
                try {
                    cv::cuda::DeviceInfo di(0);
                    if (di.isCompatible()) {
                        useCuda = true;
                        available = true;
                        deviceName = di.name();
                        cv::cuda::setDevice(0);
                        std::cout << "GPU: CUDA device detected: " << deviceName << std::endl;
                        return;
                    }
                }
                catch (...) {}
            }
        }
        catch (...) {}

        // Try OpenCL (UMat)
        try {
            if (cv::ocl::haveOpenCL()) {
                try {
                    cv::ocl::Context ctx = cv::ocl::Context::getDefault();
                    if (ctx.ndevices() > 0) {
                        available = true;
                        cv::ocl::Device dev = ctx.device(0);
                        deviceName = dev.name();
                        useCuda = false;
                        cv::ocl::setUseOpenCL(true);
                        std::cout << "GPU: OpenCL device detected: " << deviceName << std::endl;
                        return;
                    }
                }
                catch (...) {
                    try {
                        cv::ocl::Context ctx;
                        if (ctx.create(cv::ocl::Device::TYPE_ALL) && ctx.ndevices() > 0) {
                            available = true;
                            cv::ocl::Device dev = ctx.device(0);
                            deviceName = dev.name();
                            useCuda = false;
                            cv::ocl::setUseOpenCL(true);
                            std::cout << "GPU: OpenCL device detected (manual ctx): " << deviceName << std::endl;
                            return;
                        }
                    }
                    catch (...) {}
                }
            }
        }
        catch (...) {}

        std::cout << "GPU: No GPU backend available." << std::endl;
    }

    bool isAvailable() const { return available; }
    bool isCuda() const { return useCuda; }
    std::string name() const { return deviceName; }
};

/////////////////////////////////////////////////////////////////////////////////
// ThreadPool 
////////////////////////////////////////////////////////////////////////////////
class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop;

public:
    ThreadPool(size_t threads) : stop(false) {
        workers.reserve(threads);
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this] {
                            return stop || !tasks.empty();
                            });

                        if (stop && tasks.empty()) return;
                        if (tasks.empty()) continue;

                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    try {
                        task();
                    }
                    catch (...) {}
                }
                });
        }
    }

    template<class F>
    void enqueue(F&& f) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.emplace(std::forward<F>(f));
        }
        condition.notify_one();
    }

    void wait() {
        while (true) {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (tasks.empty()) break;
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    ~ThreadPool() {
        stop = true;
        condition.notify_all();
        for (auto& worker : workers) {
            if (worker.joinable()) worker.join();
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////
// ImageConverter with GPU pipeline
////////////////////////////////////////////////////////////////////////////////
struct ConversionConfig {
    int quality;
    int threadCount;
    bool useGpu;
    ConversionConfig(int q = 95, bool gpu = false) : quality(q), useGpu(gpu) {
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        threadCount = std::max(1, static_cast<int>(sysinfo.dwNumberOfProcessors));
        //if (useGpu) threadCount = std::max(1, threadCount / 2);
       //if (useGpu)  threadCount = std::max(8, threadCount);
    }
};

class ImageConverter {
private:
    ConversionConfig config;
    GPUDevice gpu_device;

    static constexpr std::string_view supported_formats[] = {
        ".png", ".jpg", ".jpeg", ".bmp", ".gif", ".tiff",
        ".tif", ".webp", ".ico", ".psd", ".jp2", ".j2k",
        ".pbm", ".pgm", ".ppm", ".sr", ".ras", ".hdr",
        ".exr", ".dib", ".jpe", ".jfif", ".pic", ".pxm",
        ".pnm", ".pfm"
    };

    bool isSupportedFormat(std::string_view extension) {
        std::string ext_lower;
        ext_lower.reserve(extension.size());
        std::transform(extension.begin(), extension.end(),
            std::back_inserter(ext_lower), ::tolower);

        for (auto format : supported_formats) {
            if (ext_lower == format) return true;
        }
        return false;
    }

     


    void scanDirectory(const std::wstring& dirPath, std::vector<std::wstring>& imageFiles) {
        std::wstring searchPath = dirPath + L"\\*.*";
        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
        if (hFind == INVALID_HANDLE_VALUE) return;

        do {
            if (wcscmp(findData.cFileName, L".") == 0 ||
                wcscmp(findData.cFileName, L"..") == 0) continue;

            std::wstring fullPath = dirPath + L"\\" + findData.cFileName;

            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                scanDirectory(fullPath, imageFiles);
            }
            else {
                std::wstring ext = PathFindExtensionW(findData.cFileName);
                if (!ext.empty()) {
                    // Convert to lowercase for comparison
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);

                    // Convert wide string to UTF-8 for isSupportedFormat check
                    int size_needed = WideCharToMultiByte(CP_UTF8, 0, ext.c_str(), -1,
                        nullptr, 0, nullptr, nullptr);
                    std::string extA(size_needed - 1, 0); // -1 to exclude null terminator
                    WideCharToMultiByte(CP_UTF8, 0, ext.c_str(), -1,
                        extA.data(), size_needed, nullptr, nullptr);

                    if (isSupportedFormat(extA)) {
                        imageFiles.push_back(std::move(fullPath));
                    }
                }
            }
        } while (FindNextFileW(hFind, &findData));
        FindClose(hFind);
    }




    std::vector<std::wstring> getAllImageFiles(const std::wstring& path) {
        std::vector<std::wstring> imageFiles;
        DWORD attribs = GetFileAttributesW(path.c_str());
        if (attribs == INVALID_FILE_ATTRIBUTES) return imageFiles;
        if (attribs & FILE_ATTRIBUTE_DIRECTORY) scanDirectory(path, imageFiles);
        else {
            std::wstring ext = PathFindExtensionW(path.c_str());
            if (!ext.empty() && isSupportedFormat(std::string(ext.begin(), ext.end()))) imageFiles.push_back(path);
        }
        return imageFiles;
    }

 


    std::wstring getOutputPath(const std::wstring& inputPath, const std::wstring& outputDir, const std::wstring& formatExt) {
        wchar_t drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME];
        _wsplitpath_s(inputPath.c_str(),
            drive, _MAX_DRIVE,
            dir, _MAX_DIR,
            fname, _MAX_FNAME,
            nullptr, 0); // no extension wanted

        // Ensure output directory has trailing backslash
        std::wstring outputDirFixed = outputDir;
        if (!outputDirFixed.empty() && outputDirFixed.back() != L'\\') {
            outputDirFixed += L'\\';
        }

        // Check if file already exists, add counter if it does
        std::wstring basePath = outputDirFixed + fname + formatExt;
        std::wstring finalPath = basePath;

        int counter = 1;
        while (PathFileExistsW(finalPath.c_str()) && counter < 1000) {
            finalPath = outputDirFixed + fname + L"_" + std::to_wstring(counter) + formatExt;
            counter++;
        }

        return finalPath;
    }




    bool convertImageCPU(const std::string& inputPathA, const std::string& outputPathA,
        const std::vector<int>& compression_params,
        const AppState& state)
    {
        try {
            cv::Mat image = cv::imread(inputPathA, cv::IMREAD_UNCHANGED);
            if (image.empty()) return false;

            // CPU pipeline
            cv::Mat proc = image;

            if (state.doResize && state.resizePercent != 100) {
                double scale = state.resizePercent / 100.0;
                cv::Size s((int)(proc.cols * scale), (int)(proc.rows * scale));
                if (s.width > 0 && s.height > 0) cv::resize(proc, proc, s, 0, 0, cv::INTER_LINEAR);
            }

            if (state.doColorConvert) {
                // NEW COLOR CONVERSION LOGIC
                switch (state.colorMode) {
                case AppState::AUTO_DETECT:
                    // Keep as is - no conversion
                    break;
                case AppState::BGR_3CHANNEL:
                    if (proc.channels() == 1) cv::cvtColor(proc, proc, cv::COLOR_GRAY2BGR);
                    else if (proc.channels() == 4) cv::cvtColor(proc, proc, cv::COLOR_BGRA2BGR);
                    break;
                case AppState::GRAYSCALE:
                    if (proc.channels() == 3) cv::cvtColor(proc, proc, cv::COLOR_BGR2GRAY);
                    else if (proc.channels() == 4) cv::cvtColor(proc, proc, cv::COLOR_BGRA2GRAY);
                    break;
                case AppState::RGBA_4CHANNEL:
                    if (proc.channels() == 1) {
                        cv::cvtColor(proc, proc, cv::COLOR_GRAY2BGRA);
                    }
                    else if (proc.channels() == 3) {
                        cv::cvtColor(proc, proc, cv::COLOR_BGR2BGRA);
                    }
                    break;
                }
            }

            if (state.doDenoise) {
                // NEW DENOISE WITH STRENGTH CONTROL
                int h = state.denoiseStrength;
                int templateWindowSize = state.denoiseTemplateWindowSize;
                int searchWindowSize = state.denoiseSearchWindowSize;

                if (proc.channels() >= 3) {
                    cv::Mat tmp;
                    cv::fastNlMeansDenoisingColored(proc, tmp, h, h,
                        templateWindowSize, searchWindowSize);
                    proc = tmp;
                }
                else {
                    cv::Mat tmp;
                    cv::fastNlMeansDenoising(proc, tmp, h,
                        templateWindowSize, searchWindowSize);
                    proc = tmp;
                }
            }

            return cv::imwrite(outputPathA, proc, compression_params);
        }
        catch (...) {
            return false;
        }
    }
     




    // CUDA-accelerated pipeline (best-effort)
    bool convertImageCUDA(const std::string& inputPathA, const std::string& outputPathA,
        const std::vector<int>& compression_params,
        const AppState& state)
    {
        try {
            cv::Mat cpuImg = cv::imread(inputPathA, cv::IMREAD_UNCHANGED);
            if (cpuImg.empty()) return false;

            cv::cuda::GpuMat gpuMat;
            gpuMat.upload(cpuImg);
            cv::cuda::GpuMat proc = gpuMat;

            // Resize
            if (state.doResize && state.resizePercent != 100) {
                double scale = state.resizePercent / 100.0;
                cv::Size s((int)(proc.cols * scale), (int)(proc.rows * scale));
                if (s.width > 0 && s.height > 0) {
                    cv::cuda::GpuMat tmp;
                    cv::cuda::resize(proc, tmp, s, 0, 0, cv::INTER_LINEAR);
                    proc = tmp;
                }
            }

             

            // Color convert - NOTE: CUDA color conversion may have limited options
            if (state.doColorConvert) {
                cv::cuda::GpuMat tmp;

                // For CUDA, we need to check what conversions are available
                // CUDA may not support all conversions, so we might need to download to CPU
                try {
                    switch (state.colorMode) {
                    case AppState::AUTO_DETECT:
                        // Keep as is
                        break;
                    case AppState::BGR_3CHANNEL:
                        if (proc.channels() == 1) {
                            cv::cuda::cvtColor(proc, tmp, cv::COLOR_GRAY2BGR);
                            proc = tmp;
                        }
                        else if (proc.channels() == 4) {
                            cv::cuda::cvtColor(proc, tmp, cv::COLOR_BGRA2BGR);
                            proc = tmp;
                        }
                        break;
                    case AppState::GRAYSCALE:
                        if (proc.channels() == 3) {
                            cv::cuda::cvtColor(proc, tmp, cv::COLOR_BGR2GRAY);
                            proc = tmp;
                        }
                        else if (proc.channels() == 4) {
                            // CUDA might not have BGRA2GRAY, fallback to CPU
                            cv::Mat cpuTmp;
                            proc.download(cpuTmp);
                            cv::cvtColor(cpuTmp, cpuTmp, cv::COLOR_BGRA2GRAY);
                            proc.upload(cpuTmp);
                        }
                        break;
                    case AppState::RGBA_4CHANNEL:
                        // CUDA might not support these, fallback to CPU
                        cv::Mat cpuTmp;
                        proc.download(cpuTmp);
                        if (cpuTmp.channels() == 1) {
                            cv::cvtColor(cpuTmp, cpuTmp, cv::COLOR_GRAY2BGRA);
                        }
                        else if (cpuTmp.channels() == 3) {
                            cv::cvtColor(cpuTmp, cpuTmp, cv::COLOR_BGR2BGRA);
                        }
                        proc.upload(cpuTmp);
                        break;
                    }
                }
                catch (...) {
                    // If CUDA conversion fails, fallback to CPU pipeline
                    return convertImageCPU(inputPathA, outputPathA, compression_params, state);
                }
            }

            // Denoise - CUDA denoise is not universally available
            if (state.doDenoise) {
                bool didCudaDenoise = false;
                try {
                    // Try CUDA denoise if available
                    // Note: This depends on your OpenCV CUDA build
                    // For now, fallback to CPU denoise
                    didCudaDenoise = false;
                }
                catch (...) {
                    didCudaDenoise = false;
                }

                if (!didCudaDenoise) {
                    // Fallback to CPU denoise
                    cv::Mat tmp;
                    proc.download(tmp);

                    int h = state.denoiseStrength;
                    int templateWindowSize = state.denoiseTemplateWindowSize;
                    int searchWindowSize = state.denoiseSearchWindowSize;

                    if (tmp.channels() >= 3) {
                        cv::Mat out;
                        cv::fastNlMeansDenoisingColored(tmp, out, h, h,
                            templateWindowSize, searchWindowSize);
                        proc.upload(out);
                    }
                    else {
                        cv::Mat out;
                        cv::fastNlMeansDenoising(tmp, out, h,
                            templateWindowSize, searchWindowSize);
                        proc.upload(out);
                    }
                }
            }

            // Download and write
            cv::Mat result;
            proc.download(result);
            return cv::imwrite(outputPathA, result, compression_params);
        }
        catch (const cv::Exception& e) {
            std::cout << "CUDA pipeline failed: " << e.what() << " - falling back to CPU\n";
            return convertImageCPU(inputPathA, outputPathA, compression_params, state);
        }
        catch (...) {
            return convertImageCPU(inputPathA, outputPathA, compression_params, state);
        }
    }






    // OpenCL/UMat pipeline
    bool convertImageOCL(const std::string& inputPathA, const std::string& outputPathA,
        const std::vector<int>& compression_params,
        const AppState& state)
    {
        try {
            cv::Mat cpu = cv::imread(inputPathA, cv::IMREAD_UNCHANGED);
            if (cpu.empty()) return false;

            cv::UMat u;
            cpu.copyTo(u);

            cv::UMat proc = u;

            if (state.doResize && state.resizePercent != 100) {
                double scale = state.resizePercent / 100.0;
                cv::Size s((int)(proc.cols * scale), (int)(proc.rows * scale));
                if (s.width > 0 && s.height > 0) cv::resize(proc, proc, s, 0, 0, cv::INTER_LINEAR);
            }

            if (state.doColorConvert) {
                // OpenCL/UMat color conversion
                switch (state.colorMode) {
                case AppState::AUTO_DETECT:
                    // Keep as is
                    break;
                case AppState::BGR_3CHANNEL:
                    if (proc.channels() == 1) cv::cvtColor(proc, proc, cv::COLOR_GRAY2BGR);
                    else if (proc.channels() == 4) cv::cvtColor(proc, proc, cv::COLOR_BGRA2BGR);
                    break;
                case AppState::GRAYSCALE:
                    if (proc.channels() == 3) cv::cvtColor(proc, proc, cv::COLOR_BGR2GRAY);
                    else if (proc.channels() == 4) cv::cvtColor(proc, proc, cv::COLOR_BGRA2GRAY);
                    break;
                case AppState::RGBA_4CHANNEL:
                    if (proc.channels() == 1) {
                        cv::cvtColor(proc, proc, cv::COLOR_GRAY2BGRA);
                    }
                    else if (proc.channels() == 3) {
                        cv::cvtColor(proc, proc, cv::COLOR_BGR2BGRA);
                    }
                    break;
                }
            }

            if (state.doDenoise) {
                int h = state.denoiseStrength;
                int templateWindowSize = state.denoiseTemplateWindowSize;
                int searchWindowSize = state.denoiseSearchWindowSize;

                if (proc.channels() >= 3) {
                    cv::UMat out;
                    cv::fastNlMeansDenoisingColored(proc, out, h, h,
                        templateWindowSize, searchWindowSize);
                    proc = out;
                }
                else {
                    cv::UMat out;
                    cv::fastNlMeansDenoising(proc, out, h,
                        templateWindowSize, searchWindowSize);
                    proc = out;
                }
            }

            cv::Mat result;
            proc.copyTo(result);
            return cv::imwrite(outputPathA, result, compression_params);
        }
        catch (const cv::Exception& e) {
            std::cout << "OpenCL/UMat pipeline failed: " << e.what() << " - falling back to CPU\n";
            return convertImageCPU(inputPathA, outputPathA, compression_params, state);
        }
        catch (...) {
            return convertImageCPU(inputPathA, outputPathA, compression_params, state);
        }
    }

public:

    friend void ProcessFilesAsync(HWND, const std::vector<std::wstring>&);

    ImageConverter(int quality = 95, bool useGpu = false) : config(quality, useGpu), gpu_device() {
        std::cout << "ImageConverter initialized - Quality: " << quality
            << ", GPU: " << ((useGpu && gpu_device.isAvailable()) ? "Enabled" : "Disabled")
            << ", Threads: " << config.threadCount << std::endl;
        if (gpu_device.isAvailable()) {
            std::cout << "Detected GPU backend: " << gpu_device.name() << " (CUDA=" << (gpu_device.isCuda() ? "yes" : "no") << ")\n";
        }
    }

    void convert(const std::wstring& inputPath, const std::wstring& outputDir,
        const std::wstring& formatExt, HWND hwnd = nullptr) {
        LARGE_INTEGER start, end, freq;
        QueryPerformanceCounter(&start);
        QueryPerformanceFrequency(&freq);

        std::vector<std::wstring> imageFiles = getAllImageFiles(inputPath);
        if (imageFiles.empty()) {
            if (hwnd) MessageBoxW(hwnd, L"No supported images found!", L"Info", MB_OK | MB_ICONINFORMATION);
            return;
        }

        // === SET TOTAL FOR PROGRESS TRACKING ===
        g_conversionTotal = (int)imageFiles.size();
        g_conversionProgress = 0;

        // Force immediate UI update to show 0% progress
        if (hwnd) {
            InvalidateRect(hwnd, NULL, FALSE);
            UpdateWindow(hwnd);
        }

        std::atomic<int> completed(0);
        std::atomic<int> failed(0);
        bool actualGpuUsage = config.useGpu && gpu_device.isAvailable();

        CreateDirectoryW(outputDir.c_str(), nullptr);

        ThreadPool pool(config.threadCount);
        for (const auto& imageFile : imageFiles) {
            pool.enqueue([this, imageFile, &outputDir, &formatExt, &completed, &failed, actualGpuUsage, hwnd] {
                // === UPDATE STATUS WITH FILENAME ===
                if (hwnd) {
                    std::lock_guard<std::mutex> lock(g_statusMutex);
                    // Extract just the filename
                    size_t lastSlash = imageFile.find_last_of(L"\\/");
                    g_conversionStatus = (lastSlash != std::wstring::npos)
                        ? imageFile.substr(lastSlash + 1)
                        : imageFile;
                }

                // Convert the image
                convertSingleImage(imageFile, outputDir, formatExt, completed, failed, actualGpuUsage);

                // === INCREMENT PROGRESS ===
                g_conversionProgress++;

                // === TRIGGER UI UPDATE ===
                if (hwnd) {
                    PostMessage(hwnd, WM_USER + 1, 0, 0);
                }
                });
        }
        pool.wait();

        QueryPerformanceCounter(&end);
        double duration = static_cast<double>(end.QuadPart - start.QuadPart) / freq.QuadPart;

        // === SHOW RESULT POPUP ===
        if (hwnd) {
            g_resultImageCount = imageFiles.size();
            g_resultFailedCount = failed.load();
            g_resultDuration = duration;
            g_resultGpuUsed = actualGpuUsage;
            g_showResult = true;
            g_resultTime = clock();
            g_lastFailedCount = failed.load();
            InvalidateRect(hwnd, NULL, FALSE);
        }
    }

////////////////////////////////////////////////////////



    void setQuality(int quality) { config.quality = quality; }
    void setUseGpu(bool useGpu) { config.useGpu = useGpu; if (useGpu) config.threadCount = std::max(1, config.threadCount / 2); }
    int getQuality() const { return config.quality; }
    bool isGpuAvailable() const { return gpu_device.isAvailable(); }
    bool isUsingGpu() const { return config.useGpu && gpu_device.isAvailable(); }

private:
    void convertSingleImage(const std::wstring& inputPath, const std::wstring& outputDir,
        const std::wstring& formatExt, std::atomic<int>& completed,
        std::atomic<int>& failed, bool useGpu)
    {
        try {
            // Convert input path to UTF-8
            int size_needed = WideCharToMultiByte(CP_UTF8, 0, inputPath.c_str(), -1,
                nullptr, 0, nullptr, nullptr);
            std::string inputPathA(size_needed, 0);
            WideCharToMultiByte(CP_UTF8, 0, inputPath.c_str(), -1,
                inputPathA.data(), size_needed, nullptr, nullptr);

            // Generate output path
            std::wstring outputPath = getOutputPath(inputPath, outputDir, formatExt);

            // Convert output path to UTF-8
            size_needed = WideCharToMultiByte(CP_UTF8, 0, outputPath.c_str(), -1,
                nullptr, 0, nullptr, nullptr);
            std::string outputPathA(size_needed, 0);
            WideCharToMultiByte(CP_UTF8, 0, outputPath.c_str(), -1,
                outputPathA.data(), size_needed, nullptr, nullptr);

            // Set compression parameters based on format
            std::vector<int> compression_params;

            if (formatExt == L".jpg" || formatExt == L".jpeg") {
                compression_params = { cv::IMWRITE_JPEG_QUALITY, config.quality };
            }
            else if (formatExt == L".png") {
                // PNG compression: 0-9 (0=no compression, 9=max compression)
                int png_compression = std::max(0, std::min(9, (100 - config.quality) / 10));
                compression_params = { cv::IMWRITE_PNG_COMPRESSION, png_compression };
            }
            else if (formatExt == L".webp") {
                compression_params = { cv::IMWRITE_WEBP_QUALITY, config.quality };
            }
            else if (formatExt == L".tif" || formatExt == L".tiff") {
                // TIFF compression: 0=none, 1=LZW, 2=JPEG, 3=PACKBITS, 4=DEFLATE
                compression_params = { cv::IMWRITE_TIFF_COMPRESSION, 1 }; // LZW compression
            }
            // For BMP and GIF, no compression parameters needed
            else if (formatExt == L".bmp") {
                compression_params = {}; // BMP has no compression
            }
            else if (formatExt == L".gif") {
                // Note: OpenCV may have limited GIF support
                compression_params = {};
            }
            else {
                // Default to JPEG quality for unknown formats
                compression_params = { cv::IMWRITE_JPEG_QUALITY, config.quality };
            }

            // Build AppState snapshot from global g_state (thread-safe copy)
            AppState stateSnapshot;
            stateSnapshot.quality = g_state.quality;
            stateSnapshot.doResize = g_state.doResize;
            stateSnapshot.doColorConvert = g_state.doColorConvert;
            stateSnapshot.doDenoise = g_state.doDenoise;
            stateSnapshot.resizePercent = g_state.resizePercent;

            // New fields for color conversion
            stateSnapshot.colorMode = g_state.colorMode;

            // New fields for denoise control
            stateSnapshot.denoiseStrength = g_state.denoiseStrength;
            stateSnapshot.denoiseTemplateWindowSize = g_state.denoiseTemplateWindowSize;
            stateSnapshot.denoiseSearchWindowSize = g_state.denoiseSearchWindowSize;

            // Resize input field
            stateSnapshot.resizeInput = g_state.resizeInput;
            stateSnapshot.resizeInputActive = g_state.resizeInputActive;

            // Debug output
#ifdef _DEBUG
            std::wcout << L"Converting: " << inputPath << L" -> " << outputPath << std::endl;
            std::cout << "Settings: Resize=" << (stateSnapshot.doResize ? "Yes" : "No");
            if (stateSnapshot.doResize) std::cout << " (" << stateSnapshot.resizePercent << "%)";
            std::cout << ", ColorConvert=" << (stateSnapshot.doColorConvert ? "Yes" : "No");
            if (stateSnapshot.doColorConvert) std::cout << " (Mode=" << (int)stateSnapshot.colorMode << ")";
            std::cout << ", Denoise=" << (stateSnapshot.doDenoise ? "Yes" : "No");
            if (stateSnapshot.doDenoise) std::cout << " (Strength=" << stateSnapshot.denoiseStrength << ")";
            std::cout << std::endl;
#endif

            bool success = false;

            // Choose the appropriate conversion pipeline
            if (useGpu && gpu_device.isAvailable()) {
                if (gpu_device.isCuda()) {
                    // CUDA-accelerated pipeline
                    success = convertImageCUDA(inputPathA, outputPathA, compression_params, stateSnapshot);
                }
                else {
                    // OpenCL/UMat pipeline
                    success = convertImageOCL(inputPathA, outputPathA, compression_params, stateSnapshot);
                }
            }
            else {
                // CPU pipeline
                success = convertImageCPU(inputPathA, outputPathA, compression_params, stateSnapshot);
            }

            if (success) {
                completed++;
#ifdef _DEBUG
                std::wcout << L"Success: " << outputPath << std::endl;
#endif
            }
            else {
                failed++;
#ifdef _DEBUG
                std::wcout << L"Failed: " << inputPath << std::endl;
#endif
            }
        }
        catch (const std::exception& e) {
            // Log exception details in debug mode
#ifdef _DEBUG
            std::cerr << "Exception in convertSingleImage: " << e.what() << std::endl;
#endif
            failed++;
        }
        catch (...) {
#ifdef _DEBUG
            std::cerr << "Unknown exception in convertSingleImage" << std::endl;
#endif
            failed++;
        }
    }
};







std::string getAvailableGpuBackend() {
    try {
        int cudaCount = 0;
        try { cudaCount = cv::cuda::getCudaEnabledDeviceCount(); }
        catch (...) { cudaCount = 0; }
        if (cudaCount > 0) {
            try {
                cv::cuda::DeviceInfo di(0);
                if (di.isCompatible()) return di.name();
            }
            catch (...) {}
        }
    }
    catch (...) {}

    try {
        if (cv::ocl::haveOpenCL()) {
            try {
                cv::ocl::Context ctx = cv::ocl::Context::getDefault();
                if (ctx.ndevices() > 0) {
                    cv::ocl::Device dev = ctx.device(0);
                    return dev.name();
                }
            }
            catch (...) {
                try {
                    cv::ocl::Context ctx;
                    if (ctx.create(cv::ocl::Device::TYPE_ALL) && ctx.ndevices() > 0) {
                        cv::ocl::Device dev = ctx.device(0);
                        return dev.name();
                    }
                }
                catch (...) {}
            }
        }
    }
    catch (...) {}

    return "";
}






/////////////////////////////////////////////////////////////////////////////////////
//
//  MAIN
// 
////////////////////////////////////////////////////////////////////////////////////

ImageConverter* g_converter = nullptr;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    InitCommonControls();

    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        MessageBoxW(NULL, L"Failed to initialize COM!", L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    hr = OleInitialize(NULL);
    if (FAILED(hr)) {
        MessageBoxW(NULL, L"Failed to initialize OLE!", L"Error", MB_OK | MB_ICONERROR);
        CoUninitialize();
        return FALSE;
    }

    InitializePathsAndFolders();

    g_converter = new ImageConverter(95, true);

    // Auto-enable GPU if available — best default behavior
    if (g_converter && g_converter->isGpuAvailable())
    {
        g_state.useGpu = true;
    }

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_QUICK, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow)) {
        delete g_converter;
        OleUninitialize();
        CoUninitialize();
        return FALSE;
    }

    EnableDarkMode();

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_QUICK));

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    delete g_converter;
    OleUninitialize();
    CoUninitialize();

    return (int)msg.wParam;
}



ATOM MyRegisterClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_QUICK));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = CreateSolidBrush(RGB(10, 81, 152));
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
    hInst = hInstance;

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    int windowWidth = WINDOW_WIDTH;
    int windowHeight = WINDOW_HEIGHT;

    int posX = (screenWidth - windowWidth) / 2;
    int posY = (screenHeight - windowHeight) / 2;

    /*
    HWND hWnd = CreateWindowW(szWindowClass, szTitle,
        WS_POPUP | WS_VISIBLE | WS_BORDER,
        posX, posY, windowWidth, windowHeight,
        nullptr, nullptr, hInstance, nullptr);
        */

    HWND hWnd = CreateWindowExW(
        WS_EX_COMPOSITED |           // Critical: forces full double-buffering during move/size
        WS_EX_APPWINDOW |            // Shows in taskbar + Alt+Tab
        WS_EX_CONTROLPARENT,         // Better keyboard navigation (optional)

        szWindowClass, szTitle,
        WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,

        posX, posY, windowWidth, windowHeight,
        nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) {
        MessageBoxW(NULL, L"Window creation failed!", L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    g_hWnd = hWnd;
    DragAcceptFiles(hWnd, TRUE);

     

     



    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    SetForegroundWindow(hWnd);
    BringWindowToTop(hWnd);

    return TRUE;
}




void ProcessFilesAsync(HWND hWnd, const std::vector<std::wstring>& paths)
{
    if (!g_converter || paths.empty()) return;

    if (g_isConverting.exchange(true)) {
        MessageBoxW(hWnd, L"Conversion already in progress!", L"Busy", MB_OK | MB_ICONINFORMATION);
        return;
    }

    g_conversionProgress = 0;
    g_conversionTotal = 0;
    g_resultImageCount = 0;
    g_resultFailedCount = 0;
    g_resultGpuUsed = false;
    g_showResult = false;

    // === 1. Collect ALL images (supports folders + files) ===
    std::vector<std::wstring> allImages;
    allImages.reserve(2000);

    for (const auto& p : paths) {
        auto files = g_converter->getAllImageFiles(p);
        allImages.insert(allImages.end(), files.begin(), files.end());
    }

    if (allImages.empty()) {
        g_isConverting = false;
        MessageBoxW(hWnd, L"No supported images found!", L"Info", MB_OK | MB_ICONINFORMATION);
        return;
    }

    // === 2. Prepare output format & folder ===
    std::wstring formatExt;
    switch (g_state.format) {
    case OutputFormat::JPG:  formatExt = L".jpg";  break;
    case OutputFormat::PNG:  formatExt = L".png";  break;
    case OutputFormat::WEBP: formatExt = L".webp"; break;
    case OutputFormat::TIF:  formatExt = L".tif";  break;
    case OutputFormat::BMP:  formatExt = L".bmp";  break;
    case OutputFormat::GIF:  formatExt = L".gif";  break;
    }

    std::wstring outputDir = g_state.outputFolder.empty() ? std::wstring(g_outputFolder) : g_state.outputFolder;

    // === 3. Compression params ===
    std::vector<int> params;
    if (g_state.format == OutputFormat::JPG)
        params = { cv::IMWRITE_JPEG_QUALITY, g_state.quality };
    else if (g_state.format == OutputFormat::WEBP)
        params = { cv::IMWRITE_WEBP_QUALITY, g_state.quality };
    else if (g_state.format == OutputFormat::PNG)
        params = { cv::IMWRITE_PNG_COMPRESSION, 9 };

    clock_t start = clock();


    // ULTRA SPEED MODE ENGAGED
       // ←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
       // SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
       // ↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑

    // === 4. Background thread with ThreadPool (THE FIXED VERSION) ===
    std::thread([hWnd, allImages = std::move(allImages), outputDir, formatExt, params, start]() {


        //////////////////////////
        //SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
        /////////////////////////


        int threadCount = std::max(8, (int)std::thread::hardware_concurrency());
        if (g_state.useGpu && g_converter->isGpuAvailable())
            threadCount = std::max(16, threadCount);

        ThreadPool pool(threadCount);

        // CORRECT ATOMIC COUNTERS
        std::atomic<int> processed{ 0 };
        std::atomic<int> succeeded{ 0 };
        std::atomic<int> failed_count{ 0 };
        bool usedGpu = false;
        bool forceCpu = g_state.doDenoise;  // Denoising kills GPU speed anyway

        // Create output folder once
        CreateDirectoryW(outputDir.c_str(), nullptr);

        // Set total immediately so UI shows "0 / N"
        g_conversionTotal.store((int)allImages.size());
        PostMessage(hWnd, WM_USER + 1, 0, 0);

        for (const auto& inputPath : allImages) {
            pool.enqueue([&, inputPath]() {
                // Update status bar with filename
                std::wstring filename = inputPath.substr(inputPath.find_last_of(L"\\") + 1);
                {
                    std::lock_guard<std::mutex> l(g_statusMutex);
                    g_conversionStatus = L"Blazing: " + filename;
                }

                std::wstring outputPath = g_converter->getOutputPath(inputPath, outputDir, formatExt);

                // Wide → UTF-8
                auto toUtf8 = [](const std::wstring& s) -> std::string {
                    int len = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, nullptr, 0, nullptr, nullptr);
                    std::string out(len - 1, 0);
                    WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, out.data(), len, nullptr, nullptr);
                    return out;
                    };

                std::string inA = toUtf8(inputPath);
                std::string outA = toUtf8(outputPath);

                bool success = false;

                if (g_state.useGpu && g_converter->isGpuAvailable() && !forceCpu) {
                    usedGpu = true;
                    if (g_converter->gpu_device.isCuda())
                        success = g_converter->convertImageCUDA(inA, outA, params, g_state);
                    else
                        success = g_converter->convertImageOCL(inA, outA, params, g_state);
                }
                else {
                    success = g_converter->convertImageCPU(inA, outA, params, g_state);
                }

                if (success) succeeded++;
                else failed_count++;

                // CORRECT PROGRESS UPDATE
                int current = ++processed;
                g_conversionProgress.store(current);

                PostMessage(hWnd, WM_USER + 1, 0, 0);
                });
        }

        pool.wait();

        double sec = (double)(clock() - start) / CLOCKS_PER_SEC;

        g_resultImageCount = succeeded.load();
        g_resultFailedCount = failed_count.load();
        g_resultDuration = sec;
        g_resultGpuUsed = usedGpu;
        g_showResult = true;
        g_resultTime = clock();
        g_isConverting = false;
        PostMessage(hWnd, WM_USER + 1, 0, 0);

        // Final update
        PostMessage(hWnd, WM_USER + 1, 0, 0);

        // Optional: force Explorer refresh
       //SHChangeNotify(SHCHG_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);

        }).detach();
}

 
 

void DrawRoundedRect(HDC hdc, int x, int y, int w, int h, int r, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_NULL, 0, 0);
    HGDIOBJ oldBrush = SelectObject(hdc, brush);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    RoundRect(hdc, x, y, x + w, y + h, r, r);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

void DrawGradientRect(HDC hdc, int x, int y, int w, int h,
    COLORREF color1, COLORREF color2, bool horizontal = true) {
    TRIVERTEX vertices[2];
    vertices[0].x = x;
    vertices[0].y = y;
    vertices[0].Red = GetRValue(color1) << 8;
    vertices[0].Green = GetGValue(color1) << 8;
    vertices[0].Blue = GetBValue(color1) << 8;
    vertices[0].Alpha = 0x0000;

    vertices[1].x = x + w;
    vertices[1].y = y + h;
    vertices[1].Red = GetRValue(color2) << 8;
    vertices[1].Green = GetGValue(color2) << 8;
    vertices[1].Blue = GetBValue(color2) << 8;
    vertices[1].Alpha = 0x0000;

    GRADIENT_RECT gRect;
    gRect.UpperLeft = 0;
    gRect.LowerRight = 1;

    GradientFill(hdc, vertices, 2, &gRect, 1,
        horizontal ? GRADIENT_FILL_RECT_H : GRADIENT_FILL_RECT_V);
}

void AboutNow(HWND hWnd) {



}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    const int PANEL_Y = 130;
    const int SLIDER_X = 110;
    const int SLIDER_Y = PANEL_Y + 35;
    const int SLIDER_W = 200;
    const int SLIDER_H = 6;

    
    static std::atomic<int> g_lastFailedCount = 0;   // Remember last failed count
    const int SETTINGS_PANEL_X = (WINDOW_WIDTH / 2) + 30;
    const int SETTINGS_PANEL_Y = PANEL_Y + 140;
    const int SETTINGS_PANEL_W = (WINDOW_WIDTH / 2) - 60;
    const int SETTINGS_PANEL_H = WINDOW_HEIGHT - SETTINGS_PANEL_Y - 60;

    // Checkbox positions
    const int CHK_RESIZE_X = SETTINGS_PANEL_X + 30;
    const int CHK_RESIZE_Y = SETTINGS_PANEL_Y + 40;
    const int CHK_SIZE = 20;

    const int CHK_COLOR_X = CHK_RESIZE_X;
    const int CHK_COLOR_Y = CHK_RESIZE_Y + 40;

    const int CHK_DENOISE_X = CHK_RESIZE_X;
    const int CHK_DENOISE_Y = CHK_COLOR_Y + 40;

    // Resize input field
    const int RESIZE_INPUT_X = CHK_RESIZE_X + 120;
    const int RESIZE_INPUT_Y = CHK_RESIZE_Y;
    const int RESIZE_INPUT_W = 60;
    const int RESIZE_INPUT_H = 25;

    // Color mode selection (below color checkbox)
    const int COLOR_MODE_Y = CHK_COLOR_Y + 30;
    const int COLOR_MODE_BTN_W = 60;
    const int COLOR_MODE_BTN_H = 25;

    // Denoise slider (below denoise checkbox)
    const int DENOISE_SLIDER_X = CHK_DENOISE_X;
    const int DENOISE_SLIDER_Y = CHK_DENOISE_Y + 30;
    const int DENOISE_SLIDER_W = 150;
    const int DENOISE_SLIDER_H = 6;
    HFONT hFonto = NULL;

    // Static variables for controls
    static HWND g_hResizeEdit = nullptr;
    static bool g_bIsDraggingDenoiseSlider = false;
    static bool g_hoverFolder = false;
    static bool g_hoverGpu = false;
   


    switch (message) {




    case WM_TIMER: {
        if (wParam == 1001) {
            // Minimize button animation
            g_minimizeAnimFrame = (g_minimizeAnimFrame + 1) % 4;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (wParam == 1002) {
            // Progress bar animation
            g_progressAnimOffset = (g_progressAnimOffset + 2) % 100;

            if (!g_isConverting.load()) {
                KillTimer(hWnd, 1002); // Stop when done
            }
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }




    case WM_CREATE: {
        DragAcceptFiles(hWnd, TRUE);

        g_hAppIcon = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_QUICK));

        // Start animation timer (every 120ms)
        SetTimer(hWnd, 1001, MINIMIZE_ANIM_INTERVAL, nullptr);

        g_minimizeAnimTime = clock();


        
       

        // Create edit control for resize input
        g_hResizeEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"100",
            WS_CHILD | ES_NUMBER | ES_RIGHT,
            RESIZE_INPUT_X, RESIZE_INPUT_Y, RESIZE_INPUT_W, RESIZE_INPUT_H,
            hWnd, (HMENU)1004, hInst, NULL);

        // Limit to 3 digits (max 100%)
        SendMessage(g_hResizeEdit, EM_SETLIMITTEXT, 3, 0);

        // Set font
        hFonto = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        SendMessage(g_hResizeEdit, WM_SETFONT, (WPARAM)hFonto, TRUE);
        DeleteObject(hFonto);

        // Initially hide it
        ShowWindow(g_hResizeEdit, SW_HIDE);
        return 0;
    }



    case WM_DROPFILES: {
        HDROP hDrop = (HDROP)wParam;
        UINT count = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);
        std::vector<std::wstring> paths;
        for (UINT i = 0; i < count; i++) {
            wchar_t buffer[MAX_PATH];
            if (DragQueryFileW(hDrop, i, buffer, MAX_PATH)) paths.emplace_back(buffer);
        }
        DragFinish(hDrop);

        // Start animation timer
        SetTimer(hWnd, 1002, 30, nullptr); // 30ms = ~33 FPS
        g_progressAnimTime = clock();

        ProcessFilesAsync(hWnd, paths);
        return 0;
    }



    case WM_USER + 1: {
    // Force immediate redraw
    InvalidateRect(hWnd, NULL, FALSE);
    UpdateWindow(hWnd);  // <- Add this for immediate update
    return 0;
}


    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);

        if (id == 1004 && code == EN_KILLFOCUS) { // Edit control lost focus
            wchar_t buffer[10];
            GetWindowText(g_hResizeEdit, buffer, 10);

            int percent = _wtoi(buffer);
            if (percent < 10) percent = 10;
            if (percent > 100) percent = 100;

            g_state.resizePercent = percent;
            swprintf_s(buffer, L"%d", percent);
            g_state.resizeInput = buffer;

            g_state.resizeInputActive = false;
            ShowWindow(g_hResizeEdit, SW_HIDE);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        bool handled = false;


        if (web) {
            ShellExecuteW(
                nullptr,              // parent window
                L"open",              // operation
                L"https://www.nutz.club/", // URL
                nullptr,              // parameters
                nullptr,              // directory
                SW_SHOWNORMAL         // show state
            );
            web = false;
        } 
        else 

        if (g_inAnimeMode) {
            // USER CLICKED → EXIT ANIMATION MODE
            g_inAnimeMode = false;

         

            // Force full repaint of the normal interface
            InvalidateRect(hWnd, nullptr, TRUE);  // TRUE = erase background (critical!)
            UpdateWindow(hWnd);                   // Force instant redraw

            break;
        }
        

        if (x >= 35 &&  x<270 &&y >= 40 && y <= 64) {  g_inAnimeMode = true;    }
         
     

        // === MINIMIZE BUTTON CLICK ===
        if (x >= MINIMIZE_ICON_X && x <= MINIMIZE_ICON_X + MINIMIZE_ICON_SIZE &&
            y >= MINIMIZE_ICON_Y && y <= MINIMIZE_ICON_Y + MINIMIZE_ICON_SIZE)
        {
            ShowWindow(hWnd, SW_MINIMIZE);
            handled = true;
        }

///////////////////////////////////////////
        // === FOLDER ICON ===
        if (x >= FOLDER_ICON_X && x <= FOLDER_ICON_X + ICON_SIZE &&
            y >= ICON_MARGIN && y <= ICON_MARGIN + ICON_SIZE)      {
            SelectOutputFolder(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
            handled = true;
        }
        // === EXIT ICON ===
        else if (x >= EXIT_ICON_X && x <= EXIT_ICON_X + ICON_SIZE &&
            y >= ICON_MARGIN && y <= ICON_MARGIN + ICON_SIZE)  {
            PostQuitMessage(0);
            handled = true;
        }

        // === CLICK OUTPUT FOLDER TEXT → Open Explorer ===
        else if (g_hoverOutputFolder) {
            std::wstring path = g_state.outputFolder.empty() ? g_outputFolder : g_state.outputFolder;
            ShellExecuteW(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            handled = true;
        }

        // Quality Slider Hit Test
        else if (x >= SLIDER_X - 10 && x <= SLIDER_X + SLIDER_W + 10 && y >= SLIDER_Y - 15 && y <= SLIDER_Y + 15) {
            g_state.isDraggingSlider = true;
            SetCapture(hWnd);
            handled = true;
        }

        

        // === GPU TOGGLE FIRST (so it wins!) ===
        else if (g_converter && g_converter->isGpuAvailable() &&
            x >= GPU_TOGGLE_X && x <= GPU_TOGGLE_X + GPU_TOGGLE_W &&
            y >= GPU_TOGGLE_Y && y <= GPU_TOGGLE_Y + GPU_TOGGLE_H)
        {
            g_state.useGpu = !g_state.useGpu;
            InvalidateRect(hWnd, NULL, FALSE);
            handled = true;
           // MessageBoxW(hWnd, L"GPU Toggled!", L"Success", MB_OK);
        }

        // Visual Checkboxes (right panel)
        else if (x >= SETTINGS_PANEL_X && x <= SETTINGS_PANEL_X + SETTINGS_PANEL_W &&
            y >= SETTINGS_PANEL_Y && y <= SETTINGS_PANEL_Y + SETTINGS_PANEL_H) {

            // Resize checkbox
            if (x >= CHK_RESIZE_X && x <= CHK_RESIZE_X + CHK_SIZE &&
                y >= CHK_RESIZE_Y && y <= CHK_RESIZE_Y + CHK_SIZE) {
                g_state.doResize = !g_state.doResize;
                if (g_state.doResize && g_state.resizePercent == 100) {
                    g_state.resizePercent = 100;  
                    g_state.resizeInput = L"100";
                }
                InvalidateRect(hWnd, NULL, FALSE);
                handled = true;
            }

            // Color convert checkbox
            else if (x >= CHK_COLOR_X && x <= CHK_COLOR_X + CHK_SIZE &&
                y >= CHK_COLOR_Y && y <= CHK_COLOR_Y + CHK_SIZE) {
                g_state.doColorConvert = !g_state.doColorConvert;
                InvalidateRect(hWnd, NULL, FALSE);
                handled = true;
            }

            // Denoise checkbox
            else if (x >= CHK_DENOISE_X && x <= CHK_DENOISE_X + CHK_SIZE &&
                y >= CHK_DENOISE_Y && y <= CHK_DENOISE_Y + CHK_SIZE) {
                g_state.doDenoise = !g_state.doDenoise;
                InvalidateRect(hWnd, NULL, FALSE);
                handled = true;
            }

            // Resize input field click
            else if (g_state.doResize &&
                x >= RESIZE_INPUT_X && x <= RESIZE_INPUT_X + RESIZE_INPUT_W &&
                y >= RESIZE_INPUT_Y && y <= RESIZE_INPUT_Y + RESIZE_INPUT_H) {
                g_state.resizeInputActive = true;
                ShowWindow(g_hResizeEdit, SW_SHOW);
                SetWindowText(g_hResizeEdit, g_state.resizeInput.c_str());
                SetFocus(g_hResizeEdit);
                SendMessage(g_hResizeEdit, EM_SETSEL, 0, -1);
                InvalidateRect(hWnd, NULL, FALSE);
                handled = true;
            }

            // Color mode selection
            else if (g_state.doColorConvert &&
                y >= COLOR_MODE_Y && y <= COLOR_MODE_Y + COLOR_MODE_BTN_H) {
                for (int i = 0; i < 4; i++) {
                    int btnX = CHK_COLOR_X + i * (COLOR_MODE_BTN_W + 10);
                    if (x >= btnX && x <= btnX + COLOR_MODE_BTN_W) {
                        g_state.colorMode = static_cast<AppState::ColorMode>(i);
                        InvalidateRect(hWnd, NULL, FALSE);
                        handled = true;
                        break;
                    }
                }
            }

            // Denoise slider
            else if (g_state.doDenoise &&
                x >= DENOISE_SLIDER_X - 10 && x <= DENOISE_SLIDER_X + DENOISE_SLIDER_W + 10 &&
                y >= DENOISE_SLIDER_Y - 10 && y <= DENOISE_SLIDER_Y + 10) {
                g_bIsDraggingDenoiseSlider = true;
                SetCapture(hWnd);
                // Update immediately
                int relX = x - DENOISE_SLIDER_X;
                if (relX < 0) relX = 0;
                if (relX > DENOISE_SLIDER_W) relX = DENOISE_SLIDER_W;
                float ratio = (float)relX / DENOISE_SLIDER_W;
                g_state.denoiseStrength = 1 + (int)(ratio * 9);
                InvalidateRect(hWnd, NULL, FALSE);
                handled = true;
            }
        }

        // Format Buttons
        else if (y >= FMT_Y2 && y <= FMT_ROW2_Y + FMT_BTN_H) {
            for (int i = 0; i < 6; i++) {
                int row = i / 3;
                int col = i % 3;
                int bx = FMT_X2 + col * (FMT_BTN_W + 10);
                int by = (row == 0) ? FMT_Y2 : FMT_ROW2_Y;

                if (x >= bx && x <= bx + FMT_BTN_W && y >= by && y <= by + FMT_BTN_H) {
                    g_state.format = static_cast<OutputFormat>(i);
                    InvalidateRect(hWnd, NULL, FALSE);
                    handled = true;
                    break;
                }
            }
            }


        // Click outside resize input deactivates it
        if (g_state.resizeInputActive && !handled) {
            g_state.resizeInputActive = false;
            ShowWindow(g_hResizeEdit, SW_HIDE);
            InvalidateRect(hWnd, NULL, FALSE);
        }


        
         



        // If not handled by any control and not dragging slider, drag the window
        if (!handled && !g_state.isDraggingSlider && !g_bIsDraggingDenoiseSlider) {
            SendMessage(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        }

        return 0;
    }

    case WM_MOUSEMOVE: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);


        


        if (x >= 820 && x < 880 && y >= 510 && y <= 532) web = true;
        else web = false;
         
        // Title hover  
        bool inTitle = (x >= 35 && x < 270 && y >= 40 && y <= 64);

        if (inTitle != g_hoverTitle) {
            g_hoverTitle = inTitle;
            InvalidateRect(hWnd, nullptr, FALSE); // redraw title
        }

        // === MINIMIZE BUTTON HOVER (using constants!) ===
        bool newHoverMinimize = (x >= MINIMIZE_ICON_X &&
            x <= MINIMIZE_ICON_X + MINIMIZE_ICON_SIZE &&
            y >= MINIMIZE_ICON_Y &&
            y <= MINIMIZE_ICON_Y + MINIMIZE_ICON_SIZE);

        // Reset animation when hover starts
        if (newHoverMinimize && !g_hoverMinimize) {
            g_minimizeAnimTime = clock();
            g_minimizeAnimFrame = 0;
        }

        // Advance animation when hovered
        if (newHoverMinimize) {
            clock_t now = clock();
            if ((now - g_minimizeAnimTime) > 120) {  // 120ms per frame
                g_minimizeAnimFrame = (g_minimizeAnimFrame + 1) % 4;
                g_minimizeAnimTime = now;
            }
        }

        // Redraw only if hover state or frame changed
        if (newHoverMinimize != g_hoverMinimize ||
            (newHoverMinimize && ((clock() - g_minimizeAnimTime) > 60)))
        {
            g_hoverMinimize = newHoverMinimize;
            InvalidateRect(hWnd, NULL, FALSE);
        }




        // === Output Folder Text Hover ===
        bool newHoverOutput = (x >= 0 && x <= (WINDOW_WIDTH/2) &&
            y >= WINDOW_HEIGHT - 40 && y <= WINDOW_HEIGHT);

        if (newHoverOutput != g_hoverOutputFolder) {
            g_hoverOutputFolder = newHoverOutput;
            InvalidateRect(hWnd, NULL, FALSE);
        }

        // === CLOSE BUTTON HOVER ===
        bool newHoverClose = (x >= EXIT_ICON_X && x <= EXIT_ICON_X + ICON_SIZE &&
            y >= ICON_MARGIN && y <= ICON_MARGIN + ICON_SIZE);

        // Update hover states
        bool newHoverFolder = (x >= FOLDER_ICON_X && x <= FOLDER_ICON_X + ICON_SIZE &&
            y >= ICON_MARGIN && y <= ICON_MARGIN + ICON_SIZE);

        bool newHoverGpu = (x >= GPU_TOGGLE_X && x <= GPU_TOGGLE_X + GPU_TOGGLE_W &&
            y >= GPU_TOGGLE_Y && y <= GPU_TOGGLE_Y + GPU_TOGGLE_H);

        // Only invalidate if ANY hover state changed
            if (newHoverClose != g_hoverClose ||
                newHoverFolder != g_hoverFolder ||
                newHoverGpu != g_hoverGpu ||
                newHoverOutput != g_hoverOutputFolder ||
                newHoverMinimize != g_hoverMinimize)
            {
                g_hoverClose = newHoverClose;
                g_hoverFolder = newHoverFolder;
                g_hoverGpu = newHoverGpu;

                InvalidateRect(hWnd, NULL, FALSE);   // THIS LINE IS REQUIRED!
            }

        if (g_state.isDraggingSlider) {
            int relX = x - SLIDER_X;
            if (relX < 0) relX = 0;
            if (relX > SLIDER_W) relX = SLIDER_W;
            float ratio = (float)relX / SLIDER_W;
            g_state.quality = 10 + (int)(ratio * 90);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (g_bIsDraggingDenoiseSlider) {
            int relX = x - DENOISE_SLIDER_X;
            if (relX < 0) relX = 0;
            if (relX > DENOISE_SLIDER_W) relX = DENOISE_SLIDER_W;
            float ratio = (float)relX / DENOISE_SLIDER_W;
            g_state.denoiseStrength = 1 + (int)(ratio * 9);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }


    case WM_ERASEBKGND:
        return TRUE;

    case WM_LBUTTONUP:
        if (g_state.isDraggingSlider) {
            g_state.isDraggingSlider = false;
            ReleaseCapture();
        }
        if (g_bIsDraggingDenoiseSlider) {
            g_bIsDraggingDenoiseSlider = false;
            ReleaseCapture();
        }
        return 0;

    case WM_CHAR: {
        if (g_state.resizeInputActive && wParam >= '0' && wParam <= '9') {
            // Only allow digits
            if (g_state.resizeInput.length() < 3) { // Max 3 digits (100%)
                g_state.resizeInput += static_cast<wchar_t>(wParam);
                g_state.resizePercent = _wtoi(g_state.resizeInput.c_str());
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        else if (g_state.resizeInputActive && wParam == VK_BACK) {
            // Backspace
            if (!g_state.resizeInput.empty()) {
                g_state.resizeInput.pop_back();
                if (g_state.resizeInput.empty()) {
                    g_state.resizeInput = L"100";
                    g_state.resizePercent = 100;
                }
                else {
                    g_state.resizePercent = _wtoi(g_state.resizeInput.c_str());
                }
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        else if (g_state.resizeInputActive && wParam == VK_RETURN) {
            // Enter to finish
            g_state.resizeInputActive = false;
            ShowWindow(g_hResizeEdit, SW_HIDE);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_KEYDOWN: {
        if (wParam == VK_ESCAPE) {
            // If in resize input mode, exit it first
            if (g_state.resizeInputActive) {
                g_state.resizeInputActive = false;
                ShowWindow(g_hResizeEdit, SW_HIDE);
                InvalidateRect(hWnd, NULL, FALSE);
            }
            else {
                // If not in input mode, quit the application
                PostQuitMessage(0);
            }
        }
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBM = CreateCompatibleBitmap(hdc, WINDOW_WIDTH, WINDOW_HEIGHT);
        SelectObject(memDC, memBM);
        RECT rc = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
        HBRUSH bgBrush = CreateSolidBrush(RGB(10, 81, 152));
        FillRect(memDC, &rc, bgBrush);
        DeleteObject(bgBrush);
        SetBkMode(memDC, TRANSPARENT);
        

        HFONT hTitle = CreateFontW(44, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, L"Segoe UI");
        HFONT hSub = CreateFontW(ICON_SIZE, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, L"Segoe UI");
        HFONT hBold = CreateFontW(18, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, L"Segoe UI");
        HFONT hSmall = CreateFontW(14, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, L"Segoe UI");


        if (g_inAnimeMode) { PlayGodTierAnimeEnding(hdc, hWnd);   break; }

        


       // == = WEB BUTTON with hover effect == = 
        SetTextColor(memDC, RGB(33, 33,66));
        SetTextColor(memDC, web?  RGB((200+rand()%50), 222, (200 + rand() % 50)) : RGB(33, 33, 66));
        SelectObject(memDC, hSmall);
        RECT rExit2 = { 820,
                        505,
                        820 + 60,
                        505 + 22 };
        DrawTextW(memDC, L"www", -1, &rExit2, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        
        


        /// === RAINBOW SPINNING MINIMIZE BUTTON (GOD MODE) ===
        {
            const wchar_t* frames[] = { L"/", L"−", L"\\", L"|" };
            const wchar_t* icon = frames[g_minimizeAnimFrame];

            // Rainbow color — changes every frame!
            int r = (int)(127 + 128 * sin(clock() * 0.008));      // 0–255
            int g = (int)(127 + 128 * sin(clock() * 0.008 + 2));  // offset phase
            int b = (int)(127 + 128 * sin(clock() * 0.008 + 4));  // offset phase

            // Brighter & more intense on hover
            if (g_hoverMinimize) {
                r = std::min(255, r + 60);
                g = std::min(255, g + 60);
                b = std::min(255, b + 60);
            }

            SetTextColor(memDC, RGB(r, g, b));
            SelectObject(memDC, hSub);
            RECT rMin = {
                MINIMIZE_ICON_X, MINIMIZE_ICON_Y,
                MINIMIZE_ICON_X + MINIMIZE_ICON_SIZE,
                MINIMIZE_ICON_Y + MINIMIZE_ICON_SIZE
            };
            DrawTextW(memDC, icon, -1, &rMin, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            // Optional: tiny glow circle behind it
            if (g_hoverMinimize) {
                
                DrawRoundedRect(memDC,
                    MINIMIZE_ICON_X - 2, MINIMIZE_ICON_Y -2,
                    MINIMIZE_ICON_SIZE + 6, MINIMIZE_ICON_SIZE + 6,
                    10, RGB(200, 0, 0) );
                DrawTextW(memDC, icon, -1, &rMin, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }
        }
   /////////////////////
         

        
        // Folder icon hover effect
        SelectObject(memDC, hSub);
        SetTextColor(memDC,  RGB(200, 200, 255) );
        RECT rFolder = { FOLDER_ICON_X, ICON_MARGIN, FOLDER_ICON_X + ICON_SIZE, ICON_MARGIN + ICON_SIZE };

        if (g_hoverFolder) {

            DrawRoundedRect(memDC,
                FOLDER_ICON_X, ICON_MARGIN, ICON_SIZE,  ICON_SIZE,
                10, RGB(200, 0, 0));
            
        }
        
        DrawTextW(memDC, L"📁", -1, &rFolder, DT_CENTER | DT_VCENTER | DT_SINGLELINE);


        // === CLOSE BUTTON with hover effect ===

         COLORREF closeColor =   RGB(156, 0, 0);  // Bright red on hover!
        SetTextColor(memDC, closeColor);
        if (g_hoverClose) {

            DrawRoundedRect(memDC,
                EXIT_ICON_X, ICON_MARGIN, ICON_SIZE, ICON_SIZE,
                10, RGB(200, 0, 0));
        }
        RECT rExit = { EXIT_ICON_X, ICON_MARGIN, EXIT_ICON_X + ICON_SIZE, ICON_MARGIN + ICON_SIZE };
        DrawTextW(memDC, L"✕", -1, &rExit, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        

        // Title
        SetTextColor(memDC, g_hoverTitle ? RGB(1, 1, 1) : RGB(255, 255, 0));
       // SetTextColor(memDC, RGB(255, 255, 0));
        SelectObject(memDC, hTitle);
        RECT rTitle = { 30, 30, WINDOW_WIDTH, 80 };
        DrawTextW(memDC, L"⚡", -1, &rTitle, DT_LEFT | DT_SINGLELINE);
        SetTextColor(memDC, g_hoverTitle ? RGB(255, 200, 100) : RGB(255, 255, 255));
       // SetTextColor(memDC, RGB(255, 255, 255));
        rTitle = { 38 + ICON_SIZE, 30, WINDOW_WIDTH, 80 };
        DrawTextW(memDC, L"QuickConvert", -1, &rTitle, DT_LEFT | DT_SINGLELINE);

        // Subtitle
        SelectObject(memDC, hBold);
        SetTextColor(memDC, RGB(200, 220, 255));
        RECT rSub = { 30, 80, WINDOW_WIDTH, 110 };
        std::wstring subtitle = L"High-Quality • ";
        if (g_converter && g_converter->isGpuAvailable()) {
            std::string backend = getAvailableGpuBackend();
            if (!backend.empty()) subtitle += std::wstring(backend.begin(), backend.end()) + L" • ";
        }
        subtitle += L"Batch Processing";
        DrawTextW(memDC, subtitle.c_str(), -1, &rSub, DT_LEFT | DT_SINGLELINE);

        // Settings panel
        DrawRoundedRect(memDC, 30, PANEL_Y, WINDOW_WIDTH - 60, 120, 15, RGB(5, 40, 80));

        // Quality & slider
        SelectObject(memDC, hBold);
        SetTextColor(memDC, RGB(180, 210, 255));
        RECT rQ = { 45, PANEL_Y + 20, 140, PANEL_Y + 40 };
        DrawTextW(memDC, L"Quality:", -1, &rQ, DT_LEFT);
        wchar_t qVal[16];
        swprintf_s(qVal, L"%d%%", g_state.quality);
        RECT rQV = { 45, PANEL_Y + 40, 140, PANEL_Y + 60 };
        DrawTextW(memDC, qVal, -1, &rQV, DT_LEFT);
        DrawRoundedRect(memDC, SLIDER_X, SLIDER_Y, SLIDER_W, SLIDER_H, 3, RGB(40, 60, 100));
        float ratio = (float)(g_state.quality - 10) / 90.0f;
        int fillW = (int)(ratio * SLIDER_W);
        DrawRoundedRect(memDC, SLIDER_X, SLIDER_Y, fillW, SLIDER_H, 3, RGB(59, 130, 246));
        DrawRoundedRect(memDC, SLIDER_X + fillW - 8, SLIDER_Y - 5, 16, 16, 16, RGB(255, 255, 255));

        // Format selector - 6 buttons in 2 rows
        for (int i = 0; i < 6; i++) {
            int row = i / 3;
            int col = i % 3;
            int bx = FMT_X2 + col * (FMT_BTN_W + 10);
            int by = (row == 0) ? FMT_Y2 : FMT_ROW2_Y;

            bool isActive = (int)g_state.format == i;
            COLORREF btnColor = isActive ? RGB(59, 130, 246) : RGB(30, 50, 90);
            COLORREF txtColor = isActive ? RGB(255, 255, 255) : RGB(150, 170, 200);

            DrawRoundedRect(memDC, bx, by, FMT_BTN_W, FMT_BTN_H, 5, btnColor);

            HPEN hPen = CreatePen(PS_SOLID, 1, isActive ? RGB(100, 180, 255) : RGB(50, 80, 120));
            SelectObject(memDC, GetStockObject(NULL_BRUSH));
            SelectObject(memDC, hPen);
            RoundRect(memDC, bx, by, bx + FMT_BTN_W, by + FMT_BTN_H, 5, 5);
            DeleteObject(hPen);

            SetTextColor(memDC, txtColor);
            RECT rBtn = { bx, by, bx + FMT_BTN_W, by + FMT_BTN_H };
            DrawTextW(memDC, formats[i], -1, &rBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }

        
        // === FOOTER - Output Folder (Clickable + Hover Effect) ===
         
            // Footer g_output_folder
            SelectObject(memDC, hBold);
            SetTextColor(memDC, RGB(150, 180, 220));

            // Hover = bright blue + underline effect
            COLORREF footerColor = g_hoverOutputFolder ? RGB(100, 180, 255) : RGB(150, 180, 220);
            SetTextColor(memDC, footerColor);

            RECT rFoot = { 30, WINDOW_HEIGHT - 45, WINDOW_WIDTH - 30, WINDOW_HEIGHT - 10 };

            std::wstring foot = L"Output: ";
            foot += g_state.outputFolder.empty() ? std::wstring(g_outputFolder) : g_state.outputFolder;

            // Draw text
            DrawTextW(memDC, foot.c_str(), -1, &rFoot, DT_LEFT | DT_SINGLELINE);

            // Underline on hover (simple & beautiful)
            if (g_hoverOutputFolder) {
                // Draw a clean, bright underline right under the text
                int underlineY = WINDOW_HEIGHT - 24;  // ← Raised from -8 to -14 (6px higher)

                HPEN pen = CreatePen(PS_SOLID, 3, RGB(100, 180, 180));  // Thinner = more elegant
                HPEN oldPen = (HPEN)SelectObject(memDC, pen);

                // Start at text position
                MoveToEx(memDC, 30, underlineY, nullptr);

                // Estimate text width more accurately
                SIZE textSize;
                GetTextExtentPoint32W(memDC, foot.c_str(), (int)foot.length(), &textSize);
                LineTo(memDC, 30 + textSize.cx, underlineY);

                SelectObject(memDC, oldPen);
                DeleteObject(pen);
            }
         
      
        
        

        // Right Settings Panel
        DrawRoundedRect(memDC, SETTINGS_PANEL_X, SETTINGS_PANEL_Y,
            SETTINGS_PANEL_W, SETTINGS_PANEL_H, 20, RGB(5, 40, 80));

        // Panel Title
        SelectObject(memDC, hBold);
        SetTextColor(memDC, RGB(100, 100, 233));
        RECT rPanelTitle = { SETTINGS_PANEL_X, SETTINGS_PANEL_Y - 5,
                             SETTINGS_PANEL_X + SETTINGS_PANEL_W, SETTINGS_PANEL_Y + 50 };
        DrawTextW(memDC, L"Image Processing Settings", -1, &rPanelTitle,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Checkbox Visuals with Labels
        SelectObject(memDC, hSub);
        SetTextColor(memDC, RGB(255, 255, 255));

        // 1. Resize Checkbox
        DrawRoundedRect(memDC, CHK_RESIZE_X, CHK_RESIZE_Y, CHK_SIZE, CHK_SIZE, 4,
            g_state.doResize ? RGB(59, 130, 246) : RGB(30, 50, 90));
        if (g_state.doResize) {
            SetTextColor(memDC, RGB(255, 255, 255));
            RECT rCheck = { CHK_RESIZE_X, CHK_RESIZE_Y, CHK_RESIZE_X + CHK_SIZE, CHK_RESIZE_Y + CHK_SIZE };
            DrawTextW(memDC, L"✓", -1, &rCheck, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        SetTextColor(memDC, RGB(200, 220, 255));
        RECT rResizeLabel = { CHK_RESIZE_X + CHK_SIZE + 10, CHK_RESIZE_Y,
                              CHK_RESIZE_X + 200, CHK_RESIZE_Y + CHK_SIZE };
        DrawTextW(memDC, L"Resize", -1, &rResizeLabel, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        // Resize percentage input (only show when resize is checked)
        if (g_state.doResize) {
            // Only draw static text if not editing
            if (!g_state.resizeInputActive) {
                DrawRoundedRect(memDC, RESIZE_INPUT_X, RESIZE_INPUT_Y,
                    RESIZE_INPUT_W, RESIZE_INPUT_H, 4, RGB(40, 60, 100));

                SetTextColor(memDC, RGB(255, 255, 255));
                RECT rResizeInput = { RESIZE_INPUT_X + 5, RESIZE_INPUT_Y,
                                      RESIZE_INPUT_X + RESIZE_INPUT_W - 5, RESIZE_INPUT_Y + RESIZE_INPUT_H };

                wchar_t resizeText[10];
                swprintf_s(resizeText, L"%d", g_state.resizePercent);
                DrawTextW(memDC, resizeText, -1, &rResizeInput,
                    DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            }

            // Percent symbol
            RECT rPercent = { RESIZE_INPUT_X + RESIZE_INPUT_W + 5, RESIZE_INPUT_Y,
                              RESIZE_INPUT_X + RESIZE_INPUT_W + 30, RESIZE_INPUT_Y + RESIZE_INPUT_H };
            DrawTextW(memDC, L"%", -1, &rPercent, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }

        // 2. Color Convert Checkbox
        DrawRoundedRect(memDC, CHK_COLOR_X, CHK_COLOR_Y, CHK_SIZE, CHK_SIZE, 4,
            g_state.doColorConvert ? RGB(59, 130, 246) : RGB(30, 50, 90));
        if (g_state.doColorConvert) {
            SetTextColor(memDC, RGB(255, 255, 255));
            RECT rCheck = { CHK_COLOR_X, CHK_COLOR_Y, CHK_COLOR_X + CHK_SIZE, CHK_COLOR_Y + CHK_SIZE };
            DrawTextW(memDC, L"✓", -1, &rCheck, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        SetTextColor(memDC, RGB(200, 220, 255));
        RECT rColorLabel = { CHK_COLOR_X + CHK_SIZE + 10, CHK_COLOR_Y,
                             CHK_COLOR_X + 200, CHK_COLOR_Y + CHK_SIZE };
        DrawTextW(memDC, L"Convert Color", -1, &rColorLabel, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        // Color mode selection (only show when color convert is checked)
        if (g_state.doColorConvert) {
            const wchar_t* colorModes[] = { L"Auto", L"BGR", L"Gray", L"RGBA" };
            for (int i = 0; i < 4; i++) {
                int btnX = CHK_COLOR_X + i * (COLOR_MODE_BTN_W + 10);
                bool isActive = (int)g_state.colorMode == i;
                COLORREF btnColor = isActive ? RGB(59, 130, 246) : RGB(30, 50, 90);
                COLORREF txtColor = isActive ? RGB(255, 255, 255) : RGB(150, 170, 200);

                DrawRoundedRect(memDC, btnX, COLOR_MODE_Y, COLOR_MODE_BTN_W, COLOR_MODE_BTN_H, 4, btnColor);

                SetTextColor(memDC, txtColor);
                RECT rMode = { btnX, COLOR_MODE_Y, btnX + COLOR_MODE_BTN_W, COLOR_MODE_Y + COLOR_MODE_BTN_H };
                DrawTextW(memDC, colorModes[i], -1, &rMode,
                    DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }
        }

        if (!g_state.doColorConvert) {
            // 3. Denoise Checkbox
            DrawRoundedRect(memDC, CHK_DENOISE_X, CHK_DENOISE_Y, CHK_SIZE, CHK_SIZE, 4,
                g_state.doDenoise ? RGB(59, 130, 246) : RGB(30, 50, 90));
            if (g_state.doDenoise) {
                SetTextColor(memDC, RGB(255, 255, 255));
                RECT rCheck = { CHK_DENOISE_X, CHK_DENOISE_Y, CHK_DENOISE_X + CHK_SIZE, CHK_DENOISE_Y + CHK_SIZE };
                DrawTextW(memDC, L"✓", -1, &rCheck, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }

            SetTextColor(memDC, RGB(200, 220, 255));
            RECT rDenoiseLabel = { CHK_DENOISE_X + CHK_SIZE + 10, CHK_DENOISE_Y,
                                   CHK_DENOISE_X + 250, CHK_DENOISE_Y + CHK_SIZE + 12 };
            DrawTextW(memDC, L"Apply Denoising", -1, &rDenoiseLabel, DT_LEFT | DT_VCENTER | DT_SINGLELINE);


            // Denoise slider (only show when denoise is checked)
            if (g_state.doDenoise) {
                // Slider track
                DrawRoundedRect(memDC, DENOISE_SLIDER_X, DENOISE_SLIDER_Y,
                    DENOISE_SLIDER_W, DENOISE_SLIDER_H, 3, RGB(40, 60, 100));

                // Slider fill
                float denoiseRatio = (float)(g_state.denoiseStrength - 1) / 9.0f;
                int denoiseFillW = (int)(denoiseRatio * DENOISE_SLIDER_W);
                DrawRoundedRect(memDC, DENOISE_SLIDER_X, DENOISE_SLIDER_Y,
                    denoiseFillW, DENOISE_SLIDER_H, 3, RGB(59, 130, 246));

                // Slider thumb
                DrawRoundedRect(memDC, DENOISE_SLIDER_X + denoiseFillW - 8,
                    DENOISE_SLIDER_Y - 5, 16, 16, 16, RGB(255, 255, 255));

                // Label
                SelectObject(memDC, hSmall);
                SetTextColor(memDC, RGB(200, 220, 255));
                RECT rDenoiseVal = { DENOISE_SLIDER_X + DENOISE_SLIDER_W + 10, DENOISE_SLIDER_Y - 10,
                    DENOISE_SLIDER_X + DENOISE_SLIDER_W + 80, DENOISE_SLIDER_Y + 20 };
                wchar_t denoiseStr[20];
                swprintf_s(denoiseStr, L"Strength: %d", g_state.denoiseStrength);
                DrawTextW(memDC, denoiseStr, -1, &rDenoiseVal,
                    DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            }

        }

   
         
        // === GPU TOGGLE BUTTON - FINAL PERFECT VERSION ===
        bool gpuAvailable = g_converter && g_converter->isGpuAvailable();
        std::wstring gpuText = gpuAvailable
            ? (g_state.useGpu ? L"GPU" : L"[GPU]")
            : L"GPU None";

        // Circle (centered vertically in button)
        int circleY = GPU_TOGGLE_Y + (GPU_TOGGLE_H - 20) / 2;  // Perfect centering!
        COLORREF circleColor = gpuAvailable
            ? (g_state.useGpu ? RGB(34, 197, 94) : RGB(40, 60, 100))
            : RGB(100, 40, 40);
        DrawRoundedRect(memDC, GPU_TOGGLE_X, circleY, 20, 20, 10, circleColor);

        // Text color + hover
        COLORREF textColor = gpuAvailable
            ? (g_state.useGpu ? RGB(34, 197, 94) : RGB(150, 170, 200))
            : RGB(200, 100, 100);
        if (g_hoverGpu && gpuAvailable)
            textColor = RGB(100, 255, 100);

        SetTextColor(memDC, textColor);
        RECT rGpuText = { GPU_TOGGLE_X + 28, GPU_TOGGLE_Y, GPU_TOGGLE_X + GPU_TOGGLE_W, GPU_TOGGLE_Y + GPU_TOGGLE_H };
        DrawTextW(memDC, gpuText.c_str(), -1, &rGpuText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        ////////////////////////////////////////////


         

         // Drop zone
        HPEN hPen = CreatePen(PS_DASH, 2, RGB(100, 150, 220));
        SelectObject(memDC, hPen);
        SelectObject(memDC, GetStockObject(NULL_BRUSH));
        // Right Settings Panel
  DrawRoundedRect(memDC, 31, PANEL_Y + 141, (WINDOW_WIDTH / 2) - 61, WINDOW_HEIGHT - 331, 20, RGB(5, 30, 60));
        RoundRect(memDC, 30, PANEL_Y + 140, (WINDOW_WIDTH / 2) - 30, WINDOW_HEIGHT - 60, 20, 20);
        DeleteObject(hPen);

        SelectObject(memDC, hBold);
        SetTextColor(memDC, RGB(155, 155, 155));
        RECT rDrop = { 0, PANEL_Y + 150, (WINDOW_WIDTH / 2) + 30, PANEL_Y + 180 };
        DrawTextW(memDC, L"Drop Images or Folders Here", -1, &rDrop, DT_CENTER | DT_SINGLELINE);


 







        ////////////////////////////////////////////
               // Show conversion progress if active
        if (g_isConverting.load()) {
            int dropX = 30;
            int dropY = PANEL_Y + 140;
            int dropW = (WINDOW_WIDTH / 2) - 60;
            int dropH = WINDOW_HEIGHT - 60 - dropY;

            int total = g_conversionTotal.load();
            int progress = g_conversionProgress.load();
            float pct = (total > 0) ? (float)progress / total : 0.0f;

            // Dark backdrop panel
            DrawRoundedRect(memDC, dropX, dropY, dropW, dropH, 20, RGB(8, 15, 35));

            // Progress bar geometry
            int barX = dropX + 40;
            int barY = dropY + 80;
            int barW = dropW - 80;
            int barH = 44;

            // === 1. OUTER GLOW (pulsing fire aura) ===
            int glowSize = 8;
            float pulse = (float)(sin(clock() * 0.006) * 0.5 + 0.5); // 0..1
            int glowAlpha = (int)(80 + pulse * 60); // 80-140

            for (int i = glowSize; i >= 1; --i) {
                COLORREF c = RGB(255, (int)(180 * i / glowSize), 0);
                HPEN pen = CreatePen(PS_SOLID, i * 2, c);
                HPEN old = (HPEN)SelectObject(memDC, pen);
                SelectObject(memDC, GetStockObject(NULL_BRUSH));
                RoundRect(memDC,
                    barX - i, barY - i,
                    barX + barW + i, barY + barH + i,
                    24, 24);
                SelectObject(memDC, old);
                DeleteObject(pen);
            }

            // === 2. MAIN FIRE FILL (multi-segment gradient) ===
            if (pct > 0.01f) {
                int fillW = (int)(barW * pct);

                // Clip to rounded corners
                HRGN clip = CreateRoundRectRgn(barX, barY, barX + fillW + 1, barY + barH + 1, 22, 22);
                SelectClipRgn(memDC, clip);

                // Animated offset for the “molten” shimmer
                int offset = (int)(clock() / 12) % 200;

                // Yellow → Orange → Red → Dark Red → Orange loop
                DrawGradientRect(memDC, barX - offset, barY, fillW + offset, barH, RGB(255, 255, 100), RGB(255, 180, 0), true);
                DrawGradientRect(memDC, barX - offset + 70, barY, fillW + offset, barH, RGB(255, 180, 0), RGB(255, 80, 0), true);
                DrawGradientRect(memDC, barX - offset + 140, barY, fillW + offset, barH, RGB(255, 80, 0), RGB(200, 0, 0), true);
                DrawGradientRect(memDC, barX - offset + 200, barY, fillW + offset, barH, RGB(200, 0, 0), RGB(255, 140, 0), true);

                // Inner bright highlight (moving hot spot)
                int hotX = barX + (int)(fillW * (0.3f + pulse * 0.4f));
                DrawGradientRect(memDC, hotX - 40, barY + 4, 80, barH - 8,
                    RGB(255, 255, 255), RGB(255, 220, 100), true);

                SelectClipRgn(memDC, NULL);
                DeleteObject(clip);
            }

            // === 3. BAR BORDER (metallic edge) ===
            //HPEN border = CreatePen(PS_SOLID, 3, RGB(255, 140, 0));
           // HPEN old = (HPEN)SelectObject(memDC, border);
            //SelectObject(memDC, GetStockObject(NULL_BRUSH));
            //RoundRect(memDC, barX, barY, barX + barW, barY + barH, 22, 22);
            //SelectObject(memDC, old);
           // DeleteObject(border);

            // === 4. TEXT – fiery style ===
            SelectObject(memDC, hBold);
            SetTextColor(memDC, RGB(255, 220, 100));

            // “Converting X / Y”
            RECT rStatus = { barX, barY - 44, barX + barW, barY - 8 };
            wchar_t txt[100];
            swprintf_s(txt, L"Converting %d / %d", progress, total);
            DrawTextW(memDC, txt, -1, &rStatus, DT_CENTER | DT_SINGLELINE);

            // Percentage – big & bold inside the bar
            SetTextColor(memDC, RGB(255, 255, 200));
            SelectObject(memDC, CreateFontW(36, 0, 0, 0, FW_BLACK, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Segoe UI"));
            RECT rPct = { barX, barY + 2, barX + barW, barY + barH - 2 };
            swprintf_s(txt, L"%d%%", (int)(pct * 100 + 0.5f));
            DrawTextW(memDC, txt, -1, &rPct, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            // Current file
            SelectObject(memDC, hSmall);
            SetTextColor(memDC, RGB(255, 180, 120));
            RECT rFile = { barX, barY + barH + 12, barX + barW, barY + barH + 34 };
            std::wstring status;
            { std::lock_guard<std::mutex> l(g_statusMutex); status = g_conversionStatus; }
            if (status.length() > 38) status = status.substr(0, 35) + L"...";
            DrawTextW(memDC, status.c_str(), -1, &rFile, DT_CENTER | DT_SINGLELINE);

            // Animated fire dots
            int dots = ((clock() - g_progressAnimTime) / 220) % 5;
            std::wstring fire = L"";
            for (int i = 0; i < dots; ++i) fire += (i & 1) ? L"●" : L"○";
            SetTextColor(memDC, RGB(255, 100, 50));
            RECT rDots = { barX, barY + barH + 38, barX + barW, barY + barH + 62 };
            DrawTextW(memDC, fire.c_str(), -1, &rDots, DT_CENTER | DT_SINGLELINE);
        }
               
        

        ///////////////////////////////////////////////////////













          
        // === RESULT OVERLAY — FINAL BEAUTIFUL CENTERED VERSION ===
        if (g_showResult) {
            if ((clock() - g_resultTime) > RESULT_DURATION) {
                g_showResult = false;
                InvalidateRect(hWnd, NULL, FALSE);
            }
            else {
                int panelX = 60;

                int panelW = WINDOW_WIDTH / 2 - 120;
                int panelH = 140;
                int panelY = PANEL_Y + 200;

                // Background panel
                DrawRoundedRect(memDC, panelX, panelY, panelW, panelH, 24, RGB(10, 35, 80));

                SelectObject(memDC, hBold);
                SetBkMode(memDC, TRANSPARENT);

                int centerX = panelX + panelW / 2;

                panelH +=100;

                // === "Done!" ===
                SetTextColor(memDC, RGB(100, 255, 150));
                RECT rDone = { panelX, panelY + 15, panelX + panelW, panelY + 45 };
                DrawTextW(memDC, L"Done!", -1, &rDone, DT_CENTER | DT_SINGLELINE);

                // === Image count ===
                SetTextColor(memDC, RGB(220, 240, 255));
                wchar_t buf[64];
                swprintf_s(buf, L"%zu images", 1+g_resultImageCount);
                RECT r1 = { panelX, panelY + 40, panelX + panelW, panelY + 60 };
                DrawTextW(memDC, buf, -1, &r1, DT_CENTER | DT_SINGLELINE);

                // === Failed count ===
                SetTextColor(memDC, g_resultFailedCount == 0 ? RGB(120, 255, 160) : RGB(255, 120, 120));
                swprintf_s(buf, g_resultFailedCount == 0 ? L"0 failed" : L"%d failed", g_resultFailedCount);
                RECT r2 = { panelX, panelY + 60, panelX + panelW, panelY + 80 };
                DrawTextW(memDC, buf, -1, &r2, DT_CENTER | DT_SINGLELINE);

                // === Time ===
                SetTextColor(memDC, RGB(180, 220, 255));
                swprintf_s(buf, L"%.1fs", g_resultDuration);
                RECT r3 = { panelX, panelY + 80, panelX + panelW, panelY + 100 };
                DrawTextW(memDC, buf, -1, &r3, DT_CENTER | DT_SINGLELINE);

                // === GPU + Speed ===
                SelectObject(memDC, hSmall);
                SetTextColor(memDC, RGB(140, 200, 255));
                double speed = g_resultDuration > 0 ? (g_resultImageCount / g_resultDuration) : 0.0;
                swprintf_s(buf, L"GPU: %s • %.1f img/s", g_resultGpuUsed ? L"Yes" : L"No", speed);
                RECT r4 = { panelX, panelY + 105, panelX + panelW, panelY + 135 };
                DrawTextW(memDC, buf, -1, &r4, DT_CENTER | DT_SINGLELINE);

                 
            }
        }
////////////////////////////////////////////////////////////////

        // Help text
        SelectObject(memDC, hSmall);
        SetTextColor(memDC, RGB(150, 200, 255));
        RECT rDesc = { SETTINGS_PANEL_X + 20, SETTINGS_PANEL_Y + SETTINGS_PANEL_H - 40,
                       SETTINGS_PANEL_X + SETTINGS_PANEL_W - 20, SETTINGS_PANEL_Y + SETTINGS_PANEL_H - 10 };
        DrawTextW(memDC, L"Settings apply to all converted images",
            -1, &rDesc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);



         






        // Cleanup fonts
        DeleteObject(hTitle);
        DeleteObject(hSub);
        DeleteObject(hBold);
        DeleteObject(hSmall);

        // Copy to screen
        BitBlt(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, memDC, 0, 0, SRCCOPY);
        DeleteObject(memBM);
        DeleteDC(memDC);
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}








INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
    case WM_INITDIALOG: return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) { EndDialog(hDlg, LOWORD(wParam)); return (INT_PTR)TRUE; }
        break;
    }
    return (INT_PTR)FALSE;
}

void SelectOutputFolder(HWND hwndOwner) {
    IFileOpenDialog* pfd = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
        IID_IFileOpenDialog, (void**)&pfd))) return;

    DWORD dwOptions = 0;
    pfd->GetOptions(&dwOptions);
    pfd->SetOptions((dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST)
        & ~FOS_FILEMUSTEXIST & ~FOS_OVERWRITEPROMPT | FOS_ALLOWMULTISELECT);

    pfd->SetTitle(L"Select output folder for converted images:");

    if (g_outputFolder[0] == L'\0') {
        PWSTR pszPath = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Pictures, 0, nullptr, &pszPath))) {
            wcscpy_s(g_outputFolder, pszPath);
            PathAddBackslashW(g_outputFolder);
            CoTaskMemFree(pszPath);
        }
    }

    if (g_outputFolder[0] != L'\0' && PathIsDirectoryW(g_outputFolder)) {
        IShellItem* pFolder = nullptr;
        if (SUCCEEDED(SHCreateItemFromParsingName(g_outputFolder, nullptr, IID_PPV_ARGS(&pFolder)))) {
            pfd->SetFolder(pFolder);
            pfd->SetDefaultFolder(pFolder);
            pFolder->Release();
        }
    }

    static const GUID guid = { 0x4E4B5853,0xA0D1,0x4201,{0x92,0x3F,0x1F,0x8F,0x2D,0x9C,0x7A,0xB1} };
    pfd->SetClientGuid(guid);

    if (SUCCEEDED(pfd->Show(hwndOwner))) {
        IShellItem* pResult = nullptr;
        if (SUCCEEDED(pfd->GetResult(&pResult))) {
            PWSTR pszPath = nullptr;
            if (SUCCEEDED(pResult->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
                g_state.outputFolder = pszPath;
                wcscpy_s(g_outputFolder, pszPath);
                PathAddBackslashW(g_outputFolder);
                CoTaskMemFree(pszPath);
            }
            pResult->Release();
        }
    }
    pfd->Release();
    InvalidateRect(g_hWnd, NULL, FALSE);
}

void EnableDarkMode() {
    auto uxtheme = LoadLibraryW(L"uxtheme.dll");
    if (uxtheme) {
        using SetPreferredAppMode = int(WINAPI*)(int);
        auto fn = (SetPreferredAppMode)GetProcAddress(uxtheme, MAKEINTRESOURCEA(135));
        if (fn) fn(2);
        FreeLibrary(uxtheme);
    }
}

void InitializePathsAndFolders()
{
    // Get full path to the running .exe (e.g. C:\Apps\QuickConvert.exe)
    WCHAR exePath[MAX_PATH] = { 0 };
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    // Remove the filename → leave only directory
    PathRemoveFileSpecW(exePath);

   
    swprintf_s(g_outputFolder, MAX_PATH, L"%s\\Converted", exePath);

    // Create the folder if it doesn't exist
    if (!PathIsDirectoryW(g_outputFolder))
    {
        CreateDirectoryW(g_outputFolder, nullptr);
    }

    // Always ensure trailing backslash
   // PathAddBackslashW(g_outputFolder);
}




void PlayGodTierAnimeEnding(HDC hdc, HWND hWnd)
{
    static clock_t startTime = 0;
    if (startTime == 0) startTime = clock();

    double t = (clock() - startTime) / (double)CLOCKS_PER_SEC;

    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBM = CreateCompatibleBitmap(hdc, WINDOW_WIDTH, WINDOW_HEIGHT);
    SelectObject(memDC, memBM);

    // Soft dark gradient background
    for (int y = 0; y < WINDOW_HEIGHT; y++) {
        int gray = 20 + (y * 40 / WINDOW_HEIGHT);
        HBRUSH b = CreateSolidBrush(RGB(10, 81, 152));
        RECT line = { 0, y, WINDOW_WIDTH, y + 1 };
        FillRect(memDC, &line, b);
        DeleteObject(b);
    }

    // Pulsing glow circle in center
    float pulse = sinf((float)t * 2.0f) * 0.5f + 0.5f;
    int glowSize = 90 + (int)(pulse * 80);  // Bigger, more powerful

    for (int i = 9; i > 0; --i) {
        // Smooth orange gradient: from deep red-orange → bright yellow-orange → white-hot core
        COLORREF outer = RGB(255, 80, 0);   // Deep orange-red
        COLORREF middle = RGB(255, 140, 0);   // Classic orange
        COLORREF inner = RGB(255, 200, 80);   // Bright orange
        COLORREF core = RGB(255, 240, 180);   // Near-white hot

        COLORREF color;
        if (i >= 7)      color = outer;
        else if (i >= 5) color = middle;
        else if (i >= 3) color = inner;
        else             color = core;

        // Add subtle pulsing brightness
        int brightness = (int)(180 + pulse * 75);
        int r = std::min(255, GetRValue(color) * brightness / 200);
        int g = std::min(255, GetGValue(color) * brightness / 280);
        int b = std::max(0, GetBValue(color) - 50);

        HPEN pen = CreatePen(PS_SOLID, i * 9, RGB(r, g, b));
        SelectObject(memDC, pen);
        SelectObject(memDC, GetStockObject(NULL_BRUSH));

        Ellipse(memDC,
            WINDOW_WIDTH / 2 - glowSize - i * 22,
            WINDOW_HEIGHT / 2 - glowSize - i * 22,
            WINDOW_WIDTH / 2 + glowSize + i * 22,
            WINDOW_HEIGHT / 2 + glowSize + i * 22
        );

        DeleteObject(pen);
    }

    // Title - elegant fade in
    float fade = std::min(1.0f, (float)t / 2.0f);
    SetTextColor(memDC, RGB((int)(255 * fade), (int)(230 * fade), (int)(200 * fade)));
    SetBkMode(memDC, TRANSPARENT);

    HFONT hBig = CreateFontW(72, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    SelectObject(memDC, hBig);
    TextOutW(memDC, WINDOW_WIDTH / 2 - 220, WINDOW_HEIGHT / 2 - 100, L"   QuickConvert ", 16);
    DeleteObject(hBig);

    // Subtext
    HFONT hMed = CreateFontW(32, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    SelectObject(memDC, hMed);
    SetTextColor(memDC, RGB((5+rand()%220), (5 + rand() % 220), (5 + rand() % 220)));
    TextOutW(memDC, WINDOW_WIDTH / 2  -30 , WINDOW_HEIGHT / 2 - 20, L"CLASS ", 6);

    // Free & Open Source + Wiki link
    SetTextColor(memDC, RGB(100, 180, 255));
    HFONT hLink = CreateFontW(28, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    SelectObject(memDC, hLink);
    TextOutW(memDC, WINDOW_WIDTH / 2 - 130, WINDOW_HEIGHT / 2 + 40, L"Freeware & Open Source • As-IS ", 31);

   

    DeleteObject(hMed);
    DeleteObject(hLink);

    // Final instruction
    if (t > 6.0f) {
        float blink = (sinf((float)t * 8) > 0.7f) ? 1.0f : 0.6f;
        SetTextColor(memDC, RGB((int)(255 * blink), (int)(255 * blink), (int)(255 * blink)));
        HFONT hSmall = CreateFontW(24, 0, 0, 0, FW_LIGHT, TRUE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        SelectObject(memDC, hSmall);
        TextOutW(memDC, WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT - 100, L"Click anywhere to continue ", 27);
        DeleteObject(hSmall);

        startTime = 0;
        g_inAnimeMode = false;
        InvalidateRect(hWnd, nullptr, TRUE);  
        UpdateWindow(hWnd);
    }

    BitBlt(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, memDC, 0, 0, SRCCOPY);

    DeleteObject(memBM);
    DeleteDC(memDC);

    // Keep animating until mouse click
    InvalidateRect(hWnd, nullptr, FALSE);
   
}