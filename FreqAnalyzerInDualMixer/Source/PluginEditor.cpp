/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
FreqAnalyzerInDualMixerAudioProcessorEditor::FreqAnalyzerInDualMixerAudioProcessorEditor (FreqAnalyzerInDualMixerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (660, 580);
    setResizable(false, false);
    
    freqAnalyzerPtr.reset( new FreqAnalyzer );
    addAndMakeVisible(*freqAnalyzerPtr);
    freqAnalyzerPtr->setBounds (15, 295, 630, 270);
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
    g.drawFittedText (+"FreqAnalyzer 0.0.1", getLocalBounds(), juce::Justification::topLeft, 1);
}

void FreqAnalyzerInDualMixerAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
