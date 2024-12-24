/*
==============================================================================

SpectrumUtil.h
Created: 26 Sep 2024 12:35:54pm
Author:  Louis Deng

==============================================================================
*/

#pragma once
namespace SpectrumUtil
{
inline float FLOOR = -192.0f;
inline float amp2db(float amp)
{
    float dbNegative = 10.0f*log10( abs(amp) );
    if (dbNegative < FLOOR) {dbNegative = FLOOR;}
    
    return dbNegative;  // expected to return -inf when amp=0.0f
}

inline void amp2db(std::vector<float>& input)
{
    for (int i=0;i<size(input);i++)
    {
        if (pow(input[i],2.0f) < 1e-16) {input[i] = FLOOR;}
        else {input[i] = 20.0f*log10( pow(input[i],2.0f) );}
    }
}

/// fft bin to frequency conversion - probably won't use this one in spectrometer
inline float bin2freq(float sr, uint32_t maxBin, uint32_t bin)
{
    float frequency = sr/(maxBin-1)*bin;
    return frequency;
}

}
