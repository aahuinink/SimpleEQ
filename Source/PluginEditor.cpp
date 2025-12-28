/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include <vector>
#include "PluginEditor.h"


std::vector<juce::Component*> SimpleEQLinuxAudioProcessorEditor::getComps() {
    return {
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &locutFreqSlider,
        &hicutFreqSlider
    };
}

//==============================================================================
SimpleEQLinuxAudioProcessorEditor::SimpleEQLinuxAudioProcessorEditor (SimpleEQLinuxAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    for ( auto* component : SimpleEQLinuxAudioProcessorEditor::getComps() ) {
        addAndMakeVisible(component);
    }
    setSize (600, 400);
}

SimpleEQLinuxAudioProcessorEditor::~SimpleEQLinuxAudioProcessorEditor()
{
}

//==============================================================================
void SimpleEQLinuxAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void SimpleEQLinuxAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    //
    // set the bounds of various components based on a percentage of the window size
    auto bounds = getLocalBounds();
    
    // remove chops it up
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33); // TODO reponse area

    locutFreqSlider.setBounds(bounds.removeFromLeft(bounds.getHeight() * 0.33));
    hicutFreqSlider.setBounds(bounds.removeFromRight(bounds.getHeight() * 0.5));

    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
    peakQualitySlider.setBounds(bounds);
}
