/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_dsp/juce_dsp.h"
//==============================================================================
/**
*/
class SimpleEQLinuxAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SimpleEQLinuxAudioProcessor();
    ~SimpleEQLinuxAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    //==============================================================================
    //
    

    /* @brief   Creates the audio processor parameter layout 
     * @returns Audio processor parameter layout
    */
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // @brief   The Audio Processor value tree getStateInformation
    juce::AudioProcessorValueTreeState apvts = juce::AudioProcessorValueTreeState(*this, nullptr, "Parameters", createParameterLayout());

private:

    // create some type aliases to make our DSP life easier
    //
    // 12 dB/octave IIR filter. Can be configured as lopass, hipass, peak, notch, etc.
    using Filter = juce::dsp::IIR::Filter<float>;
    
    // A processor chain holds a bunch of DSP instances and runs them on a sample block in sequence
    // 
    // up to 48dB/oct Cut filter
    // We will add all our EQ filters to this chain, up to 4 filters for a maximum of 48dB/oct
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

    // Represent the whole mono channel chain: LoCut -> Paramentric -> HiCut
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

    // We need two mono chains to implement stereo
    MonoChain leftChain;
    MonoChain rightChain;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQLinuxAudioProcessor)
};
