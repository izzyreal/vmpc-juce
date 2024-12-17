/*
    This file is part of vmpc-juce, a JUCE implementation of VMPC2000XL.

    vmpc-juce is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License (GPL) as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    vmpc-juce is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with vmpc-juce. If not, see <https://www.gnu.org/licenses/>.

    This project uses JUCE, which is licensed under the GNU Affero General Public License (AGPL).
    See <https://juce.com> for details.
*/
#pragma once

#include "juce_audio_processors/juce_audio_processors.h"

#include "gui/VmpcNoCornerResizerLookAndFeel.hpp"

namespace vmpc_juce::gui::vector { class View; }
namespace melatonin { class Inspector; }

namespace vmpc_juce {

class VmpcProcessor;

class VmpcEditor : public juce::AudioProcessorEditor
{
public:
    explicit VmpcEditor(VmpcProcessor&);
    ~VmpcEditor() override;

    void resized() override;

    static const int initial_width =  445;
    static const int initial_height = 342;

private:
    melatonin::Inspector* inspector = nullptr;
    VmpcProcessor &vmpcProcessor;
    vmpc_juce::gui::vector::View* view = nullptr;
    const float initial_scale = 1.31f;

    juce::Font *nimbusSans = nullptr;
    juce::Font *mpc2000xlFaceplateGlyphs = nullptr;
    VmpcNoCornerResizerLookAndFeel lookAndFeel;
};
}
