/*
  ==============================================================================

    FreqAnalyzer.h
    Created: 9 Sep 2024 4:13:54pm
    Author:  Louis Deng

    fft-based processing, includes dry and wet proportions, 2 channels
     
    audio signals being injected to the buffer in this instance
    the freq-domain points is passed to pluginEditor to be displayed
  ==============================================================================
*/

#pragma once
#include "SpectrumUtil.h"
// all FFT-related objects in this header has fixed order: fftsize = 2048 (2e11)
// fft :: 2^N sized fft -- 2^11 = 2048, ~23.4fps
const uint32_t FFTORDER = 11;   // so far the FFTORDER is fixed at 11, may be subject to change later

/// single data stream fft Unit (one signal channel)
class fftUnit
{
public:
    fftUnit()
    {
        // initialize juce fft object
        fftOp.reset(new juce::dsp::FFT(FFTORDER));
        sizeBuffer = (fftOp->getSize()) << 1;
        // make two buffers the size of fft buffersize
        iBuffer.resize(sizeBuffer);
        oBuffer.resize(sizeBuffer);
        
        // nyquist size is half of total fftsize
        sizeNyquist = sizeBuffer >> 1;
        
        DBG("fftUnit buffer size is " + juce::String(sizeBuffer));
        DBG("fftUnit Nyquist size is " + juce::String(sizeNyquist));
    }
    
    ~fftUnit()
    {
    }
    
    /// inject a single sample to this fft unit, returns if this unit is ready to show its complete spectrum
    void injectSample (float input)
    {
        iBuffer[iterBuffer] = input;
        iterBuffer++;
        
        if ( !(iterBuffer<sizeNyquist) )
        {
            // reset buffer iterator
            iterBuffer = 0;
            // copy i to o
            oBuffer = iBuffer;
            // calculate o
            fftOp->performFrequencyOnlyForwardTransform(&(oBuffer[0]));
            if (!ready) ready = true;
#ifdef DEBUG
            else DBG("queue stalled for fftUnit D/W: " + juce::String(iddbgDW) + " L/R: " + juce::String(iddbgLR));
#endif
        }
    }
    
    std::vector<float> getBuffer() const { return oBuffer; }
    
    /// get size of buffer
    uint32_t getSizeBuffer() const { return sizeBuffer; }
    
    /// get size of useful info buffer (should be half of size of buffer)
    uint32_t getSizeNyquist() const { return sizeNyquist; }
    
    bool ready = false;
    
    /// debug identity
#ifdef DEBUG
    int iddbgDW = -1;
    int iddbgLR = -1;
#endif
    
private:
    /// I/O Buffer
    std::vector<float> iBuffer;
    std::vector<float> oBuffer;
    
    /// iteration related parameter
    uint32_t iterBuffer = 0;
    
    uint32_t sizeBuffer;
    uint32_t sizeNyquist;
    
    /// base unit
    std::unique_ptr<juce::dsp::FFT> fftOp;
    
};  // fftUnit class brackets

/// Channel component - includes dry and wet
class FreqAnalChannel : public juce::Component
{
public:
    FreqAnalChannel(uint32_t chan): chanid(chan)
    {
        // init
        dryUnit.reset( new fftUnit );
        wetUnit.reset( new fftUnit );
        
        iterBuffer = dryUnit->getSizeBuffer();
        
        // should be 1/2 fftSize
        dBDry.resize(dryUnit->getSizeNyquist());
        dBWet.resize(wetUnit->getSizeNyquist());
        
        // set bounds for graphics
        /*
        int x0 = 15;
        int y0 = 295;
        int xi = 315;
        int yi = 135;
        for (int v=0; v<4; v++){
            bounds[v][0] = chan*xi+x0+(v%2)*xi;
            bounds[v][1] = y0+(v/2)*yi;
        }
         */
        xPos = 15+(int)chan*315;
        
#ifdef DEBUG
        dryUnit->iddbgDW = 0;
        wetUnit->iddbgDW = 1;
        dryUnit->iddbgLR = (int)chan;
        wetUnit->iddbgLR = (int)chan;
#endif
        
    }
    ~FreqAnalChannel()
    {
    }
    
    void injectSampleTo (float inputSample, uint32_t drywet)
    {
        switch(drywet){
            case 0:
                dryUnit->injectSample( inputSample );
                break;
            case 1:
                wetUnit->injectSample( inputSample );
                break;
        }
        
        if (dryUnit->ready && wetUnit->ready)
        {
            spectrumGen();
            const juce::MessageManagerLock mmLrepaint;
            repaint();
            dryUnit->ready = false;
            wetUnit->ready = false;
        }
    }
    
    /// inherited from juce::component
    void paint(juce::Graphics& g) override
    {
        DBG("mono channel paint called for channel: " + juce::String(chanid));
        
    }
    
    /// Component bounds (0,0) (0,x) (0,y) (x,y)
    //std::vector<std::vector<int>> bounds = std::vector<std::vector<int>>(4,std::vector<int>(2));
    int xPos;
    
private:
    // iterB
    int iterBuffer;
    
    // dry and wet fft units
    std::unique_ptr<fftUnit> dryUnit;
    std::unique_ptr<fftUnit> wetUnit;
    
    // ready to show content
    bool dryReady;
    bool wetReady;
    
    // SPL in dB lines
    std::vector<float> dBDry;
    std::vector<float> dBWet;
    
    // chan-id
    uint32_t chanid;
    
    void spectrumGen()
    {
        dBDry = dryUnit->getBuffer();
        SpectrumUtil::amp2db(dBDry);
        dBWet = wetUnit->getBuffer();
        SpectrumUtil::amp2db(dBWet);
    }
    
};  // FreqAnalChannel class brackets

//#include <juce_FFT.h>
/// Component Freq Analyzer, data organizing
class FreqAnalyzer : public juce::Component
{
    
public:
    FreqAnalyzer()
    {
        addChildComponent(LFAC,0);
        addChildComponent(RFAC,0);
        addAndMakeVisible(&LFAC);
        addAndMakeVisible(&RFAC);
    }
    ~FreqAnalyzer()
    {
    }
    
    /// input a single sample to specified channel and dry/wet configuration
    void injectSampleToTo(float inputSample, uint32_t leftright, uint32_t drywet)
    {
        switch(leftright){
            case 0:
                LFAC.injectSampleTo(inputSample,drywet);
                break;
            case 1:
                RFAC.injectSampleTo(inputSample,drywet);
                break;
        }
    }
        
    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colours::white);
        
        juce::Rectangle<int> rectAreaL (LFAC.xPos, 295, 315, 270);
        juce::Rectangle<int> rectAreaR (RFAC.xPos, 295, 315, 270);
        
        g.drawRect(rectAreaL);
        g.drawRect(rectAreaR);
    }
    
private:
    // bin number max
    uint32_t iterBLimit = pow(2,FFTORDER-1);
    
    /// leftright, drywet buffers, initialize with identities
    FreqAnalChannel LFAC = FreqAnalChannel(0);
    FreqAnalChannel RFAC = FreqAnalChannel(1);
        
};  // FreqAnalyzer class brackets

