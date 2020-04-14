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
#include "Graphics.h"
#include "UI.h"
#include "Utils.h"

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif


class D3D12Application
{
public:

    void Init(ConfigInfo &config)
    {
        // Create a new window
        HRESULT hr = Window::Create(config.width, config.height, config.instance, window, L"Color Banding and Dithering");
        Utils::Validate(hr, L"Error: failed to create window!");

        // Initialize command line settings
        d3d.width = config.width;
        d3d.height = config.height;
        d3d.vsync = config.vsync;

        // Initialize constants
        constants.lightPosition = DirectX::XMFLOAT3((float)d3d.width / 2.f, 50.f, (float)d3d.height / 2.f);
        constants.color = DirectX::XMFLOAT3(0.04f, 0.3f, 1.f);
        constants.resolutionX = d3d.width;
        constants.frameNumber = 1;
        constants.useDithering = 1;
        constants.showNoise = 0;
        constants.noiseType = 0;
        constants.distributionType = 0;
        constants.useTonemapping = 1;

        // 8-bits provides 256 possible values (per channel), so the maximum difference between
        // any two colors is 1/256 (again, per channel). We insert noise into each channel in the range [0, 1/256]
        // to approximate values between the range representable by the 8-bit format.
        constants.noiseScale = (1.f / 256.f);

        // Initialize the dxc shader compiler
        D3DShaders::Init_Shader_Compiler(shaderCompiler);

        // Initialize D3D12
        D3D12::Create_Device(d3d);
        D3D12::Create_Command_Queue(d3d);
        D3D12::Create_Command_Allocator(d3d);
        D3D12::Create_CommandList(d3d);
        D3D12::Create_Viewport(d3d);
        D3D12::Create_Scissor(d3d);
        D3D12::Create_SwapChain(d3d, window);
        D3D12::Create_Fence(d3d);
        D3D12::Reset_CommandList(d3d);

        // Create common resources
        D3DResources::Create_Descriptor_Heaps(d3d, resources);
        D3DResources::Create_BackBuffer_RTV(d3d, resources);
        D3DResources::Load_Shaders(resources, shaderCompiler);
        D3DResources::Create_PSO(d3d, resources);
        D3DResources::Create_ConstantBuffer(d3d, resources, constants);

        // Initialize the UI
        UI::Init(window, d3d, resources);

        // Load blue noise textures
        D3DResources::Load_Blue_Noise_Texture_Array(d3d, resources, 64);
        D3DResources::Load_Blue_Noise_Texture(d3d, resources);

        d3d.cmdList->Close();
        ID3D12CommandList* pGraphicsList = { d3d.cmdList };
        d3d.cmdQueue->ExecuteCommandLists(1, &pGraphicsList);

        D3D12::WaitForGPU(d3d);
        D3D12::Reset_CommandList(d3d);
    }
    
    void Update()
    {
        if (animateLight)
        {
            constants.lightPosition.x = (d3d.width / 2) + 200.f * cos(angle);
            constants.lightPosition.y = 50.f + 30.f * sin(angle);
            constants.lightPosition.z = (d3d.height / 2) + 200.f * sin(angle);
            
            if(d3d.vsync) angle += 0.01f;
            else angle += 0.001f;
        }

        memcpy(resources.bandingCBStart, &constants, sizeof(BandingConstants));

        constants.frameNumber++;
    }

    void Render()
    {
        D3D12::Build_CmdList(d3d, resources);
        UI::Build_CmdList(d3d, resources, constants, animateLight);

        D3D12::Submit_CmdList(d3d);
        D3D12::WaitForGPU(d3d);

        D3D12::Present(d3d);
        D3D12::MoveToNextFrame(d3d);
        D3D12::Reset_CommandList(d3d);
    }

    void Cleanup() 
    {
        D3D12::WaitForGPU(d3d);
        CloseHandle(d3d.fenceEvent);

        UI::Destroy();
        D3DResources::Destroy(resources);
        D3DShaders::Destroy(shaderCompiler);
        D3D12::Destroy(d3d);

        DestroyWindow(window);
    }
    
private:
    HWND window;
    D3D12Global d3d = {};
    D3D12Resources resources = {};
    BandingConstants constants = {};
    D3D12ShaderCompilerInfo shaderCompiler;

    float angle = 0.f;
    bool animateLight = false;
};

/**
 * Program entry point
 */
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{    
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    HRESULT hr = EXIT_SUCCESS;
    {
        MSG msg = { 0 };

        // Get the application configuration
        ConfigInfo config;
        hr = Utils::ParseCommandLine(lpCmdLine, config);
        if (hr != EXIT_SUCCESS) return hr;

        // Initialize
        D3D12Application app;
        app.Init(config);

        // Main loop
        while (WM_QUIT != msg.message)
        {
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            app.Update();
            app.Render();
        }

        app.Cleanup();
    }

#if defined _CRTDBG_MAP_ALLOC
    _CrtDumpMemoryLeaks();
#endif

    return hr;
}