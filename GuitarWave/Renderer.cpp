#include "renderer.h"

#include <Windows.h>
#include <Wingdi.h>
#include "OpenGL.h"

#pragma comment(lib, "OpenGL32.lib")

namespace
{
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
    PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
}

LRESULT CALLBACK WinProcCallback(HWND hwnd, UINT uMSG, WPARAM wParam, LPARAM lParam)
{
    switch (uMSG)
    {
    case WM_CLOSE:
    {
        break;
    }

    case WM_QUIT:
    {
        break;
    }

    case WM_SIZE:
    {
        std::size_t width = LOWORD(lParam);
        std::size_t height = HIWORD(lParam);
        PostMessage(hwnd, WM_PAINT, 0, 0);
        return 0;
    }

    case WM_KEYDOWN:
    {
        break;
    }

    case WM_KEYUP:
    {
        break;
    }

    case WM_ACTIVATE:
    {
        break;
    }
    }

    return DefWindowProc(hwnd, uMSG, wParam, lParam);
}

Renderer::Renderer()
{
    className = L"GuitarName";
    running = false;
}

void* GetAnyGLFuncAddress(const char* name)
{
    void* p = (void*)wglGetProcAddress(name);
    if (p == 0 ||
        (p == (void*)0x1) || (p == (void*)0x2) || (p == (void*)0x3) ||
        (p == (void*)-1))
    {
        HMODULE module = LoadLibraryA("opengl32.dll");

        if (module != 0)
        {
            p = (void*)GetProcAddress(module, name);
        }
    }

    return p;
}

void Renderer::Init(HINSTANCE hInstance)
{
    PIXELFORMATDESCRIPTOR pfd;
    ZeroMemory(&pfd, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 16;
    pfd.cStencilBits = 8;
    pfd.cAlphaBits = 0;
    pfd.iLayerType = PFD_MAIN_PLANE;

    const wchar_t dummyClassName[] = L"DummyClass";

    HGLRC dummyContext = 0;
    HWND dummyWindow = 0;
    WNDCLASSEX dummyWindowClass =
    {
        sizeof(WNDCLASSEX),                 // cbSize
        CS_HREDRAW | CS_VREDRAW | CS_OWNDC, // style
        WinProcCallback,                    // lpfnWndProc
        0,                                  // cbClsExtra
        0,                                  // cbWndExtra
        hInstance,                          // hInstance
        0,                                  // hIcon
        LoadCursor(NULL, IDC_ARROW),        // hCursor
        (HBRUSH)(COLOR_WINDOW + 1),         // hbrBackground
        0,                                  // lpszMenuName
        dummyClassName,                     // lpszClassName
        0                                   // hIconSm
    };

    RegisterClassEx(&dummyWindowClass);

    dummyWindow = CreateWindowEx(
        0,
        dummyClassName,
        L"Dummy Window Text",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    HDC dummyDeviceContext = GetDC(dummyWindow);
    int dummyPixelFormat = ChoosePixelFormat(dummyDeviceContext, &pfd);
    SetPixelFormat(dummyDeviceContext, dummyPixelFormat, &pfd);

    dummyContext = wglCreateContext(dummyDeviceContext);
    wglMakeCurrent(dummyDeviceContext, dummyContext);

    LoadOpenGLFunctions();

    wglDeleteContext(dummyContext);
    DestroyWindow(dummyWindow);
    UnregisterClass(dummyClassName, hInstance);

    WNDCLASSEX windowClass =
    {
        sizeof(WNDCLASSEX),                 // cbSize
        CS_HREDRAW | CS_VREDRAW | CS_OWNDC, // style
        WinProcCallback,                    // lpfnWndProc
        0,                                  // cbClsExtra
        0,                                  // cbWndExtra
        hInstance,                          // hInstance
        0,                                  // hIcon
        LoadCursor(NULL, IDC_ARROW),        // hCursor
        (HBRUSH)(COLOR_WINDOW + 1),         // hbrBackground
        0,                                  // lpszMenuName
        className.c_str(),                  // lpszClassName
        0                                   // hIconSm
    };

    ATOM registration = RegisterClassEx(&windowClass);

    UINT32 styleEx;
    UINT32 style;

    style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    styleEx = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;

    RECT rect = { 0, 0, 480, 640 };
    AdjustWindowRectEx(&rect, style, FALSE, styleEx);

    HWND m_WindowHandle = CreateWindowEx(
        0,
        className.c_str(),
        L"Guitar Wave",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    m_DeviceContext = GetDC(m_WindowHandle);

    int contextAttribs[] =
    {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 5,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#ifdef _DEBUG
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#else
        WGL_CONTEXT_FLAGS_ARB, 0,
#endif
            0
    };

    int pfattribs[] =
    {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB, 32,
        WGL_DEPTH_BITS_ARB, 24,
        WGL_STENCIL_BITS_ARB, 8,
        0
    };

    unsigned int formatcount;
    int pixelFormat = 1;

    BOOL choosePixelFormatResult = wglChoosePixelFormatARB(m_DeviceContext, pfattribs, nullptr, 1, (int*)&pixelFormat, &formatcount);
    BOOL setPixelResult = SetPixelFormat(m_DeviceContext, pixelFormat, &pfd);
    HGLRC openGLContext = wglCreateContextAttribsARB(m_DeviceContext, nullptr, contextAttribs);

    GLint major, minor;
#define GL_MAJOR_VERSION                  0x821B
#define GL_MINOR_VERSION                  0x821C

    wglMakeCurrent(m_DeviceContext, openGLContext);

    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);

    ShowWindow(m_WindowHandle, 1);
    SetForegroundWindow(m_WindowHandle);
    SetFocus(m_WindowHandle);

    running = true;
}

void Renderer::Render()
{
    MSG message;
    while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE) > 0)
    {
        TranslateMessage(&message);

        DispatchMessage(&message);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, (GLsizei)640, (GLsizei)480);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glDisable(GL_BLEND);

    glClearColor(0.0f, 0.-f, 0.0f, 0.0f);

    SwapBuffers(m_DeviceContext);
}

void Renderer::LoadOpenGLFunctions()
{
    wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)GetAnyGLFuncAddress("wglCreateContextAttribsARB");
    glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)GetAnyGLFuncAddress("glBindFramebuffer");
    wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)GetAnyGLFuncAddress("wglChoosePixelFormatARB");
}
