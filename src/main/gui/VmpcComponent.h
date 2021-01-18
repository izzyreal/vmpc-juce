/*
  ==============================================================================

    VmpcComponent.h
    Created: 11 Jan 2018 8:31:02am
    Author:  Izmar

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "InputCatcherControl.h"

//==============================================================================
/*
*/
class VmpcComponent
	: public juce::Component
{
public:
    VmpcComponent(const String& componentName = "")
    {
        setWantsKeyboardFocus(false);
    }

	VmpcComponent()
	{
        setWantsKeyboardFocus(false);
	}

    ~VmpcComponent()
    {
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VmpcComponent)
};
