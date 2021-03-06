/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 4.2.4

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

#ifndef __JUCE_HEADER_7A0AAA5230769E18__
#define __JUCE_HEADER_7A0AAA5230769E18__

//[Headers]     -- You can add your own extra header files here --
#include "JuceHeader.h"
#include "PluginProcessor.h"
#include "flatLookAndFeel.h"
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class PluginInterface  : public AudioProcessorEditor,
                         public Timer,
                         public SliderListener,
                         public ButtonListener
{
public:
    //==============================================================================
    PluginInterface (TremuluxAudioProcessor& p);
    ~PluginInterface();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    void timerCallback() override;
    void setRateDialRanges(const float max);

    void visibilityChanged() override;
    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;
    void buttonClicked (Button* buttonThatWasClicked) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    TremuluxAudioProcessor& processor;
    float lastUnsyncedFreqs[2]{5.0, 8.0};
    float lastSyncedFreqs[2]{5.0, 8.0};
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<Slider> modRateDial1;
    ScopedPointer<Label> label;
    ScopedPointer<Slider> modDepth1;
    ScopedPointer<Label> label2;
    ScopedPointer<Slider> mix;
    ScopedPointer<Label> label5;
    ScopedPointer<Slider> modRateDial2;
    ScopedPointer<Label> label6;
    ScopedPointer<Slider> modDepth2;
    ScopedPointer<Label> label7;
    ScopedPointer<ToggleButton> modSyncButton1;
    ScopedPointer<ToggleButton> modSyncButton2;
    ScopedPointer<Label> modFreqText1;
    ScopedPointer<Label> modFreqText2;
    ScopedPointer<Label> modDepthText1;
    ScopedPointer<Label> modDepthText2;
    ScopedPointer<Label> mixText;
    Path internalPath1;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginInterface)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

#endif   // __JUCE_HEADER_7A0AAA5230769E18__
