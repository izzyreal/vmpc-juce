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
#include "MouseWheelControllable.hpp"

using namespace vmpc_juce::gui;

void MouseWheelControllable::processWheelEvent(const juce::MouseWheelDetails& details,
                                                       std::function<void(int)> updateFn)
{
    auto y = -details.deltaY;
    
    acc += y;
    
    auto increment = 0;
    
    if (acc > THRESHOLD)
    {
        increment = 1;
        
        while (acc > THRESHOLD)
            acc -= THRESHOLD;
    }
    else if (abs(acc) > THRESHOLD)
    {
        increment = -1;
        
        while (abs(acc) > THRESHOLD)
            acc += THRESHOLD;
    }

    if (increment == 0) return;
    
    updateFn(increment);
}
