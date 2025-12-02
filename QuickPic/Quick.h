#pragma once

#include <windows.h>

#include "resource.h"
#include "targetver.h"

void InitializeConverter(char* exePath);

// Process a dropped file or folder path
void ProcessDroppedPath(const wchar_t* path, HWND hwnd);

// Cleanup resources
void ShutdownConverter(); 

void set(); 
int init();


