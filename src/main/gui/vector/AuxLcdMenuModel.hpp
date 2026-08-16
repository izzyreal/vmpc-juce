#pragma once

#include <functional>
#include <string>
#include <utility>

namespace vmpc_juce::gui::vector
{
    class AuxLcdMenuModel
    {
    public:
        explicit AuxLcdMenuModel(std::function<bool()> toggleToUse)
            : toggle(std::move(toggleToUse))
        {
        }

        bool setOpen(const bool shouldBeOpen)
        {
            if (open == shouldBeOpen)
            {
                return false;
            }
            open = shouldBeOpen;
            return true;
        }

        bool isOpen() const
        {
            return open;
        }

        bool activate()
        {
            setOpen(toggle());
            return open;
        }

        std::string tooltipText() const
        {
            return open ? "Close auxiliary LCD" : "Open auxiliary LCD";
        }

    private:
        std::function<bool()> toggle;
        bool open = false;
    };
} // namespace vmpc_juce::gui::vector
