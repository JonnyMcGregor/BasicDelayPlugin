/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
LapseAudioProcessor::LapseAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       ),
	parameters(*this, nullptr, Identifier("SimpleDelay"),
						   {             
							 std::make_unique<AudioParameterFloat>("delayTime",
																   "Delay Time",
																   0.0f,
																   2000.0f,
																   1000.0f),
							 std::make_unique<AudioParameterFloat>("feedback",
																  "Feedback",
																   0,
																   1.0,
																   0.5),
							 std::make_unique<AudioParameterFloat>("panPosition",
																   "Pan",
																	0,
																	1,
																	0.5),
							 std::make_unique<AudioParameterBool>("quantiseDelayTime",
																  "Quantise Delay Time",
																  true),
							 std::make_unique<AudioParameterFloat>("timerValue",
																	"Node Change Time",
																	1,
																	5,
																	3
																  )						 
						   })

#endif
{
	delayParameter = parameters.getRawParameterValue("delayTime");
	feedbackParameter = parameters.getRawParameterValue("feedback");
	panParameter = parameters.getRawParameterValue("panPosition");
	timeModeParameter = parameters.getRawParameterValue("quantiseDelayTime");
	timerInterval = parameters.getRawParameterValue("timerValue");
}

LapseAudioProcessor::~LapseAudioProcessor()
{
}

//==============================================================================
const String LapseAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool LapseAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool LapseAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool LapseAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double LapseAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int LapseAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int LapseAudioProcessor::getCurrentProgram()
{
    return 0;
}

void LapseAudioProcessor::setCurrentProgram (int index)
{
}

const String LapseAudioProcessor::getProgramName (int index)
{
    return {};
}

void LapseAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void LapseAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	lastSampleRate = sampleRate;

	const int numberInputChannels = getTotalNumInputChannels();
	
	const int delayBufferSize = sampleRate * samplesPerBlock;

	delayBuffer.setSize(numberInputChannels, delayBufferSize);
	dryBuffer.setSize(numberInputChannels, samplesPerBlock);
	reverseBuffer.setSize(numberInputChannels, delayBufferSize);

	delayBuffer.clear();
	dryBuffer.clear();
	reverseBuffer.clear();

	delayContainer.initialise(sampleRate, samplesPerBlock, delayBufferSize);
    panSmoothed.reset(sampleRate, 0.0015);
    
	playHead = getPlayHead();
	if (playHead != nullptr)
	{
		playHead->getCurrentPosition(playposinfo);
		bpm = playposinfo.bpm;
		timeSigNumerator = playposinfo.timeSigNumerator;
		currentBeat = playposinfo.ppqPosition;
		calculateNoteLengths();
        startTimer(halfNoteInSeconds*1000);
	}
}

void LapseAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool LapseAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
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

void LapseAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
	for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
		buffer.clear(i, 0, buffer.getNumSamples());
    
    //Some hosts are inconsistent with their calling of prepareToPlay(), therefore we must
    //check that the delayContainer is properly initialised, and if not, initialise it. This
    //should only happen either on the first call of processBlock, or if the buffer size changes.
    
    if(dryBuffer.getNumSamples() != buffer.getNumSamples())
    {
        const int delayBufferSize = 2.0f * (getSampleRate() + buffer.getNumSamples());
        
        delayBuffer.setSize(totalNumInputChannels, delayBufferSize);
        dryBuffer.setSize(totalNumInputChannels, buffer.getNumSamples());
        reverseBuffer.setSize(totalNumInputChannels, delayBufferSize);
        
        delayBuffer.clear();
        dryBuffer.clear();
        reverseBuffer.clear();
        
        delayContainer.initialise(getSampleRate(), buffer.getNumSamples(), delayBufferSize);
    }
    
	//Get bpm and time info from daw:
	playHead = getPlayHead();
	if (playHead != nullptr)
	{
		playHead->getCurrentPosition(playposinfo);
		bpm = playposinfo.bpm;
		timeSigNumerator = playposinfo.timeSigNumerator;
		currentBeat = playposinfo.ppqPosition;
		calculateNoteLengths();
        if(!isTimerRunning())
        {
            startTimer(halfNoteInSeconds*1000);
        }
	}
	
	//Set up parameters:
	const int bufferLength = buffer.getNumSamples();
	const int delayBufferLength = delayBuffer.getNumSamples();

    float delayTime = *delayParameter;
	float feedback = *feedbackParameter;
	float panValue = *panParameter;
    
	//Delay Processing:
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
		delayContainer.fillDryBuffer(channel, buffer, dryBuffer);
		delayContainer.fillDelayBuffer(channel, buffer, delayBuffer);
		delayContainer.initialDelayEffect(channel, buffer, delayBuffer, delayTime);
		delayContainer.mixBuffers(channel, buffer, dryBuffer, 0.5);
		delayContainer.feedbackDelay(channel, buffer, delayBuffer, oldFeedback, feedback);
		
		panAudio(channel, buffer, panValue);
		oldFeedback = feedback;
    }
	//update writePosition for delay processing
	writePosition += bufferLength;
	writePosition %= delayBufferLength;
}

