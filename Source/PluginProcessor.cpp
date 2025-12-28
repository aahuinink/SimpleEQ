/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Params.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_audio_processors_headless/juce_audio_processors_headless.h"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "juce_dsp/juce_dsp.h"
#include <cstddef>
#include <memory>
#include <utility>

using namespace Params;


//==============================================================================
SimpleEQLinuxAudioProcessor::SimpleEQLinuxAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

SimpleEQLinuxAudioProcessor::~SimpleEQLinuxAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleEQLinuxAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleEQLinuxAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQLinuxAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQLinuxAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleEQLinuxAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleEQLinuxAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleEQLinuxAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleEQLinuxAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimpleEQLinuxAudioProcessor::getProgramName (int index)
{
    return {};
}

void SimpleEQLinuxAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SimpleEQLinuxAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    // a process spec contains information about the context in which the DSP algorithm's prepare() method is called
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;   // monochain can only handle one channel at a time

    leftChain.prepare(spec);
    rightChain.prepare(spec);

    ChainSettings chainSettings = getChainSettings(apvts);
    
    // calculate filter coefficients
    // these are allocated to the heap (!!) for some stupid reason
    auto peakCoeff = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, 
        chainSettings.peakFreq, 
        chainSettings.peakQuality, 
        juce::Decibels::decibelsToGain(chainSettings.peakGain));

    // apply filter coefficients to filters in chains
    *leftChain.get<ChainPositions::Peak>().coefficients = *peakCoeff;
    *rightChain.get<ChainPositions::Peak>().coefficients = *peakCoeff;

    // for the hi and locut filters, we use a helper function to design
    // the filter coefficients based on the slope (order)
    IIRCoeffArray loCutCoeff = 
        juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(
                chainSettings.loCutFreq,
                sampleRate, 
                (chainSettings.loCutSlope + 1) * 2);

    IIRCoeffArray hiCutCoeff = 
        juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(
                chainSettings.hiCutFreq,
                sampleRate, 
                (chainSettings.hiCutSlope + 1) * 2);
    
    auto& leftLoCut = leftChain.get<ChainPositions::LoCut>();
    auto& rightLoCut = rightChain.get<ChainPositions::LoCut>();
    auto& leftHiCut = leftChain.get<ChainPositions::HiCut>();
    auto& rightHiCut = rightChain.get<ChainPositions::HiCut>();

    Helpers::setCutfilterCoeff<4>(leftLoCut, loCutCoeff);
    Helpers::setCutfilterCoeff<4>(rightLoCut, loCutCoeff);
    Helpers::setCutfilterCoeff<4>(leftHiCut, hiCutCoeff);
    Helpers::setCutfilterCoeff<4>(rightHiCut, hiCutCoeff);
}

void SimpleEQLinuxAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleEQLinuxAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

// The processing chain requires a processing context to run audio samples through the links in the chain.
// The context requires an Audio block. The Audio buffer may contain multiple channels, so we need to extract
// the left and right channels and wrap them in an audio block
void SimpleEQLinuxAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());


    // Update the filters based on the gui input
    updateFilters();

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.

    // wrap the buffer in an audio block and extract the channels 
    // The audio block is a list of pointers to the channels contained in the audio buffer.
    // Think of it like a view into that data
    juce::dsp::AudioBlock<float> block(buffer);

    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    // create processing contexts for each channel
    // We will use the replacing type which modifies the audio block in place (i.e. getInput and getOutput return the same data)
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
    
    leftChain.process(leftContext);
    rightChain.process(rightContext);

}

//==============================================================================
bool SimpleEQLinuxAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleEQLinuxAudioProcessor::createEditor()
{
    return new SimpleEQLinuxAudioProcessorEditor (*this);
}

//==============================================================================
void SimpleEQLinuxAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // save to memory block using output stream 
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void SimpleEQLinuxAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if ( tree.isValid() ) {
        apvts.replaceState(tree);
        updateFilters();
    }
}

