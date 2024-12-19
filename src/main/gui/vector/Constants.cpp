#pragma once

#include "Constants.hpp"

using namespace vmpc_juce::gui::vector;

juce::Colour Constants::chassisColour = juce::Colour::fromRGB(230, 238, 233);
juce::Colour Constants::labelColour = juce::Colour::fromRGB(90, 90, 90);
juce::Colour Constants::darkLabelColour = labelColour.darker();
juce::Colour Constants::greyFacePaintColour = juce::Colour::fromRGB(139, 151, 163);
juce::Colour Constants::betweenChassisAndLabelColour = chassisColour.darker(0.2f);

juce::Colour Constants::lcdOn = juce::Colour::fromRGB(86, 61, 135).darker().darker();
juce::Colour Constants::lcdOnLight = Constants::lcdOn.brighter(0.8f).darker();
//juce::Colour Constants::lcdOffBacklit = juce::Colour::fromRGB(170, 225, 180).brighter(0.f);
juce::Colour Constants::lcdOffBacklit = juce::Colours::yellowgreen.brighter();
juce::Colour Constants::lcdOff = lcdOffBacklit/*.darker(0.4f)*/;

float Constants::BASE_FONT_SIZE = 5.8f;
float Constants::LINE_SIZE = BASE_FONT_SIZE * 0.1f;

float Constants::lineThickness1 = 0.5f;
float Constants::lineThickness2 = 3.f;
