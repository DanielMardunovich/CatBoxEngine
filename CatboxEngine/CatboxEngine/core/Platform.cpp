#include "Platform.h"
#include <glfw3.h>
#include <iostream>
#include <cstring>
#if defined(_WIN32)
#include <windows.h>
#include <commdlg.h>
#endif

// static drop callback: place first dropped path into clipboard so Engine can poll it
static void DropCallback(GLFWwindow* window, int count, const char** paths)
{
    if (count > 0 && paths[0])
    {
        glfwSetClipboardString(window, paths[0]);
    }
}

bool Platform::Init(int width, int height, const char* title)
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    return true;
}

void Platform::Shutdown()
{
    if (window)
    {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}

void Platform::InstallDropCallback(GLFWwindow* window)
{
    if (!window) return;
    glfwSetDropCallback(window, DropCallback);
}

bool Platform::OpenFileDialog(char* outPath, int maxLen, const char* filter)
{
#if defined(_WIN32)
    OPENFILENAMEA ofn;
    CHAR szFile[1024];
    memset(&ofn, 0, sizeof(ofn));
    memset(szFile, 0, sizeof(szFile));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = (DWORD)sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (GetOpenFileNameA(&ofn))
    {
        #if defined(_MSC_VER)
            strncpy_s(outPath, maxLen, ofn.lpstrFile, _TRUNCATE);
        #else
            strncpy(outPath, ofn.lpstrFile, maxLen-1);
            outPath[maxLen-1] = '\0';
        #endif
        return true;
    }
    return false;
#else
    (void)outPath; (void)maxLen; (void)filter; return false;
#endif
}

bool Platform::SaveFileDialog(char* outPath, int maxLen, const char* filter)
{
#if defined(_WIN32)
    OPENFILENAMEA ofn;
    CHAR szFile[1024];
    memset(&ofn, 0, sizeof(ofn));
    memset(szFile, 0, sizeof(szFile));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = (DWORD)sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = "scene";
    ofn.Flags = OFN_OVERWRITEPROMPT;
    if (GetSaveFileNameA(&ofn))
    {
        #if defined(_MSC_VER)
            strncpy_s(outPath, maxLen, ofn.lpstrFile, _TRUNCATE);
        #else
            strncpy(outPath, ofn.lpstrFile, maxLen-1);
            outPath[maxLen-1] = '\0';
        #endif
        return true;
    }
    return false;
#else
    (void)outPath; (void)maxLen; (void)filter; return false;
#endif
}
