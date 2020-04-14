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

#include "Utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <fstream>
#include <shellapi.h>

using namespace std;

namespace Utils
{

//--------------------------------------------------------------------------------------
// Command Line Parser
//--------------------------------------------------------------------------------------

HRESULT ParseCommandLine(LPWSTR lpCmdLine, ConfigInfo &config)
{
    LPWSTR* argv = NULL;
    int argc = 0;

    argv = CommandLineToArgvW(GetCommandLine(), &argc);
    if (argv == NULL)
    {
        MessageBox(NULL, L"Unable to parse command line!", L"Error", MB_OK);
        return E_FAIL;
    }

    if (argc > 1)
    {
        char str[256];
        int i = 1;
        while (i < argc)
        {
            wcstombs(str, argv[i], 256);

            if (strcmp(str, "-width") == 0)
            {
                i++;
                wcstombs(str, argv[i], 256);
                config.width = atoi(str);
                i++;
                continue;
            }

            if (strcmp(str, "-height") == 0)
            {
                i++;
                wcstombs(str, argv[i], 256);
                config.height = atoi(str);
                i++;
                continue;
            }

            if (strcmp(str, "-vsync") == 0)
            {
                i++;
                wcstombs(str, argv[i], 256);
                config.vsync = (atoi(str) > 0);
                i++;
                continue;
            }

            i++;
        }
    }
    else {
        MessageBox(NULL, L"Incorrect command line usage!", L"Error", MB_OK);
        return E_FAIL;
    }

    LocalFree(argv);
    return S_OK;
}

//--------------------------------------------------------------------------------------
// Error Messaging
//--------------------------------------------------------------------------------------

void Validate(HRESULT hr, LPWSTR msg)
{
    if (FAILED(hr))
    {
        MessageBox(NULL, msg, L"Error", MB_OK);
        PostQuitMessage(EXIT_FAILURE);
    }
}

//--------------------------------------------------------------------------------------
// File Reading
//--------------------------------------------------------------------------------------

vector<char> ReadFile(const string &filename)
{
    ifstream file(filename, ios::ate | ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("Error: failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------

/**
* Format the loaded texture into the layout we use with D3D12.
*/
void FormatTexture(TextureInfo &info, UINT8* pixels)
{
    const UINT numPixels = (info.width * info.height);
    const UINT oldStride = info.stride;
    const UINT oldSize = (numPixels * info.stride);

    const UINT newStride = 4;                // uploading textures to GPU as DXGI_FORMAT_R8G8B8A8_UNORM
    const UINT newSize = (numPixels * newStride);
    info.pixels.resize(newSize);

    for (UINT i = 0; i < numPixels; i++)
    {
        info.pixels[i * newStride] = pixels[i * oldStride];            // R
        info.pixels[i * newStride + 1] = pixels[i * oldStride + 1];    // G
        info.pixels[i * newStride + 2] = pixels[i * oldStride + 2];    // B
        info.pixels[i * newStride + 3] = 0xFF;                         // A (always 1)
    }

    info.stride = newStride;
}

/**
* Load an image from disk.
*/
TextureInfo LoadTexture(string filepath)
{
    TextureInfo result = {};

    // Load image pixels with stb_image
    UINT8* pixels = stbi_load(filepath.c_str(), &result.width, &result.height, &result.stride, STBI_default);
    if (!pixels)
    {
        throw runtime_error("Error: failed to load image!");
    }

    FormatTexture(result, pixels);
    stbi_image_free(pixels);
    return result;
}

}
