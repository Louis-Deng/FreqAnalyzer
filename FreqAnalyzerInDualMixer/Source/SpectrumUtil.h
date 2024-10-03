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
    inline float amp2db(float amp)
    {
        float dbNegative = 20.0f*log10( pow(amp,2.0f) );
        return dbNegative;  // expected to return -inf when amp=0.0f
    }

    inline void amp2db(std::vector<float>& input)
    {
        for (int i=0;i<size(input);i++)
        {
            input[i] = 20.0f*log10( pow(input[i],2.0f) );
        }
    }
    
    /// fft bin to frequency conversion - probably won't use this one in spectrometer
    inline float bin2freq(float sr, uint32_t maxBin, uint32_t bin)
    {
        float frequency = sr/(maxBin-1)*bin;
        return frequency;
    }
}
