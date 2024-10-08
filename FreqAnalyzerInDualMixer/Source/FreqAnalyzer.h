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
const float SR_DEFAULT = 48e3f; // default samplerate for generating display

/// single data stream fft Unit (one signal channel)
class fftUnit
{
public:
    fftUnit()
    {
        // initialize juce fft object
        fftOp.reset(new juce::dsp::FFT(FFTORDER));
        sizeBuffer = (fftOp->getSize());
        // make two buffers the size of fft buffersize
        iBuffer.resize(sizeBuffer);
        oBuffer.resize(sizeBuffer);
        
        // nyquist size is half of total fftsize
        sizeNyquist = sizeBuffer >> 1;
        //DBG("fftUnit buffer size is " + juce::String(sizeBuffer));
        //DBG("fftUnit Nyquist size is " + juce::String(sizeNyquist));
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
            //DBG("injectSample hit top");
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

/// Aux class for making a log2 x-axis of frequency, reversible conversion TBD
class FreqScale4Display
{
public:
    FreqScale4Display()
    {
        fSize = pow(2,FFTORDER-1);
        freqAxis.resize(fSize);
        remapFreq();
    }
    ~FreqScale4Display()
    {
    }
    
    void changeSR(float sr)
    {
        DBG("change of SR detected within the FreqScale static Object, remap freq now...");
        sampleRate = sr;
        remapFreq();
    }
    
    // return the max value
    float maxFreq() const   {return freqAxis[fSize-1];}
    
    // the thing itself
    std::vector<float> freqAxis;
    
private:
    float sampleRate = SR_DEFAULT;
    int fSize;
    
    void remapFreq()
    {
        // prevent displaying current remapping values of X
        //const juce::MessageManagerLock mmL2;
        // ignore zero frequency
        freqAxis[0] = 0.0f;
        // create a raw axis and convert to log2
        for (int i=1;i<fSize;i++)
        {
            freqAxis[i] = log2((float)i/fSize*sampleRate);
        }
    }
};

static FreqScale4Display fScale;

/// Channel component - includes dry and wet
class FreqAnalChannel : public juce::Component
{
public:
    FreqAnalChannel(uint32_t chan): chanid(chan)
    {
        // init
        dryUnit.reset( new fftUnit );
        wetUnit.reset( new fftUnit );
        
        graphXSize = dryUnit->getSizeNyquist();
        
        // should be whole fftSize
        dBDry.resize(dryUnit->getSizeBuffer());
        dBWet.resize(dryUnit->getSizeBuffer());
        
        // should be 1/2 fftSize
        xCoords.resize(graphXSize);
        
        // should be 1/2 fftSize - 1
        dryLines.resize(graphXSize-1);
        wetLines.resize(graphXSize-1);
        
#ifdef DEBUG
        dryUnit->iddbgDW = 0;
        wetUnit->iddbgDW = 1;
        dryUnit->iddbgLR = (int)chan;
        wetUnit->iddbgLR = (int)chan;
#endif
        recalculateIncrements();
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
            //DBG("both ready for this channel " + juce::String(chanid) + " , repaint called");
            dryUnit->ready = false;
            wetUnit->ready = false;
        }
    }
    
    void resized() override
    {
        if (getHeight()!=0 && getWidth()!=0)
        {
            recalculateXcoords();
            recalculateIncrements();
            DBG("FAC " + juce::String((float)getWidth()) + " " + juce::String((float)getHeight()));
        }
    }
    
    /// inherited from juce::component
    void paint(juce::Graphics& g) override
    {
        const juce::MessageManagerLock mmLpaintnow;
        //DBG("mono channel paint called for channel: " + juce::String(chanid));
        
        float dLast, wLast, dThis, wThis;
        // void drawLine(float startX, float startY, float endX, float endY) const
        // void drawLine(float startX, float startY, float endX, float endY, float lineThickness) const
        for (int i=1;i<graphXSize;i++)
        {//skip zero frequency and nyquist frequency
            if (i==1)
            {
                dThis = dBDry[i]*yIncrement;
                wThis = dBWet[i]*yIncrement;
            }else{
                dLast = dThis;
                wLast = wThis;
                
                dThis = dBDry[i]*yIncrement;
                wThis = dBWet[i]*yIncrement;
                
                // undraw previous lines (?)
                /*
                g.setColour(juce::Colours::white);
                g.drawLine(dryLines[i-1]);
                g.drawLine(wetLines[i-1]);
                */
                
                // set and draw new lines
                dryLines[i-1].setStart(xCoords[i-1],dLast);
                dryLines[i-1].setEnd(xCoords[i],dThis);
                if (chanid == 0)
                {
                    g.setColour(juce::Colours::yellow);
                    g.setOpacity(0.5);
                }
                else if (chanid == 1)
                {
                    g.setColour(juce::Colours::orange);
                    g.setOpacity(0.5);
                }
                g.drawLine(dryLines[i-1]);
                
                wetLines[i-1].setStart(xCoords[i-1],dLast);
                wetLines[i-1].setEnd(xCoords[i],wThis+dThis);
                if (chanid == 0)
                {
                    g.setColour(juce::Colours::pink);
                    g.setOpacity(0.5);
                }
                else if (chanid == 1)
                {
                    g.setColour(juce::Colours::purple);
                    g.setOpacity(0.5);
                }
                g.drawLine(wetLines[i-1]);
                
            }
        }
        
    }
    
private:
    // iterB
    int graphXSize;
    
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
    
    // dimension related floats
    float yIncrement;
    std::vector<float> xCoords;
    
    // lines
    std::vector<juce::Line<float>> dryLines;
    std::vector<juce::Line<float>> wetLines;
    
    void spectrumGen()
    {
        dBDry = dryUnit->getBuffer();
        SpectrumUtil::amp2db(dBDry);
        dBWet = wetUnit->getBuffer();
        SpectrumUtil::amp2db(dBWet);
    }
    
    /// called when channel component initialized or resized
    void recalculateIncrements()
    {
        yIncrement = (float)getHeight()/-192.0f;
        DBG("FACh y inc = " + juce::String(yIncrement));
    }
    
    void recalculateXcoords()
    {
        for (int i=0;i<fScale.freqAxis.size();i++)
        {
            xCoords[i] = (float)fScale.freqAxis[i]*(getWidth()-2.0f)/fScale.maxFreq() + 1.0f;
        }
    }
    
};  // FreqAnalChannel class brackets

//#include <juce_FFT.h>
/// Component Freq Analyzer, two channels, two graphs, each with both D/W
class FreqAnalyzer : public juce::Component
{
    
public:
    FreqAnalyzer()
    {        
        addAndMakeVisible(&LFAC);
        addAndMakeVisible(&RFAC);
        LFAC.setOpaque(false);
        RFAC.setOpaque(false);
    }
    ~FreqAnalyzer()
    {
    }
    
