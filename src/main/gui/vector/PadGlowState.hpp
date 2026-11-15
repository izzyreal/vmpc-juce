#pragma once

#include "controller/Bank.hpp"

#include <algorithm>
#include <array>
#include <optional>

namespace vmpc_juce::gui::vector
{
    // UI-thread state for one physical pad position. Program events retain
    // their bank identity; selecting a bank only changes the rendered layers.
    class PadGlowState
    {
    public:
        struct Layers
        {
            float physical = 0.f;
            float selectedBank = 0.f;
            float otherBanks = 0.f;
        };

        void registerPhysicalPress(const mpc::Velocity velocity)
        {
            registerPress(physicalPress, velocity);
        }

        bool registerPhysicalAftertouch(const mpc::Pressure pressure)
        {
            return registerAftertouch(physicalPress, pressure);
        }

        bool registerPhysicalRelease()
        {
            return registerRelease(physicalPress);
        }

        void registerProgramPress(const mpc::ProgramPadIndex index,
                                  const mpc::Velocity velocity)
        {
            registerPress(programPressFor(index), velocity);
        }

        bool registerProgramAftertouch(const mpc::ProgramPadIndex index,
                                       const mpc::Pressure pressure)
        {
            return registerAftertouch(programPressFor(index), pressure);
        }

        bool registerProgramRelease(const mpc::ProgramPadIndex index)
        {
            return registerRelease(programPressFor(index));
        }

        bool setActiveBank(const mpc::controller::Bank bank)
        {
            if (activeBank == bank)
            {
                return false;
            }
            activeBank = bank;
            return true;
        }

        bool isPhysicallyPressed() const
        {
            return physicalPress && physicalPress->pressCount > 0;
        }

        Layers getLayers() const
        {
            Layers layers;
            layers.physical = getAlpha(physicalPress);
            for (size_t bank = 0; bank < programPresses.size(); ++bank)
            {
                const auto alpha = getAlpha(programPresses[bank]);
                if (bank == static_cast<size_t>(activeBank))
                {
                    layers.selectedBank = alpha;
                }
                else
                {
                    layers.otherBanks = std::max(layers.otherBanks, alpha);
                }
            }
            return layers;
        }

        bool advance()
        {
            bool changed = processDecay(physicalPress, 0.8f);
            for (auto &press : programPresses)
            {
                changed = processDecay(press, 0.6f) || changed;
            }
            return changed;
        }

        // Physical feedback is displayed by the glow SVG on the timer; the
        // program layers are displayed by Pad::paint(). Acknowledge all banks,
        // including weaker blue layers, so none waits forever to start fading.
        void markPhysicalGlowDisplayed()
        {
            markDisplayed(physicalPress);
        }

        void markProgramGlowsDisplayed()
        {
            for (auto &press : programPresses)
            {
                markDisplayed(press);
            }
        }

    private:
        struct Press
        {
            mpc::VelocityOrPressure velocityOrPressure;
            enum class Phase
            {
                Immediate,
                Sustained,
                Releasing
            } phase = Phase::Immediate;
            float alpha = 1.f;
            int pressCount = 1;
            bool wasDisplayed = false;
        };

        std::optional<Press> physicalPress;
        std::array<std::optional<Press>,
                   mpc::Mpc2000XlSpecs::PROGRAM_BANK_COUNT>
            programPresses;
        mpc::controller::Bank activeBank = mpc::controller::Bank::A;

        std::optional<Press> &programPressFor(const mpc::ProgramPadIndex index)
        {
            return programPresses[static_cast<size_t>(
                mpc::controller::programPadIndexToBank(index))];
        }

        static float getAlpha(const std::optional<Press> &press)
        {
            if (!press)
            {
                return 0.f;
            }
            const auto velocity = std::clamp(
                static_cast<int>(press->velocityOrPressure), 50, 127);
            return std::clamp(press->alpha *
                                  (static_cast<float>(velocity) / 127.f),
                              0.f, 1.f);
        }

        static void registerPress(std::optional<Press> &press,
                                  const mpc::Velocity velocity)
        {
            if (press)
            {
                ++press->pressCount;
                press->velocityOrPressure = velocity;
                press->phase = Press::Phase::Immediate;
                press->alpha = 1.f;
            }
            else
            {
                press = Press{velocity};
            }
        }

        static bool registerAftertouch(std::optional<Press> &press,
                                       const mpc::Pressure pressure)
        {
            if (!press)
            {
                return false;
            }
            press->velocityOrPressure = pressure;
            return true;
        }

        static bool registerRelease(std::optional<Press> &press)
        {
            if (!press || press->pressCount == 0)
            {
                return false;
            }
            if (--press->pressCount == 0)
            {
                press->phase = Press::Phase::Releasing;
                return true;
            }
            return false;
        }

        static void markDisplayed(std::optional<Press> &press)
        {
            if (press)
            {
                press->wasDisplayed = true;
            }
        }

        static bool processDecay(std::optional<Press> &press,
                                 const float sustainAlpha)
        {
            if (!press || (press->alpha == 1.f && !press->wasDisplayed))
            {
                return false;
            }
            if (press->phase == Press::Phase::Immediate)
            {
                press->alpha -= 0.035f;
                if (press->alpha <= sustainAlpha)
                {
                    press->alpha = sustainAlpha;
                    press->phase = Press::Phase::Sustained;
                }
                return true;
            }
            if (press->phase == Press::Phase::Releasing)
            {
                press->alpha -= 0.025f;
                if (press->alpha <= 0.f)
                {
                    press.reset();
                }
                return true;
            }
            return false;
        }
    };
} // namespace vmpc_juce::gui::vector
