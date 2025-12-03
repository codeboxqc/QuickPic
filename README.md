[quickconvert_readme.md](https://github.com/user-attachments/files/23884969/quickconvert_readme.md)
# ⚡ QuickConvert

**High-performance batch image converter with GPU acceleration**

![Version](https://img.shields.io/badge/version-1.0-blue.svg)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey.svg)
![License](https://img.shields.io/badge/license-Freeware-green.svg)
![GPU](https://img.shields.io/badge/GPU-CUDA%20%7C%20OpenCL-orange.svg)

---
![Untitled](https://github.com/user-attachments/assets/ba7f99ec-08bc-43a8-97fe-aeb1bcbbbccf)

https://github.com/user-attachments/assets/f79fc295-9bdb-428f-a335-f2a1192a45ce



## 📋 Table of Contents

- [Features](#-features)
- [Screenshots](#-screenshots)
- [System Requirements](#-system-requirements)
- [Installation](#-installation)
- [Building from Source](#-building-from-source)
- [Usage Guide](#-usage-guide)
- [Technical Details](#-technical-details)
- [Troubleshooting](#-troubleshooting)
- [Contributing](#-contributing)
- [License](#-license)

---

## ✨ Features

### Core Capabilities
- **Batch Processing** - Convert hundreds of images in seconds
- **GPU Acceleration** - CUDA and OpenCL support for 5-10x faster processing
- **Multiple Formats** - JPG, PNG, WEBP, TIF, BMP, GIF
- **Drag & Drop** - Simple, intuitive interface
- **Recursive Scanning** - Automatically processes entire folder structures

### Image Processing Options
- **Smart Resize** - Scale images by percentage (10-100%)
- **Color Conversion** - Auto-detect, BGR, Grayscale, RGBA modes
- **Denoise Filter** - Advanced non-local means denoising with adjustable strength
- **Quality Control** - Adjustable compression quality (10-100%)

### Performance
- **Multi-threaded** - Utilizes all CPU cores
- **Mixed Pipeline** - Intelligently switches between GPU/CPU based on task
- **Real-time Progress** - Live conversion status with animated progress bar
- **Smart Output** - Automatic file naming prevents overwrites

---

## 📸 Screenshots

```
[Main Interface]
┌─────────────────────────────────────────────────────┐
│ ⚡ QuickConvert                              📁 ✕  │
│ High-Quality • GPU Detected • Batch Processing      │
├─────────────────────────────────────────────────────┤
│                                                     │
│  Quality: 95%  ████████████████░░  JPG PNG WEBP   │
│                                     TIF BMP GIF    │
│                                                     │
│  ┌─────────────────────┐  ┌───────────────────┐   │
│  │ Drop Images or      │  │ Settings Panel    │   │
│  │ Folders Here        │  │ ☑ Resize: 100%    │   │
│  │                     │  │ ☐ Color Convert   │   │
│  │    [Progress Bar]   │  │ ☐ Denoise         │   │
│  └─────────────────────┘  └───────────────────┘   │
│                                                     │
│  Output: C:\Converted                    GPU: ON   │
└─────────────────────────────────────────────────────┘
```

---

## 💻 System Requirements

### Minimum
- **OS**: Windows 10 (64-bit) or later
- **CPU**: Dual-core processor
- **RAM**: 2 GB
- **Disk**: 100 MB free space

### Recommended
- **OS**: Windows 11 (64-bit)
- **CPU**: Quad-core processor or better
- **RAM**: 8 GB or more
- **GPU**: NVIDIA GPU with CUDA support (GTX 900 series or newer)
  - OR AMD/Intel GPU with OpenCL support

### Supported GPUs
- **NVIDIA**: GTX 900+ series, RTX series (CUDA)
- **AMD**: Radeon RX 400+ series (OpenCL)
- **Intel**: Iris Xe, Arc series (OpenCL)

---

## 📦 Installation

### Quick Start (Pre-built Binary)

1. Download the latest release from  https://github.com/codeboxqc/QuickPic/releases/tag/1
2. Extract `QuickConvert.zip` to your desired location
3. Run `QuickConvert.exe`
4. (Optional) Create a desktop shortcut

**No installation required!** Just extract and run.

---

## 🛠️ Building from Source

### Prerequisites

#### Required Software
1. **Visual Studio 2022** (Community Edition or higher)
   - Download: https://visualstudio.microsoft.com/
   - Workloads required:
     - "Desktop development with C++"
     - Windows 10 SDK (10.0.19041.0 or later)

2. **OpenCV 4.13.0** with CUDA support
   - Download pre-built: https://opencv.org/releases/
   - OR build from source with CUDA enabled (see below)

3. **CUDA Toolkit 12.x** (for GPU acceleration)
   - Download: https://developer.nvidia.com/cuda-downloads
   - Required even if you only use OpenCL (for headers)

#### Optional
- **CMake 3.20+** (if building OpenCV from source)
- **Git** for cloning the repository

---

### Step 1: Install Visual Studio 2022

1. Download Visual Studio 2022 Community
2. Run installer and select:
   - ✅ Desktop development with C++
   - ✅ C++ CMake tools for Windows
   - ✅ Windows 10 SDK (latest version)
3. Complete installation (requires ~7 GB)

---

### Step 2: Install CUDA Toolkit

1. Download CUDA Toolkit 12.x from NVIDIA
2. Run installer with default settings
3. Verify installation:
   ```cmd
   nvcc --version
   ```
   Should output: `Cuda compilation tools, release 12.x`

---

### Step 3: Install/Build OpenCV

#### Option A: Use Pre-built OpenCV (Easier)

1. Download OpenCV 4.13.0 from https://opencv.org/releases/
2. Extract to `C:\opencv`
3. Your directory structure should be:
   ```
   C:\opencv\
   ├── build\
   │   ├── x64\
   │   │   └── vc16\
   │   │       ├── bin\
   │   │       │   └── opencv_world4130.dll
   │   │       └── lib\
   │   │           └── opencv_world4130.lib
   │   └── include\
   │       └── opencv2\
   └── sources\
   ```

#### Option B: Build OpenCV with CUDA (Advanced)

```cmd
# Clone OpenCV
git clone https://github.com/opencv/opencv.git
git clone https://github.com/opencv/opencv_contrib.git
cd opencv
git checkout 4.13.0
cd ../opencv_contrib
git checkout 4.13.0

# Create build directory
cd ../opencv
mkdir build
cd build

# Configure with CMake
cmake -G "Visual Studio 17 2022" -A x64 ^
  -DOPENCV_EXTRA_MODULES_PATH=../../opencv_contrib/modules ^
  -DWITH_CUDA=ON ^
  -DCUDA_FAST_MATH=ON ^
  -DWITH_CUBLAS=ON ^
  -DCUDA_ARCH_BIN="6.1;7.5;8.6;8.9" ^
  -DWITH_OPENCL=ON ^
  -DBUILD_opencv_world=ON ^
  -DBUILD_EXAMPLES=OFF ^
  -DBUILD_TESTS=OFF ^
  ..

or

cmake -S C:\src\opencv -B C:\build\opencv -G "Visual Studio 17 2022" -A x64 `
  -DBUILD_SHARED_LIBS=ON `
  -DWITH_CUDA=ON `
  -DBUILD_opencv_world=ON `
  -DOPENCV_EXTRA_MODULES_PATH=C:\src\opencv_contrib\modules `
  -DBUILD_LIST=core,imgproc,imgcodecs,highgui,photo,cudaarithm,cudafilters,cudaimgproc,cudawarping,cudev `
  -DCMAKE_INSTALL_PREFIX=C:\build\opencv\install



# Build (takes 30-60 minutes)
cmake --build . --config Release --parallel 8

# Install to C:\opencv
cmake --install . --prefix C:/opencv
```

---

### Step 4: Configure Environment Variables

1. Open System Properties → Advanced → Environment Variables
2. Add to **System Variables**:
   - Variable: `OPENCV_DIR`
   - Value: `C:\opencv\build`

3. Add to **Path** variable:
   - `C:\opencv\build\x64\vc16\bin`
   - `C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.x\bin`

4. Click OK and restart any open command prompts

---

### Step 5: Clone QuickConvert Repository

```cmd
git clone https://github.com/yourusername/quickconvert.git
cd quickconvert
```

---

### Step 6: Configure Visual Studio Project

1. Open `Quick.sln` in Visual Studio 2022

2. Right-click project → **Properties**

3. **C/C++ → General → Additional Include Directories**:
   ```
   C:\opencv\build\include
   $(CUDA_PATH)\include
   ```

4. **Linker → General → Additional Library Directories**:
   ```
   C:\opencv\build\x64\vc16\lib
   $(CUDA_PATH)\lib\x64
   ```

5. **Linker → Input → Additional Dependencies**:
   ```
   opencv_world4130.lib
   cudart.lib
   ```

6. **Build Events → Post-Build Event** (auto-copy DLLs):
   ```cmd
   xcopy /Y /D "C:\opencv\build\x64\vc16\bin\opencv_world4130.dll" "$(OutDir)"
   xcopy /Y /D "C:\opencv\build\x64\vc16\bin\opencv_videoio_ffmpeg4130_64.dll" "$(OutDir)"
   ```

---

### Step 7: Build the Project

1. Select **Release** configuration and **x64** platform
2. Build → **Rebuild Solution** (Ctrl+Shift+B)
3. Check for errors in Output window
4. Executable will be in: `x64\Release\QuickConvert.exe`

---

### Step 8: Test the Build

1. Copy required DLLs to executable directory:
   ```cmd
   copy "C:\opencv\build\x64\vc16\bin\*.dll" "x64\Release\"
   ```

2. Run `QuickConvert.exe`
3. Check if GPU is detected (shown in subtitle)
4. Drop a test image to verify conversion works

---

## 📖 Usage Guide

### Basic Conversion

1. **Launch QuickConvert**
2. **Set Output Folder** (click 📁 icon)
3. **Adjust Quality** (slider, 10-100%)
4. **Select Format** (JPG, PNG, WEBP, etc.)
5. **Drag & Drop** images or folders
6. **Wait for completion** (progress bar + popup result)

### Advanced Settings

#### Resize Images
1. ☑ Enable "Resize" checkbox
2. Enter percentage (10-100%)
3. Example: `50` = half size, `200` = not allowed (max 100%)

#### Color Conversion
1. ☑ Enable "Convert Color" checkbox
2. Select mode:
   - **Auto** - Keep original format
   - **BGR** - Standard RGB (3 channels)
   - **Gray
