/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
FreqAnalyzerInDualMixerAudioProcessorEditor::FreqAnalyzerInDualMixerAudioProcessorEditor (FreqAnalyzerInDualMixerAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor (&p), audioProcessor (p), valueTreeState(vts)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (660, 580);
    setResizable(false, false);
    
    
    addAndMakeVisible(mDWMixKnob);
    // Add dry/wet mix slider object
    mDWMixKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    //drywetMix.setMouseDragSensitivity(80);
    mDWMixKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 120, 20);
    mDWMixKnobLabel.setText ("Dry/Wet Mix", juce::dontSendNotification);
    //true is to the left, false is above
    mDWMixKnobLabel.attachToComponent (&mDWMixKnob, false);
    mDWMixKnob.setBounds(20, 90, 120, 120);
    mDWMixKnob.addListener(this);
    mDWMixKnobAtt.reset (new SliderAttachment (valueTreeState, "00-allmix", mDWMixKnob));
    
    freqAnalyzerPtr.reset( new FreqAnalyzer );
    for(int i=0; i<2; i++){
        audioProcessor.mDWM[i]->communicateFreqAnalyzerPtrs(freqAnalyzerPtr);
    }
    addAndMakeVisible(*freqAnalyzerPtr);
    freqAnalyzerPtr->setBounds(15, 295, 630, 270);
   
}

FreqAnalyzerInDualMixerAudioProcessorEditor::~FreqAnalyzerInDualMixerAudioProcessorEditor()
{
}

//==============================================================================
void FreqAnalyzerInDualMixerAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::grey);
    g.setFont (juce::FontOptions (12.0f));
    g.drawFittedText ("FreqAnalyzer 0.0.1", getLocalBounds(), juce::Justification::topLeft, 1);
}

void FreqAnalyzerInDualMixerAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}

void FreqAnalyzerInDualMixerAudioProcessorEditor::sliderValueChanged(juce::Slider* sliderRef)
{
    if (sliderRef == &mDWMixKnob)
    {
        for(int i=0; i<2; i++){
            audioProcessor.mDWM[i]->injectProportion(0.0f);
        }
    }
}
