/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"
#include "Node.h"
#include "LapseLookAndFeel.h"
#include <vector>
#include <cmath>
//==============================================================================
/**
*/
class LapseAudioProcessorEditor  : public AudioProcessorEditor, public ChangeListener
{
public:
    LapseAudioProcessorEditor (LapseAudioProcessor&, AudioProcessorValueTreeState&, ChangeBroadcaster&);
    ~LapseAudioProcessorEditor();

	void setUpAttachments();

    //==============================================================================
	void resized() override;
	void paint (Graphics&) override;

	void drawStaticUIElements(Graphics&);
    
    void mouseDown(const MouseEvent&) override;
	void mouseDoubleClick(const MouseEvent&) override;
	void mouseDrag(const MouseEvent&) override;

	void selectNodeForMovement(const MouseEvent&);
    void updateNodeSize(const MouseEvent &m, Node&);
	void updateNodePosition(const MouseEvent &m, Node&);
	void keepNodeInField(float&, float&, Node selectedNode);

	float quantisePosition(float position, float noteLengthInMS);

	void drawQuantiseGrid(Graphics&);
	void drawNodeConnectorLines(Graphics&, int i, std::vector<Node>&);
	void drawBorderOnSelectedNode(Graphics&, Node node);
	
	void changeListenerCallback(ChangeBroadcaster *source) override;

	std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> timeModeAttachment;
	std::unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment> nodeTimingBoxAttachment;

private:
	ChangeBroadcaster& broadcaster;
	
	Font largeFont = Font(Typeface::createSystemTypefaceFor(BinaryData::RobotoThin_ttf, BinaryData::RobotoThin_ttfSize));
	Font mediumFont = Font(Typeface::createSystemTypefaceFor(BinaryData::RobotoThin_ttf, BinaryData::RobotoThin_ttfSize));
	Font smallFont = Font(Typeface::createSystemTypefaceFor(BinaryData::RobotoThin_ttf, BinaryData::RobotoThin_ttfSize));
	
	Colour textColour = Colour::fromRGB(114, 114, 114);
	
	ToggleButton quantiseButton;

	ComboBox nodeTimingBox;

	LapseLookAndFeel lf;

    LapseAudioProcessor& processor;

	AudioProcessorValueTreeState& state;

	Rectangle<float> nodeField;
    
    Colour backgroundColour = Colour::fromRGB(221, 221, 221);
    
	Colour nodeColour[10]{ Colour::fromRGB(64, 94, 221), 
						   Colour::fromRGB(72, 181, 15),
						   Colour::fromRGB(239, 29, 129),
						   Colour::fromRGB(221, 162, 31),
						   Colour::fromRGB(239, 20, 20),
						   Colour::fromRGB(64, 94, 221),
						   Colour::fromRGB(72, 181, 15),
						   Colour::fromRGB(239, 29, 129),
						   Colour::fromRGB(221, 162, 31),
						   Colour::fromRGB(239, 20, 20)};

	int maximumNodeSize = 40;
    int minimumNodeSize = 10;
	int defaultNodeSize = 20;
    int maximumNumberOfNodes = 10;
	ModifierKeys modKeys;

	float feedback = 0;
	float pan = 0;
	float delayTime = 0;
	Node *selectedNode = nullptr;
	
    float previousNodeWidth;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LapseAudioProcessorEditor)
};