// Create the parameter layout
juce::AudioProcessorValueTreeState::ParameterLayout SimpleEQLinuxAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    // add low-cut filter
    layout.add(std::make_unique<juce::AudioParameterFloat>(
          PID_LOCUT_FREQ, 
          PID_LOCUT_FREQ, 
          juce::NormalisableRange<float>(FREQ_20_HZ, FREQ_20000_HZ, DEFAULT_INTERVAL_FILTER, DEFAULT_SKEW), 
          1.f));
    
    // add hi-cut filter
    layout.add(std::make_unique<juce::AudioParameterFloat>(
          PID_HICUT_FREQ, 
          PID_HICUT_FREQ, 
          juce::NormalisableRange<float>(FREQ_20_HZ, FREQ_20000_HZ, DEFAULT_INTERVAL_FILTER, DEFAULT_SKEW), 
          750.f));

    // add peak filter
    layout.add(std::make_unique<juce::AudioParameterFloat>(
          PID_PEAK_FREQ, 
          PID_PEAK_FREQ, 
          juce::NormalisableRange<float>(FREQ_20_HZ, FREQ_20000_HZ, DEFAULT_INTERVAL_FILTER, DEFAULT_SKEW), 
          750.f));
    
    // add gain 
    layout.add(std::make_unique<juce::AudioParameterFloat>(
          PID_PEAK_GAIN, 
          PID_PEAK_GAIN, 
          juce::NormalisableRange<float>(MIN_GAIN, MAX_GAIN, DEFAULT_INTERVAL_GAIN, DEFAULT_SKEW), 
          0.0f));

    // add peak filter quality
    layout.add(std::make_unique<juce::AudioParameterFloat>(
          PID_PEAK_QUALITY, 
          PID_PEAK_QUALITY, 
          juce::NormalisableRange<float>(
            MIN_Q_FACTOR, 
            MAX_Q_FACTOR, 
            0.05f, 
            DEFAULT_SKEW), 
          0.0f));

    juce::StringArray filter_slopes;

    for (int i = 1; i < DEFAULT_SLOPE_COUNT+1; i++) {
      juce::String str;
      str << i * DEFAULT_SLOPE_STEP;
      str << " dB/octave";
      filter_slopes.add(str);
    }

    // add filter slope choices
    layout.add(std::make_unique<juce::AudioParameterChoice>(
          PID_LOCUT_SLOPE,
          PID_LOCUT_SLOPE,
          filter_slopes,
          0
          ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
          PID_HICUT_SLOPE, 
          PID_HICUT_SLOPE, 
          filter_slopes, 
          0));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
            PID_PEAK_SLOPE, 
            PID_PEAK_SLOPE, 
            filter_slopes, 
            0));

    return layout;
}


ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;
    
    settings.loCutFreq = apvts.getRawParameterValue(PID_LOCUT_FREQ)->load();
    settings.hiCutFreq = apvts.getRawParameterValue(PID_HICUT_FREQ)->load();
    settings.peakFreq = apvts.getRawParameterValue(PID_PEAK_FREQ)->load();
    settings.loCutSlope = apvts.getRawParameterValue(PID_LOCUT_SLOPE)->load();
    settings.hiCutSlope = apvts.getRawParameterValue(PID_HICUT_SLOPE)->load();
    settings.peakGain = apvts.getRawParameterValue(PID_PEAK_GAIN)->load();
    settings.peakQuality = apvts.getRawParameterValue(PID_PEAK_QUALITY)->load();

    return settings;
}

void SimpleEQLinuxAudioProcessor::updateFilters() {

    ChainSettings newChainSettings = getChainSettings(apvts);

    // check if peak filter settings have changed
    if (
            (newChainSettings.peakFreq != currentChainSettings.peakFreq) ||
            (newChainSettings.peakQuality != currentChainSettings.peakQuality) ||
            (newChainSettings.peakGain != currentChainSettings.peakGain)
       )
    {
        auto peakCoeff = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            getSampleRate(), 
            newChainSettings.peakFreq, 
            newChainSettings.peakQuality, 
            juce::Decibels::decibelsToGain(newChainSettings.peakGain));

        // apply filter coefficients to filters in chains
        *leftChain.get<ChainPositions::Peak>().coefficients = *peakCoeff;
        *rightChain.get<ChainPositions::Peak>().coefficients = *peakCoeff;
    }
    
    // if locut settings have changed...
    if (
            (newChainSettings.loCutFreq != currentChainSettings.loCutFreq) ||
            (newChainSettings.loCutSlope != currentChainSettings.loCutSlope)
       )
    {
        // for the hi and locut filters, we use a helper function to design
        // the filter coefficients based on the slope (order)
        IIRCoeffArray loCutCoeff = 
            juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(
                    newChainSettings.loCutFreq,
                    getSampleRate(), 
                    (newChainSettings.loCutSlope + 1) * 2);

        auto& leftLoCut = leftChain.get<ChainPositions::LoCut>();
        auto& rightLoCut = rightChain.get<ChainPositions::LoCut>();

        Helpers::setCutfilterCoeff<NUM_CUTFILTER_STAGES> ( leftLoCut, loCutCoeff );
        Helpers::setCutfilterCoeff<NUM_CUTFILTER_STAGES> ( rightLoCut, loCutCoeff );
    }
    
    // if hicut settings have changed...
    if (
            (newChainSettings.hiCutFreq != currentChainSettings.hiCutFreq) ||
            (newChainSettings.hiCutSlope != currentChainSettings.hiCutSlope)
       )
    {
        IIRCoeffArray hiCutCoeff = 
            juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(
                    newChainSettings.hiCutFreq,
                    getSampleRate(), 
                    (newChainSettings.hiCutSlope + 1) * 2);

        auto& leftHiCut = leftChain.get<ChainPositions::HiCut>();
        auto& rightHiCut = rightChain.get<ChainPositions::HiCut>();

        Helpers::setCutfilterCoeff<NUM_CUTFILTER_STAGES>(leftHiCut, hiCutCoeff);
        Helpers::setCutfilterCoeff<NUM_CUTFILTER_STAGES>(rightHiCut, hiCutCoeff);
    }

    currentChainSettings = newChainSettings;

}

// @brief       Sets the filter coefficients of a sequential filter using a variadic fold expression because of the clunky get() syntax
template<size_t... filterIndicies>
void SimpleEQLinuxAudioProcessor::Helpers::setSequentialFilterCoeffs (
    CutFilter& cutfilter, 
    const IIRCoeffArray coeffArray,
    std::index_sequence<filterIndicies...>
    )
{
    const auto maxActiveFilterIndex = coeffArray.size() - 1;
    
    ([&]
     {
        if ( filterIndicies > maxActiveFilterIndex ) {
            cutfilter.setBypassed<filterIndicies>(true);
        } else {
            *cutfilter.get<filterIndicies>().coefficients = *coeffArray[filterIndicies];
            cutfilter.setBypassed<filterIndicies>(false);
        }
     }
     (), ...);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleEQLinuxAudioProcessor();
}