void LapseAudioProcessor::calculateNoteLengths()
{
		quarterNoteInSeconds = 60 / bpm;
		halfNoteInSeconds = quarterNoteInSeconds * 2;
		oneBarInSeconds = halfNoteInSeconds * 2;
		twoBarsInSeconds = oneBarInSeconds * 2;
		eighthNoteInSeconds = quarterNoteInSeconds * 0.5;
		sixteenthNoteInSeconds = eighthNoteInSeconds * 0.5;
		thirtySecondNoteInSeconds = sixteenthNoteInSeconds * 0.5;
}

void LapseAudioProcessor::panAudio(int channel, AudioBuffer<float> audioBuffer, float panValue)
{
    if(numberOfVisibleNodes == 1)
        panSmoothed.setTargetValue(panValue);
    
    panValue = panSmoothed.getNextValue();
	for (int sample = 0; sample < audioBuffer.getNumSamples(); sample++)
	{
		if (channel == 0)
			audioBuffer.setSample(channel, sample, audioBuffer.getSample(channel, sample) * cos(panValue*pi/2));
		else
			audioBuffer.setSample(channel, sample, audioBuffer.getSample(channel, sample) * sin(panValue*pi/2));
	}
}

float LapseAudioProcessor::smoothParameterChange(float& currentValue, float& previousValue)
{
	currentValue = previousValue + ((currentValue - previousValue) * 0.01);
	return currentValue;
}

void LapseAudioProcessor::timerCallback()
{
	if (!isFirstTimeOpeningEditor && numberOfVisibleNodes > 1)
	{
		previousDelayNode = currentDelayNode;

		if (oldTimerValue != *timerValues[(int)*timerInterval - 1])
		{
			startTimer(*timerValues[(int)*timerInterval - 1] * 1000);
			oldTimerValue = *timerValues[(int)*timerInterval - 1];
		}
        
		changeCurrentDelayNode();
        
		if (previousDelayNode != currentDelayNode)
		{
			updatePanParameter();
			updateFeedbackParameter();
			updateDelayTimeParameter();
            panSmoothed.setTargetValue(parameters.getParameter("panPosition")->getValue());
        }

	}

	firstBeatOfBar.sendChangeMessage();
}

void LapseAudioProcessor::changeCurrentDelayNode()
{
	if (nodes[currentDelayNode].isDelayNode)
	{
		nodes[currentDelayNode].isDelayNode = false;
	}

	if (currentDelayNode < numberOfVisibleNodes && numberOfVisibleNodes > 1)
	{
		currentDelayNode++;
		if (currentDelayNode == numberOfVisibleNodes)
		{
			currentDelayNode = 0;
		}
	}

	else
		currentDelayNode = 0;

	nodes[currentDelayNode].isDelayNode = true;
}

void LapseAudioProcessor::updatePanParameter()
{
	float pan = jmap(nodes[currentDelayNode].getXPosition(), 200.0f, 555.0f, 0.0f, 1.0f);
	parameters.getParameter("panPosition")->beginChangeGesture();
	parameters.getParameter("panPosition")->setValueNotifyingHost(pan);
	parameters.getParameter("panPosition")->endChangeGesture();
}

void LapseAudioProcessor::updateFeedbackParameter()
{
	float feedback = jmap(nodes[currentDelayNode].getDiameter(), 10.0f, 40.0f, 0.0f, 1.0f);
	parameters.getParameter("feedback")->beginChangeGesture();
	parameters.getParameter("feedback")->setValueNotifyingHost(feedback);
	parameters.getParameter("feedback")->endChangeGesture();
}

void LapseAudioProcessor::updateDelayTimeParameter()
{
	float delayTime = jmap(nodes[currentDelayNode].getYPosition(), 333.0f, 45.0f, 0.0f, 1.0f);
	parameters.getParameter("delayTime")->beginChangeGesture();
	parameters.getParameter("delayTime")->setValueNotifyingHost(delayTime);
	parameters.getParameter("delayTime")->endChangeGesture();
}

//==============================================================================
bool LapseAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* LapseAudioProcessor::createEditor()
{
    return new LapseAudioProcessorEditor (*this, parameters, firstBeatOfBar);
}

//==============================================================================
void LapseAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    auto state = parameters.copyState();
    std::unique_ptr<XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void LapseAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    std::unique_ptr<XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
            parameters.replaceState (ValueTree::fromXml (*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LapseAudioProcessor();
}

//==============================================================================
