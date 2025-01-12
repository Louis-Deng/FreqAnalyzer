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
// with 50% zero padding -- 2^10 = 1024, ~ 46.9fps
// to maintain the same framerate (~23.4fps) i.e. update every 2048 samples
// -> we can use 50% overlap FFTN = 4096    -> copy i=2048~4095(end) to new buffer i=0~2047
// or we can use 75% overlap FFTN = 8192    -> copy i=2048~8191(end) to new buffer i=0~6043

// so far the FFTORDER_A is fixed at 11, may be subject to change later
// code is written as such that sample injection is always capped at 2048, fft calculation is always per 1024 samples
const uint32_t FFTORDER_A = 11;   // active FFT_ORDER (what is updated every frame) == 11
static uint32_t FFTORDER_T = 11; // total length of FFT in 2^Order (11,12,13)
const float SR_DEFAULT = 48e3f; // default samplerate for generating display

/// single data stream fft Unit (one signal channel)
class fftUnit
{
public:
    fftUnit()
    {
        // initialize juce fft object
        fftOp.reset(new juce::dsp::FFT(FFTORDER_T));
        sizeBuffer = (fftOp->getSize());
        // make two buffers the size of fft buffersize
        iBuffer.resize(sizeBuffer);
        oBuffer.resize(sizeBuffer);
        
        // Nyquist size is half of total fftsize
        sizeNyquist = sizeBuffer >> 1;
        // active size is (1-overlap)% of sizeNyquist (100,50,25 %)
        sizeStream = sizeNyquist >> MAGDIFF;
        
#ifdef DEBUG
        DBG("fftUnit buffer size is " + juce::String(sizeBuffer));
        DBG("fftUnit Nyquist size is " + juce::String(sizeNyquist));
        DBG("fftUnit Overlap is " + juce::String((1.0f-1.0f/pow(2.0f,MAGDIFF))*100.0f) + "%");
        DBG("fftUnit Active Stream size is " + juce::String(sizeStream));
#endif
    }
    
    ~fftUnit()
    {
    }
        
    /// inject a single sample to this fft unit, turn 'ready' to true if this unit is ready to show its complete spectrum
    void injectSample (float input)
    {
        iBuffer[iterNyquistCounter] = input;
        iterNyquistCounter++;
        iterActiveCounter++;
        
        if ( !(iterActiveCounter<sizeStream) )
        {
            // reset active fft counter - overlap dependent
            iterActiveCounter = 0;
            // copy i to o ###NEED UPDATE TO REFLECT CORRECT TIME SERIES
            std::copy(iBuffer.begin(),iBuffer.end(),oBuffer.begin());
            // calculate o
            // EVERY 1024
            fftOp->performFrequencyOnlyForwardTransform(&(oBuffer[0]));
            if (!ready) ready = true;
#ifdef DEBUG
            else DBG("queue stalled for fftUnit D/W: " + juce::String(iddbgDW) + " L/R: " + juce::String(iddbgLR));
#endif
        }
        
        if ( !(iterNyquistCounter<sizeNyquist) )
        {
            // reset Nyquist fft iterator - zero-padding dependent
            // EVERY 2048
            iterNyquistCounter = 0;
        }
    }
    
    std::vector<float> getBuffer() const { return oBuffer; }
    
    /// get size of buffer
    uint32_t getSizeBuffer() const { return sizeBuffer; }
    
    uint32_t getSizeNyquist() const { return sizeNyquist; }
    
    bool ready = false;
    
    /// debug identity
#ifdef DEBUG
    int iddbgDW = -1;
    int iddbgLR = -1;
#endif
    
private:
    /// Overlap related
    uint32_t MAGDIFF = FFTORDER_T-FFTORDER_A;
    /// I/O Buffer
    std::vector<float> iBuffer;
    std::vector<float> oBuffer;
    
    /// iteration related parameter
    uint32_t iterNyquistCounter = 0;
    uint32_t iterActiveCounter = 0;
    
    uint32_t sizeBuffer;
    uint32_t sizeNyquist;
    uint32_t sizeStream;
    
    /// base unit
    std::unique_ptr<juce::dsp::FFT> fftOp;
    
};  // fftUnit class brackets

