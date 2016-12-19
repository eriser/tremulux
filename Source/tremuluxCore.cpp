/*
  ==============================================================================

    This file was auto-generated by the Introjucer!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "tremuluxCore.h"
#include "PluginEditor.h"
#include "tremuluxGUI.h"



//==============================================================================
String TremuluxCore::mixParamID("mix");
String TremuluxCore::bypassParamID("bypass");
String TremuluxCore::gainParamID("gain");
String TremuluxCore::blendParamID("blend");
String TremuluxCore::rateParamID[2]{"rate1", "rate2"};
String TremuluxCore::depthParamID[2]{"depth1", "depth2"};
String TremuluxCore::syncToggleParamID[2]{"syncToggle1", "syncToggle2"};
String TremuluxCore::syncModeParamID[2]{"syncMode1", "syncMode2"};

inline double computeBandpassBandwidthFromRate(const double rate)
{
    const double k = rate * rate;
    return -3.0 * k + 0.5 * rate + 3000;
}

TremuluxCore::TremuluxCore() :
logFile(APPDIR_PATH + "/tremuluxLog.txt"),
logger(logFile, "TREMULUX log", 0),

sampleRate(0),
maxModRate(0),

sineTable(make_shared<Wavetable<double> >(Wavetable<double>::Waveform::SINE, 4194304, sampleRate)),
mods{{Sine<double>(sineTable, sampleRate), Sine<double>(sineTable, sampleRate)}},
//bandPasses{{RecursiveFilter<double>(NUM_COEFFICIENTS, BANDPASS_GAIN, NUMERATOR_COEFFICIENTS, DENOMINATOR_COEFFICIENTS),
//    RecursiveFilter<double>(NUM_COEFFICIENTS, BANDPASS_GAIN, NUMERATOR_COEFFICIENTS, DENOMINATOR_COEFFICIENTS)}},
bandPasses{{RecursiveBandPass<double>(sampleRate, BANDPASS_CENTER_FREQUENCY, BANDPASS_BANDWIDTH),
            RecursiveBandPass<double>(sampleRate, BANDPASS_CENTER_FREQUENCY, BANDPASS_BANDWIDTH)}},
undoManager(new UndoManager()),
parameterManager(new AudioProcessorValueTreeState(*this, undoManager)),

bypassData(false),
mixData(1.0),
gainData(1.0),
blendData(0.5),
interpData(1024),

rateData(),
rateChanged(),
depthData(),
syncToggleData(),
syncModeData(),
      
lastBPM(120),
lastTimeSigDenominator(4), lastTimeSigNumerator(4)
{
    ////////////////
    // Tempo Syncing
    for(int i = 0; i < NUM_MODS; ++i)
    {
        rateData[i].store(2 + 2 * i);
        rateChanged[i].store(false);
        depthData[i].store(0.75);
        syncToggleData[i].store(false);
        syncModeData[i].store((SYNC_OPTIONS)(2 + i * 2));
    }

    // Set up parameter ranges
    NormalisableRange<float> genericRange(0.0, 1.0, 0.0f);
    NormalisableRange<float> hzRateRange(0.1, RATE_DIAL_RANGE, 0.0f, 1.5);
    NormalisableRange<float> syncedRateRange(0, RATE_DIAL_RANGE, 1.0f);
    NormalisableRange<float> toggleRange(0.0, 1.0, 1);
    NormalisableRange<float> plusSixDbRange(1.0, 2.0, 0.0);
    
    // Helps to avoid bugs..
    unsigned int oscillatorID;
    
    //////////////////////////
    // Oscillator I Parameters
    //
    // Rate I
    oscillatorID = 0;
    parameterManager->createAndAddParameter(rateParamID[oscillatorID], "Rate I (Hz)", TRANS("Rate I (Hz)"),
                                            hzRateRange, hzRateRange.snapToLegalValue(rateData[oscillatorID].load()),
                                            hzRateValueToTextFunction, hzRateTextToValueFunction);
    parameterManager->addParameterListener(rateParamID[oscillatorID], this);
    
    parameterManager->createAndAddParameter(syncModeParamID[oscillatorID], "Rate I (Synced)", TRANS("Rate I (Synced)"),
                                            syncedRateRange, syncedRateRange.snapToLegalValue(syncModeData[oscillatorID].load()),
                                            syncedRateValueToTextFunction, syncedRateTextToValueFunction);
    parameterManager->addParameterListener(syncModeParamID[oscillatorID], this);
    
    // Tempo Sync I
    parameterManager->createAndAddParameter(syncToggleParamID[oscillatorID], "Tempo Sync I", TRANS("Tempo Sync I"),
                                            toggleRange, toggleRange.snapToLegalValue(syncToggleData[oscillatorID].load()),
                                            genericValueToTextFunction, genericTextToValueFunction);
    parameterManager->addParameterListener(syncToggleParamID[oscillatorID], this);
    
    // Depth I
    parameterManager->createAndAddParameter(depthParamID[oscillatorID], "Depth I", TRANS("Depth I"),
                                            genericRange, genericRange.snapToLegalValue(depthData[oscillatorID].load()),
                                            percentValueToTextFunction, percentTextToValueFunction);
    parameterManager->addParameterListener(depthParamID[oscillatorID], this);

    ///////////////////////////
    // Oscillator II Parameters
    //
    // Rate II
    oscillatorID = 1;
    parameterManager->createAndAddParameter(rateParamID[oscillatorID], "Rate II (Hz)", TRANS("Rate II (Hz)"),
                                            hzRateRange, hzRateRange.snapToLegalValue(rateData[oscillatorID].load()),
                                            hzRateValueToTextFunction, hzRateTextToValueFunction);
    parameterManager->addParameterListener(rateParamID[oscillatorID], this);
    
    parameterManager->createAndAddParameter(syncModeParamID[oscillatorID], "Rate II (Synced)", TRANS("Rate II (Synced)"),
                                            syncedRateRange, syncedRateRange.snapToLegalValue(syncModeData[oscillatorID].load()),
                                            syncedRateValueToTextFunction, syncedRateTextToValueFunction);
    parameterManager->addParameterListener(syncModeParamID[oscillatorID], this);
    
    // Tempo Sync II
    parameterManager->createAndAddParameter(syncToggleParamID[oscillatorID], "Tempo Sync II", TRANS("Tempo Sync II"),
                                            toggleRange, toggleRange.snapToLegalValue(syncToggleData[oscillatorID].load()),
                                            genericValueToTextFunction, genericTextToValueFunction);
    parameterManager->addParameterListener(syncToggleParamID[oscillatorID], this);
    
    // Depth II
    parameterManager->createAndAddParameter(depthParamID[oscillatorID], "Depth II", TRANS("Depth II"),
                                            genericRange, genericRange.snapToLegalValue(depthData[oscillatorID].load()),
                                            percentValueToTextFunction, percentTextToValueFunction);
    parameterManager->addParameterListener(depthParamID[oscillatorID], this);
    ////////////////////
    // Master Parameters
    //
    // Mix
    parameterManager->createAndAddParameter(mixParamID, "Mix", TRANS("Mix"),
                                 genericRange, genericRange.snapToLegalValue(mixData.load()),
                                            percentValueToTextFunction, percentTextToValueFunction);
    parameterManager->addParameterListener(mixParamID, this);
    
    // Bypass
    parameterManager->createAndAddParameter(bypassParamID, "Bypass", TRANS("Bypass"),
                                            toggleRange, toggleRange.snapToLegalValue(bypassData.load()),
                                            genericValueToTextFunction, genericTextToValueFunction);
    parameterManager->addParameterListener(bypassParamID, this);
    
    // Gain
    parameterManager->createAndAddParameter(gainParamID, "Gain", TRANS("Gain"),
                                            plusSixDbRange, plusSixDbRange.snapToLegalValue(gainData.load()),
                                            dbValueToTextFunction, dbTextToValueFunction);
    parameterManager->addParameterListener(gainParamID, this);
    
    // Blend
    parameterManager->createAndAddParameter(blendParamID, "Blend", TRANS("Blend"),
                                            genericRange, genericRange.snapToLegalValue(blendData.load()),
                                            percentValueToTextFunction, percentTextToValueFunction);
    parameterManager->addParameterListener(blendParamID, this);
    
    parameterManager->state = ValueTree("Tremulux");
}

TremuluxCore::~TremuluxCore()
{}

//==============================================================================
const String TremuluxCore::getName() const
{
    return JucePlugin_Name;
}

void TremuluxCore::parameterChanged(const String &parameterID, float newValue)
{
    // Master Parameters
    if(parameterID == mixParamID)
    {
        mixData.store(newValue);
    }
    else if(parameterID == bypassParamID)
    {
        bypassData.store(newValue);
    }
    else if(parameterID == gainParamID)
    {
        gainData.store(newValue);
    }
    else if(parameterID == blendParamID)
    {
        blendData.store(newValue);
    }
    // Oscillator Parameters
    else
    {
        for(int i = 0; i < NUM_MODS; ++i)
        {
            if(parameterID == rateParamID[i])
            {
                rateData[i].store(newValue);
                rateChanged[i].store(true);
            }
            else if(parameterID == syncModeParamID[i])
            {
                syncModeData[i].store(newValue);
                rateChanged[i].store(true);
            }
            else if(parameterID == syncToggleParamID[i])
            {
                syncToggleData[i].store(newValue);
                rateChanged[i].store(true);
            }
            else if(parameterID == depthParamID[i])
            {
                depthData[i].store(newValue);
            }
        }
    }
}

AudioProcessorValueTreeState& TremuluxCore::getParameterManager()
{
    return *parameterManager;
}

const String TremuluxCore::getInputChannelName (int channelIndex) const
{
    return String (channelIndex + 1);
}

const String TremuluxCore::getOutputChannelName (int channelIndex) const
{
    return String (channelIndex + 1);
}

bool TremuluxCore::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool TremuluxCore::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool TremuluxCore::silenceInProducesSilenceOut() const
{
    return false;
}

double TremuluxCore::getTailLengthSeconds() const
{
    return 0.0;
}

int TremuluxCore::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int TremuluxCore::getCurrentProgram()
{
    return 0;
}

void TremuluxCore::setCurrentProgram (int index)
{
}

const String TremuluxCore::getProgramName (int index)
{
    return String();
}

void TremuluxCore::changeProgramName (int index, const String& newName)
{}

//==============================================================================
void TremuluxCore::prepareToPlay (double sr, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    if(sr != sampleRate)
    {
        sampleRate = sr;
        sineTable->init(sampleRate);
        for(int i = 0; i < NUM_MODS; ++i)
        {
            const float rate = rateData[i].load(),
                        depth = depthData[i].load();
            mods[i].init(sampleRate);
            mods[i].start(depth, rate, 0.0);
            bandPasses[i].setBandwidth(computeBandpassBandwidthFromRate(rate), sampleRate);
        }
    }
    interpData.store(samplesPerBlock);
    numInputChannels = getMainBusNumInputChannels();
    numOutputChannels = getMainBusNumOutputChannels();
    
    isMono = (numInputChannels == 1 && numOutputChannels == 1);
    isStereoToMono = (numOutputChannels == 2 && numOutputChannels == 1);
    
    blendBuffer.setSize(numOutputChannels, samplesPerBlock);
}

void TremuluxCore::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool TremuluxCore::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Only mono/stereo and input/output must have same layout
    const AudioChannelSet& mainInput  = layouts.getMainInputChannelSet();
    const AudioChannelSet& mainOutput = layouts.getMainOutputChannelSet();
    
    // do not allow disabling the main buses
    if(mainInput.isDisabled())
    {
        return false;
    }
    
    // No more than two channels
    if(mainInput.size() > 2 || mainOutput.size() > 2)
    {
        return false;
    }
    
    return true;
}

void TremuluxCore::numChannelsChanged()
{
//    numInputChannels = getTotalNumInputChannels();
//    numOutputChannels = getTotalNumOutputChannels();
//    
//    isMono = (numInputChannels == 1 && numOutputChannels == 1);
//    isStereoToMono = (numOutputChannels == 2 && numOutputChannels == 1);
//    
//    if(numOutputChannels == 1)
//    {
//        if(gui && gui->blendDial)
//        {
//            gui->blendDial->setEnabled(false);
//        }
//    }
//    
}

void TremuluxCore::numBusesChanged()
{
//    numInputChannels = getTotalNumInputChannels();
//    numOutputChannels = getTotalNumOutputChannels();
//    
//    isMono = (numInputChannels == 1 && numOutputChannels == 1);
//    isStereoToMono = (numOutputChannels == 2 && numOutputChannels == 1);
//    
//    if(numOutputChannels == 1)
//    {
//        if(gui && gui->blendDial)
//        {
//            gui->blendDial->setEnabled(false);
//        }
//    }
//    
}

void TremuluxCore::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // I've added this to avoid people getting screaming feedback
    // when they first compile the plugin, but obviously you don't need to
    // this code if your algorithm already fills all the output channels.
    
    const bool isActive = !bypassData.load();
    if(isActive)
    {
        // Update oscillators to reflect any potential changes in gui/transport
        updateOscillators(interpData.load());
        
        const unsigned int numSamples = buffer.getNumSamples();
        const float mix = mixData.load(),
                    mixComplement = 1.0 - mix,
        gain = gainData.load(),
        blend = blendData.load() * 0.5,
        blendComplement = 1.0 - blend;
        
        if(isStereoToMono)
        {
            // if stereo input but mono output, sum the channels before processing
            const float *leftData = buffer.getReadPointer(0),
            *rightData = buffer.getReadPointer(1);
            float *monoData = blendBuffer.getWritePointer(0);
            for(int i = 0; i < numSamples; ++i)
            {
                const float leftInput = *leftData++,
                rightInput = *rightData++;
                *monoData++ = blend * rightInput + blendComplement * leftInput;
            }
        }
        
        
        const ScopedLock sl (callbackLock);
        for(int outChannel = 0; outChannel < numOutputChannels; ++outChannel)
        {
            float* outData = buffer.getWritePointer(outChannel);
            const unsigned int inChannel = (numOutputChannels > numInputChannels)? 0 : outChannel;
            const float* inData = (isStereoToMono)? blendBuffer.getReadPointer(inChannel) : buffer.getReadPointer(inChannel);
            const int stopCondition = (numOutputChannels == 2)? outChannel + 1 : NUM_MODS;
            for(int i = 0; i < numSamples; ++i)
            {
                const float input = *inData++;
                float output = input, modulation = 0;
                output = bandPasses[outChannel].next(input);
                
                // For mono output configurations, both oscillators modulate the same signal
                for(int m = outChannel; m < stopCondition; ++m)
                {
                    modulation += mods[m].next();
                }

                output *= (1.0 + modulation);
                *outData++ = gain * (mix * output + mixComplement * input);
            }
        }
        if(numOutputChannels == 2)
        {
            // For stereo output configurations, blend determines crosstalk
            const float *leftData = buffer.getReadPointer(0),
                        *rightData = buffer.getReadPointer(1);
            float *leftBlendData = blendBuffer.getWritePointer(0),
                    *rightBlendData = blendBuffer.getWritePointer(1);
            for(int i = 0; i < numSamples; ++i)
            {
                const float leftInput = *leftData++,
                            rightInput = *rightData++;
                *leftBlendData++ = blend * rightInput + blendComplement * leftInput;
                *rightBlendData++ = blend * leftInput + blendComplement * rightInput;
            }
            for(int channel = 0; channel < numOutputChannels; ++channel)
            {
                buffer.copyFrom(0, 0, blendBuffer, 0, 0, numSamples);
                buffer.copyFrom(1, 0, blendBuffer, 1, 0, numSamples);
            }
        }
    }
    else
    {
        processBlockBypassed(buffer, midiMessages);
    }
}

//==============================================================================
bool TremuluxCore::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* TremuluxCore::createEditor()
{
    return new NewProjectAudioProcessorEditor (*this);
}

//==============================================================================
void TremuluxCore::getStateInformation(MemoryBlock& destData)
{
    // Create an outer XML element..
    XmlElement xml ("Tremulux");
    
    serialize(xml);
    // then use this helper function to stuff it into the binary blob and return it..
    copyXmlToBinary(xml, destData);
}

void TremuluxCore::setStateInformation(const void* data, int sizeInBytes)
{
    ScopedPointer<XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    
    if (xmlState != nullptr)
    {
        deserialize(*xmlState);
    }
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TremuluxCore();
}


//==============================================================================
// Custom methods
void TremuluxCore::setGUI(TremuluxGUI* frontend)
{
    gui = frontend;
}

inline int TremuluxCore::getSyncMode(const int modID)
{
    return (SYNC_OPTIONS)std::floorf(rateData[modID].load());
}

float TremuluxCore::getSyncedRate(const int modID)
{
    // This should only be called for synced oscillators
    const int mode = syncModeData[modID].load();
    const float bpm = lastBPM.load();
    const unsigned int timeSigNumerator = lastTimeSigNumerator.load(),
    timeSigDenominator = lastTimeSigDenominator.load();
    
    float rate = 0;
    
    if(mode == TWO_BARS || mode == ONE_BAR)
    {
        float beat;
        switch(timeSigDenominator)
        {
            case 2:
                beat = SYNC_MODE_FACTORS[HALF];
                break;
            case 4:
                beat = SYNC_MODE_FACTORS[QUARTER];
                break;
            case 8:
                beat = SYNC_MODE_FACTORS[SIXTEENTH];
                break;
            default:
                // This shouldn't happen
                beat = SYNC_MODE_FACTORS[EIGHTH];
                break;
        }
        rate = beat * bpm / timeSigNumerator;
        if(mode == TWO_BARS)
        {
            rate *= 0.5;
        }
    }
    else
    {
        rate = SYNC_MODE_FACTORS[mode] * bpm;
    }
    return rate;
}

float TremuluxCore::getUnsyncedRate(const int modID)
{
   return MIN_FREE_RATE + (rateData[modID].load() * ONE_BY_RATE_DIAL_RANGE) * (MAX_FREE_RATE - MIN_FREE_RATE);
}

void TremuluxCore::updateOscillators(const int interpolationLength)
{
    // Check for changes in tempo and time signature
    const bool transportChanged = updateTransportInfo();
    for(int i = 0; i < NUM_MODS; ++i)
    {
        const float oldRate = mods[i].getTargetFrequency();
        float newRate = oldRate,
        oldDepth = mods[i].getTargetAmplitude(),
        newDepth = depthData[i].load();
    
        if(newDepth != oldDepth)
        {
            // For mono output configurations, both oscillators modulate the same signal
            if(numOutputChannels == 1)
            {
                newDepth *= MODULATION_SCALING_FACTOR;
            }
            mods[i].updateAmp(newDepth, interpolationLength);
        }
        
        // If oscillator is synced...
        if(syncToggleData[i].load())
        {
            if(transportChanged || rateChanged[i])
            {
                newRate = getSyncedRate(i);
            }
        }
        // If oscillator is not synced...
        else if(rateChanged[i])
        {
            newRate = getUnsyncedRate(i);
        }
        
        if(oldRate != newRate)
        {
            mods[i].updateFreq(newRate, interpolationLength);
        }
    }
}

bool TremuluxCore::updateTransportInfo(const bool force)
{
    // Called at beginning of processBlock iff at least one oscillator is synced
    transport = getPlayHead();
    transport->getCurrentPosition(transportInfo);
    float bpm = transportInfo.bpm;
    int timeSigDenominator = transportInfo.timeSigDenominator,
    timeSigNumerator = transportInfo.timeSigNumerator;
    
    // Only update if transport data has changed
    if(force || bpm != lastBPM ||
       timeSigDenominator != lastTimeSigDenominator ||
       timeSigNumerator != lastTimeSigNumerator)
    {
        lastBPM.store(bpm);
        lastTimeSigDenominator.store(timeSigDenominator);
        lastTimeSigNumerator.store(timeSigNumerator);
        return true;
    }
    return false;
}

void TremuluxCore::serialize(XmlElement& xml)
{
    // Store the values of all our parameters, using their param ID as the XML attribute
    for (int i = 0; i < getNumParameters(); ++i)
    {
        if(AudioProcessorParameterWithID* p = dynamic_cast<AudioProcessorParameterWithID*> (getParameters().getUnchecked(i)))
        {
            xml.setAttribute (p->paramID, p->getValue());
        }
    }
}

void TremuluxCore::deserialize(XmlElement& xml)
{
    // make sure that it's actually our type of XML object..
    if(xml.hasTagName ("Tremulux"))
    {
        // Now reload our parameters..
        for(int i = 0; i < getNumParameters(); ++i)
        {
            if(AudioProcessorParameterWithID* p = dynamic_cast<AudioProcessorParameterWithID*> (getParameters().getUnchecked(i)))
            {
                p->setValue ((float) xml.getDoubleAttribute (p->paramID, p->getValue()));
            }
        }
    }
}