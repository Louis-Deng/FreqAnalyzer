/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class FreqAnalyzerInDualMixerAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Slider::Listener
{
public:
    FreqAnalyzerInDualMixerAudioProcessorEditor (FreqAnalyzerInDualMixerAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~FreqAnalyzerInDualMixerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    void sliderValueChanged(juce::Slider*) override;
    
    // memory of VTS
    typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
    typedef juce::AudioProcessorValueTreeState::ComboBoxAttachment ComboBoxAttachment;
    typedef juce::AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    FreqAnalyzerInDualMixerAudioProcessor& audioProcessor;
    
    // param vts object
    juce::AudioProcessorValueTreeState& valueTreeState;
    
    // custom added objects
    std::shared_ptr<FreqAnalyzer> freqAnalyzerPtr;
    
    juce::Slider mDWMixKnob;
    juce::Label mDWMixKnobLabel;
    std::unique_ptr<SliderAttachment> mDWMixKnobAtt;
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreqAnalyzerInDualMixerAudioProcessorEditor)
};
