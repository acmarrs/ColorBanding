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

#pragma once

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include "UI.h"

// Helper to display a (?) mark that shows a tooltip when hovered
static void ShowHelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void CreateDebugWindow(D3D12Global &d3d, BandingConstants &constants, bool &animateLight)
{
    bool useDitheringCheckBox = constants.useDithering;
    bool showNoiseCheckBox = constants.showNoise;
    bool useTonemappingCheckBox = constants.useTonemapping;
    bool useTriangularDistribution = constants.distributionType;

    ImGui::SetNextWindowSize(ImVec2(340, 0));
    ImGui::Begin("Debug Options and Performance", NULL, ImGuiWindowFlags_NoResize);
    ImGui::Text("Frame Time Average: %.3f ms/frame (%.1f FPS) ", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate); 
    ImGui::Text("Frame Number: %i", constants.frameNumber);
    ImGui::Checkbox("Vsync", &d3d.vsync);
    ImGui::SameLine(); ShowHelpMarker("Enable or disable vertical sync");
    ImGui::Checkbox("Animate Light", &animateLight);

    if (ImGui::Checkbox("Enable Tonemapping", &useTonemappingCheckBox))
    {
        constants.useTonemapping = useTonemappingCheckBox ? 1 : 0;
    }
    ImGui::SameLine(); ShowHelpMarker("Enable or disable tonemapping");

    if (ImGui::Checkbox("Enable Dithering", &useDitheringCheckBox))
    {
        constants.useDithering = useDitheringCheckBox ? 1 : 0;
    }
    ImGui::SameLine(); ShowHelpMarker("Enable or disable dithering using various noise techniques");
    if (constants.useDithering)
    {
        ImGui::Separator();
        ImGui::RadioButton("White Noise", &constants.noiseType, 0);
        if (constants.noiseType == 0)
        {
            ImGui::SetCursorPosX(30);
            if (ImGui::Checkbox("Use Triangular Distribution", &useTriangularDistribution))
            {
                constants.distributionType = useTriangularDistribution ? 1 : 0;
            }
        }
        
        ImGui::RadioButton("Blue Noise", &constants.noiseType, 1);
        if (constants.noiseType == 1)
        {
            ImGui::SetCursorPosX(30);
            if (ImGui::Checkbox("Use Triangular Distribution", &useTriangularDistribution))
            {
                constants.distributionType = useTriangularDistribution ? 1 : 0;
            }
        }

        ImGui::RadioButton("LDS Blue Noise", &constants.noiseType, 2);
        if (constants.noiseType == 2)
        {
            ImGui::SetCursorPosX(30);
            if (ImGui::Checkbox("Use Triangular Distribution", &useTriangularDistribution))
            {
                constants.distributionType = useTriangularDistribution ? 1 : 0;
            }
        }

        if (ImGui::Checkbox("Show Noise", &showNoiseCheckBox))
        {
            constants.showNoise = showNoiseCheckBox ? 1 : 0;
        }

        if (constants.showNoise)
        {
            constants.noiseScale = 1.f;
        }
        else 
        {
            if(constants.noiseScale == 1.f) constants.noiseScale = (1.f / 256.f);
            ImGui::SliderFloat("Noise Scale", &constants.noiseScale, 0.f, 0.008f, "%.5f");
            ImGui::SameLine(); ShowHelpMarker("Change the magnitude of the noise");
        }
    }
    ImGui::SetWindowPos("Debug Options and Performance", ImVec2((d3d.width - ImGui::GetWindowWidth() - 10.f), 10));
    ImGui::End();
}

 //--------------------------------------------------------------------------------------
 // UI Functions
 //--------------------------------------------------------------------------------------

namespace UI
{

    void Init(HWND &window, D3D12Global &d3d, D3D12Resources &resources)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.DisplaySize = ImVec2((float)d3d.width, (float)d3d.height);
        io.IniFilename = NULL;

        ImGui::StyleColorsDark();

        ImGui_ImplWin32_Init(window);
        ImGui_ImplDX12_Init(
            d3d.device,
            2,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            resources.uiDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
            resources.uiDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    }

    void Build_CmdList(D3D12Global &d3d, D3D12Resources &resources, BandingConstants &constants, bool &animateLight)
    {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        CreateDebugWindow(d3d, constants, animateLight);

        // Transition the back buffer to a render target (from present)
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = d3d.backBuffer[d3d.frameIndex];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        d3d.cmdList->ResourceBarrier(1, &barrier);
        d3d.cmdList->OMSetRenderTargets(1, &d3d.backBufferRTV[d3d.frameIndex], false, nullptr);
        d3d.cmdList->SetDescriptorHeaps(1, &resources.uiDescriptorHeap);

        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), d3d.cmdList);

        // Transition the back buffer to present (from render target)
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        d3d.cmdList->ResourceBarrier(1, &barrier);
    }

    void Destroy()
    {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

}
