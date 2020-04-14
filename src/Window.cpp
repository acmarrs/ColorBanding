/* Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Window.h"

#include "imgui.h"
#include "imgui_impl_win32.h"

#include <iostream>

/**
 * Windows message loop.
 */
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam)) return true;

    switch(message)
    {
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
                return 0;
        case WM_KEYUP:
            if (wParam == VK_ESCAPE) PostQuitMessage(0);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

namespace Window
{

/**
* Create a new window.
*/
HRESULT Create(LONG width, LONG height, HINSTANCE &instance, HWND &window, LPCWSTR title)
{
    // Register the window class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = instance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"WindowClass";
    wcex.hIcon = nullptr;
    wcex.hIconSm = nullptr;

    if (!RegisterClassEx(&wcex))
    {
        throw std::runtime_error("Failed to register window!");
    }

    // Get the desktop resolution
    RECT desktop;
    const HWND hDesktop = GetDesktopWindow();
    GetWindowRect(hDesktop, &desktop);

    int x = (desktop.right - width) / 2;
    int y = (desktop.bottom - height) / 2;

    // Create the window
    RECT rc = { 0, 0, width, height };
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    window = CreateWindow(wcex.lpszClassName, title, WS_OVERLAPPEDWINDOW, x, 0, (rc.right - rc.left), (rc.bottom - rc.top), NULL, NULL, instance, NULL);
    if (!window) return E_FAIL;

    // Set the window icon
    HANDLE hIcon = LoadImageA(GetModuleHandle(NULL), "nvidia.ico", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
    SendMessage(window, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

    // Show the window
    ShowWindow(window, SW_SHOWDEFAULT);
    UpdateWindow(window);

    return S_OK;
}

}