    void setSR(float sr)//don't suppose this will be used any time soon, unless we are displaying rulers
    {
        if(sr==sampleRate)
        {
            // checkpoint preventing unnecessary remap
            sampleRate = sr;
            fScale.changeSR(sampleRate);
        }
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
    
    void resized() override
    {
        rectAreaL = juce::Rectangle<int>(0, 0, getWidth(), getHeight());
        rectAreaR = juce::Rectangle<int>(0, 0, getWidth(), getHeight());
        DBG("FAer: " + juce::String(getWidth()) + " " + juce::String(getHeight()));
        LFAC.setBounds(rectAreaL);
        RFAC.setBounds(rectAreaR);
    }
        
    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colours::white);
        g.drawRect(rectAreaL);
        g.drawRect(rectAreaR);
        
        //can this not be called so often?? - unfortunately, child component calls repaint, this gets called
        //DBG("freqAnalyzer borders painted");
    }
    
private:
    /// leftright, drywet buffers, initialize with identities
    FreqAnalChannel LFAC = FreqAnalChannel(0);
    FreqAnalChannel RFAC = FreqAnalChannel(1);
    
    juce::Rectangle<int> rectAreaL;
    juce::Rectangle<int> rectAreaR;
    
    float sampleRate = SR_DEFAULT;
    
};  // FreqAnalyzer class brackets


