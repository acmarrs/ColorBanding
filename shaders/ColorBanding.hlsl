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

#include "Common.hlsl"

// ---[ Structures ]---

struct PSInput
{
    float4 position : SV_POSITION;
    float2 tex0        : TEXCOORD;
};

// ---[ Resources ]---

cbuffer BandingConstants : register(b0)
{
    float3  lightPosition;
    float   noiseScale;
    float3  color;
    uint    resolutionX;
    uint    frameNumber;
    int     useDithering;
    int     showNoise;
    int     noiseType;
    int     distributionType;
    int     useTonemapping;
    int2    pad;
};

Texture2D<float4> blueNoise : register(t0);
Texture2DArray<float4> blueNoiseArray : register(t1);

// ---[ Vertex Shader ]---

PSInput VS(uint VertID : SV_VertexID)
{
    float div2 = (VertID / 2);
    float mod2 = (VertID % 2);

    PSInput result;
    result.position.x = (mod2 * 4.f) - 1.f;
    result.position.y = (div2 * 4.f) - 1.f;
    result.position.zw = float2(0.f, 1.f);

    result.tex0.x = mod2 * 2.f;
    result.tex0.y = 1.f - (div2 * 2.f);

    return result;
}

/**
* Generate three components of white noise in image-space.
*/
float3 GetWhiteNoise(uint2 position, uint width, uint frame, uint distribution, float scale)
{
    // Generate a unique seed based on:
    // space - this thread's (x, y) position in the image
    // time  - the current frame number
    uint seed = ((position.y * width) + position.x) * frame;

    // Generate three uniformly distributed random values in the range [0, 1]
    float3 rnd;
    rnd.x = GenerateRandomNumber(seed);
    rnd.y = GenerateRandomNumber(seed);
    rnd.z = GenerateRandomNumber(seed);

    if (distribution == 1)
    {
        // Use a triangular distribution instead of a uniform distribution
        
        // Option 1: Generate a second set of random samples
        //float3 rnd1;
        //rnd1.x = GenerateRandomNumber(seed);
        //rnd1.y = GenerateRandomNumber(seed);
        //rnd1.z = GenerateRandomNumber(seed);
        //rnd = (rnd + rnd1) / 2.f;

        // Option 2: Transform the uniform distribution of the first sample to be triangular
        rnd = mad(rnd, 2.f, -1.f);                      // shift to [-1, 1]
        rnd = sign(rnd) * (1.f - sqrt(1.f - abs(rnd))); // transform from uniform to triangular
        rnd = (rnd * 0.5f) + 0.5f;                      // shift back to [0, 1]
    }

    // D3D rounds when converting from FLOAT to UNORM
    // Shift the random values from [0, 1] to [-0.5, 0.5]
    rnd -= 0.5f;

    // Scale the noise magnitude, values are in the range [-scale/2, scale/2]
    // The scale should be determined by the precision (and therefore quantization amount) of the target image's format
    return (rnd * scale);
}

/**
* Generate three components of blue noise in image-space.
* Blue noise texture from Christoph Peters at: http://momentsingraphics.de/BlueNoise.html
*/
float3 GetBlueNoise(uint2 position, uint width, uint frame, uint distribution, float scale)
{
    // Load a blue noise value from texture based on:
    // space - this thread's (x, y) position in the image
    // time  - the current frame number, used to select the texture array slice
    float3 rnd = blueNoiseArray.Load(int4(position.xy % 64, frame % 64, 0)).rgb;

    if (distribution == 1)
    {
        // Use a triangular distribution instead of a uniform distribution

        // Option 1: Load a second set of blue noise samples
        //float3 rnd1 = blueNoiseArray.Load(int4(position.xy % 64, (frame + 1) % 64, 0)).rgb;
        //rnd = (rnd + rnd1) / 2.f;

        // Option 2: Transform the uniform distribution of the first sample to be triangular
        rnd = mad(rnd, 2.f, -1.f);                      // shift to [-1, 1]
        rnd = sign(rnd) * (1.f - sqrt(1.f - abs(rnd))); // transform from uniform to triangular
        rnd = (rnd * 0.5f) + 0.5f;                      // shift back to [0, 1]
    }

    // D3D rounds when converting from FLOAT to UNORM
    // Shift the random values from [0, 1] to [-0.5, 0.5]
    rnd -= 0.5f;

    // Scale the noise magnitude, values are in the range [-scale/2, scale/2]
    // The scale should be determined by the precision (and therefore quantization amount) of the target image's format
    return (rnd * scale);
}

/**
* Generate three components of low discrepancy blue noise in image-space.
* Blue noise texture from Christoph Peters at: http://momentsingraphics.de/BlueNoise.html
*/
float3 GetLDSBlueNoise(uint2 position, uint width, uint frame, uint distribution, float scale)
{
    static const float goldenRatioConjugate = 0.61803398875f;

    // Load a blue noise value from texture
    float3 rnd = blueNoise.Load(int3(position % 256, 0)).rgb;

    // Generate a low discrepancy sequence
    rnd = frac(rnd + goldenRatioConjugate * ((frame - 1) % 16));

    if (distribution == 1)
    {
        // Transform the uniform distribution of the first sample to be triangular
        // Not sure if this even makes sense...
        rnd = mad(rnd, 2.f, -1.f);                      // shift to [-1, 1]
        rnd = sign(rnd) * (1.f - sqrt(1.f - abs(rnd))); // transform from uniform to triangular
        rnd = (rnd * 0.5f) + 0.5f;                      // shift back to [0, 1]
    }

    // D3D rounds when converting from FLOAT to UNORM
    // Shift the random values from [0, 1] to [-0.5, 0.5]
    rnd -= 0.5f;

    // Scale the noise magnitude, values are in the range [-scale/2, scale/2]
    // The scale should be determined by the precision (and therefore quantization amount) of the target image's format
    return (rnd * scale);
}

// ---[ Pixel Shader ]---

float4 PS(PSInput input) : SV_TARGET
{
    float3 worldPosition = float3(input.position.x, 0.f, input.position.y);
    float3 normal = float3(0.f, 1.f, 0.f);
    float3 lightVector = float3(lightPosition - worldPosition);
    float3 lightDirection = normalize(lightVector);

    // Compute color
    float3 result = saturate(color  * dot(normal, lightDirection));
    
    // Apply tonemapping
    if (useTonemapping)
    {
        result = ACESFilm(result);
    }

    // Dither
    if (useDithering > 0)
    {
        // Compute the noise
        float3 noise = 0;
        if (noiseType == 0)
        {
            noise = GetWhiteNoise(uint2(input.position.xy), resolutionX, frameNumber, distributionType, noiseScale);
        }
        else if(noiseType == 1)
        {
            noise =  GetBlueNoise(uint2(input.position.xy), resolutionX, frameNumber, distributionType, noiseScale);
        }
        else if (noiseType == 2)
        {
            noise = GetLDSBlueNoise(uint2(input.position.xy), resolutionX, frameNumber, distributionType, noiseScale);
        }

        if (showNoise)
        {
            return float4(noise, 1.f);
        }

        // Add the noise to the color
        result += noise;
    }

    // Gamma correct
    result = LinearToSRGB(result);

    return float4(result, 1.f);
}