/// Aux class for making a log2 x-axis of frequency
class FreqScale4Display
{
public:
    FreqScale4Display()
    {
        fSize = pow(2,FFTORDER_T-1);
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
        // ignore zero frequency
        freqAxis[0] = 0.0f;
        // create a raw axis and convert to log10
        for (int bin=1;bin<fSize;bin++)
        {
            freqAxis[bin] = log10((float)bin)/log10(fSize);
        }
        // this gives a vector between 0~1 in freqAxis vector, we should multiply later by the width of the graph window
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
        initializeXGaps(graphXSize);
        
        // should be whole fftSize
        dBDry.resize(dryUnit->getSizeBuffer());
        dBWet.resize(wetUnit->getSizeBuffer());
        
        DBG("dry wet resized to " + juce::String(dBDry.size()) + " " + juce::String(dBWet.size()));
        
        // coordinate series size defined here
        xCoords.resize(graphXSize);
        
        // should be gapped-bin-size - 1
        dryLines.resize(xGaps.size()-1);
        wetLines.resize(xGaps.size()-1);
        
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
            const juce::MessageManagerLock mmLrepaint;
            spectrumGen();
            repaint();
        }
    }
    
    void resized() override
    {
        if (getHeight()!=0 && getWidth()!=0)
        {
            recalculateXcoords();   //should come after initiate xGaps
            recalculateYIncrements();
            DBG("FAC " + juce::String((float)getWidth()) + " " + juce::String((float)getHeight()));
        }
    }
    
    /// inherited from juce::component
    void paint(juce::Graphics& g) override
    {
        //const juce::MessageManagerLock mmLpaintnow;
        
        //DBG("mono channel paint called for channel: " + juce::String(chanid));
        
        float dLast, wLast, dThis, wThis;
        // void drawLine(float startX, float startY, float endX, float endY) const
        // void drawLine(float startX, float startY, float endX, float endY, float lineThickness) const
        for (int x=0;x<xGaps.size();x++)
        {
            if (x==0)
            {
                dThis = dBDry[xGaps[0]]*yIncrement;
                wThis = dBWet[xGaps[0]]*yIncrement;
            }else{
                
                int iThis = xGaps[x];
                int iLast = xGaps[x-1];
                
                dLast = dThis;
                wLast = wThis;
                
                dThis = dBDry[iThis]*yIncrement;
                wThis = dBWet[iThis]*yIncrement;
                
                // set and draw new lines
                dryLines[x-1].setStart(xCoords[iLast],dLast+1.0f);
                dryLines[x-1].setEnd(xCoords[iThis],dThis+1.0f);
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
                g.drawLine(dryLines[x-1]);
                
                wetLines[x-1].setStart(xCoords[iLast],dLast+wLast+1.0f);
                wetLines[x-1].setEnd(xCoords[iThis],dThis+wThis+1.0f);
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
                g.drawLine(wetLines[x-1]);
                
            }
        } // for loop brackets
    }
    
private:
    // iterB
    int graphXSize;
    
    // dry and wet fft units
    std::unique_ptr<fftUnit> dryUnit;
    std::unique_ptr<fftUnit> wetUnit;
        
    // buffers storing SPL in dB
    std::vector<float> dBDry;
    std::vector<float> dBWet;
    
    // chan-id
    uint32_t chanid;
    
    // iteration scaling
    std::vector<int> xGaps;
    
    // dimension related floats
    float yIncrement;
    std::vector<float> xCoords;
    
    // lines
    std::vector<juce::Line<float>> dryLines;
    std::vector<juce::Line<float>> wetLines;
    
    void spectrumGen()
    {
        const juce::MessageManagerLock mmLspectrumGennow;
        
        dBDry = dryUnit->getBuffer();
        dBWet = wetUnit->getBuffer();
        
        SpectrumUtil::amp2db(dBDry);
        SpectrumUtil::amp2db(dBWet);
        
        // un-ready the units
        dryUnit->ready = false;
        wetUnit->ready = false;
    }
    
    /// called when channel initialized, incrementally omit higher frequency bins
    void initializeXGaps(int xTotal)
    {
        int iterX = 1;  // ignore zero frequency =/=0
        int gap = 1;
        int indexGap = 0;
        while (iterX < xTotal)  // ignore Nyquist frequency </=
        {
            
            xGaps.push_back(iterX);
            iterX += gap;
            
            indexGap+=1;
            
            if (!(indexGap%32))
                gap*=2; // double the gap every 32 bin selections
        }
        DBG("xGap series size = " + juce::String(xGaps.size()));
    }
    
    /// called when channel component initialized or resized
    void recalculateYIncrements()
    {
        yIncrement = (float)(getHeight()-2.0f)/SpectrumUtil::FLOOR;
        DBG("FACh y inc = " + juce::String(yIncrement));
    }
    
    void recalculateXcoords()
    {
        for (int bin=0;bin<fScale.freqAxis.size();bin++)
        {
            // should ignore bin=0
            // leave 1 pixel on L/R ends
            xCoords[bin] = fScale.freqAxis[bin]*(getWidth()-2.0f) + 1.0f;
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
        const juce::MessageManagerLock mmLpaintBordernow;
        
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


