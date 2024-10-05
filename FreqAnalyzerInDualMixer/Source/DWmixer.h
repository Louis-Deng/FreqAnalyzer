/*
  ==============================================================================

    DWmixer.h
    Created: 10 Aug 2024 11:40:58am
    Author:  Louis Deng
 
    Dry/Wet signal mixer, sends signal to frequency analyzer about its dry and wet samples
    must be using the same signal type (double/float) with FreqAnalyzer class

  ==============================================================================
*/

#pragma once

#include "FreqAnalyzer.h"

template <typename SignalType>
class DWmixer
{
public:
    
    DWmixer()
    {
    }
    
    ~DWmixer()
    {
    }
    
    void communicateFreqAnalyzerPtrs(std::shared_ptr<FreqAnalyzer> freqAnalyzerPtr)
    {
        faPtr = freqAnalyzerPtr;
    }
        
    /// process buffered input (R+W Permission for wet, R Permission for dry): apply sample-wise tick calculation, and replace buffer with output.
    void processBuffer(const float *dryBufferRead, float *wetBufferWrite, int numSamps)
    {
        for (int i=0;i<numSamps;i++)
        {
            //overwrite wet with dry+wet
            *(wetBufferWrite+i) = mix(*(dryBufferRead+i),*(wetBufferWrite+i));
        }
        //DBG("processed one buffer");
    }
    
    void injectProportion(double input)
    {
        proportion = input;
    }
    
    void setid(uint32_t channelid)
    {
        thisChanid = channelid;
    }
    
private:
    // basics
    SignalType outSamp_ = 0.0;
    double proportion = 0.0;
    
    // channel id, distinguish left and right
    uint32_t thisChanid;
    
    // shared ptr storage
    std::shared_ptr<FreqAnalyzer> faPtr;
    
    SignalType mix(SignalType dryInput, SignalType wetInput)
    {
        SignalType dryPPT = (1.0-proportion)*(double)dryInput;
        SignalType wetPPT = proportion*(double)wetInput;
        //embed freq analyzer, inject dry and wet of this channel
        if (faPtr!=nullptr)
        {
            faPtr->injectSampleToTo(dryPPT,thisChanid,0);
            faPtr->injectSampleToTo(wetPPT,thisChanid,1);
            //DBG("injectedToTo freqAnalyzer once");
        }
        outSamp_ = dryPPT+wetPPT;
        return outSamp_;
    }
};
