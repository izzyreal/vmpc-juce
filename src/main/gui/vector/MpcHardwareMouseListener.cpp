#include "MpcHardwareMouseListener.hpp"

#include "hardware2/Hardware2.h"

#include "Knob.hpp"

#include "TooltipOverlay.hpp"
#include "Slider.hpp"
#include "inputlogic/HostInputEvent.h"
#include "juce_audio_processors/juce_audio_processors.h"

using namespace vmpc_juce::gui::vector;

bool MpcHardwareMouseListener::showKeyTooltipUponNextClick = false;

MpcHardwareMouseListener::MpcHardwareMouseListener(mpc::Mpc &mpcToUse, const std::string labelToUse) 
    : mpc(mpcToUse), label(labelToUse) {}

void MpcHardwareMouseListener::timerCallback()
{
    if (!hideKeyTooltipUntilAfterMouseExit && lastEventComponent != nullptr)
    {
        setKeyTooltipVisibility(lastEventComponent, true);
    }
}

void MpcHardwareMouseListener::mouseMove(const juce::MouseEvent &e)
{
    const auto currentModifiers = juce::ModifierKeys::getCurrentModifiers();

    if (!currentModifiers.isAnyModifierKeyDown())
    {
        if (isTimerRunning()) stopTimer();
        return;
    }

    lastEventComponent = e.eventComponent;
    if (!isTimerRunning()) startTimer(1000);
}

void MpcHardwareMouseListener::mouseExit(const juce::MouseEvent &)
{
    hideKeyTooltipUntilAfterMouseExit = false;
    if (isTimerRunning()) stopTimer();
    if (lastEventComponent != nullptr)
    {
        setKeyTooltipVisibility(lastEventComponent, false);
    }
    lastEventComponent = nullptr;
}

void MpcHardwareMouseListener::mouseWheelMove(const juce::MouseEvent &event, const juce::MouseWheelDetails &)
{
    setKeyTooltipVisibility(event.eventComponent, false);

    if (label == "rec_gain" || label == "main_volume")
    {
        auto pot = label == "rec_gain" ? mpc.getHardware()->getRecPot() : mpc.getHardware()->getVolPot();
        auto knob = dynamic_cast<Knob*>(event.eventComponent);
        pot->setValue(knob->getAngleFactor() * 100);
    }
    else if (label == "slider")
    {
        syncMpcSliderModelWithUi(event.eventComponent);
    }
}

std::optional<mpc::inputlogic::HostInputEvent> constructHostInputEvent(const juce::MouseEvent &e, std::string label)
{
    if (label.empty() || label == "data-wheel" || label == "rec_gain" || label == "main_volume" || label == "slider")
    {
        return std::nullopt;
    }

    using namespace mpc::inputlogic;

    HostInputEvent hostEvent;
    hostEvent.source = HostInputEvent::MOUSE;

    const float normX = e.position.getX() / static_cast<float>(e.eventComponent->getWidth());
    const float normY = e.position.getY() / static_cast<float>(e.eventComponent->getHeight());

    hostEvent.payload = MouseEvent {
        MouseEvent::ButtonState{
            e.mods.isLeftButtonDown(),
            e.mods.isMiddleButtonDown(),
            e.mods.isRightButtonDown()
        },
        MouseEvent::NONE,
        normX,
        normY,
        0.f,
        0.f,
        0.f,
        MouseEvent::BUTTON_DOWN
    };

    MouseEvent& mouseEvent = std::get<MouseEvent>(hostEvent.payload);

    using Gui = MouseEvent::GuiElement;
    if      (label == "pad-1")  mouseEvent.guiElement = Gui::PAD1;
    else if (label == "pad-2")  mouseEvent.guiElement = Gui::PAD2;
    else if (label == "pad-3")  mouseEvent.guiElement = Gui::PAD3;
    else if (label == "pad-4")  mouseEvent.guiElement = Gui::PAD4;
    else if (label == "pad-5")  mouseEvent.guiElement = Gui::PAD5;
    else if (label == "pad-6")  mouseEvent.guiElement = Gui::PAD6;
    else if (label == "pad-7")  mouseEvent.guiElement = Gui::PAD7;
    else if (label == "pad-8")  mouseEvent.guiElement = Gui::PAD8;
    else if (label == "pad-9")  mouseEvent.guiElement = Gui::PAD9;
    else if (label == "pad-10") mouseEvent.guiElement = Gui::PAD10;
    else if (label == "pad-11") mouseEvent.guiElement = Gui::PAD11;
    else if (label == "pad-12") mouseEvent.guiElement = Gui::PAD12;
    else if (label == "pad-13") mouseEvent.guiElement = Gui::PAD13;
    else if (label == "pad-14") mouseEvent.guiElement = Gui::PAD14;
    else if (label == "pad-15") mouseEvent.guiElement = Gui::PAD15;
    else if (label == "pad-16") mouseEvent.guiElement = Gui::PAD16;
    else if (label == "rec") mouseEvent.guiElement = Gui::REC;
    else if (label == "overdub") mouseEvent.guiElement = Gui::OVERDUB;
    else if (label == "stop") mouseEvent.guiElement = Gui::STOP;
    else if (label == "play") mouseEvent.guiElement = Gui::PLAY;
    else if (label == "play-start") mouseEvent.guiElement = Gui::PLAY_START;
    else if (label == "main-screen") mouseEvent.guiElement = Gui::MAIN_SCREEN;
    else if (label == "prev-step-event") mouseEvent.guiElement = Gui::PREV_STEP_EVENT;
    else if (label == "next-step-event") mouseEvent.guiElement = Gui::NEXT_STEP_EVENT;
    else if (label == "go-to") mouseEvent.guiElement = Gui::GO_TO;
    else if (label == "prev-bar-start") mouseEvent.guiElement = Gui::PREV_BAR_START;
    else if (label == "next-bar-end") mouseEvent.guiElement = Gui::NEXT_BAR_END;
    else if (label == "tap") mouseEvent.guiElement = Gui::TAP;
    else if (label == "next-seq") mouseEvent.guiElement = Gui::NEXT_SEQ;
    else if (label == "track-mute") mouseEvent.guiElement = Gui::TRACK_MUTE;
    else if (label == "open-window") mouseEvent.guiElement = Gui::OPEN_WINDOW;
    else if (label == "full-level") mouseEvent.guiElement = Gui::FULL_LEVEL;
    else if (label == "sixteen-levels") mouseEvent.guiElement = Gui::SIXTEEN_LEVELS;
    else if (label == "f1") mouseEvent.guiElement = Gui::F1;
    else if (label == "f2") mouseEvent.guiElement = Gui::F2;
    else if (label == "f3") mouseEvent.guiElement = Gui::F3;
    else if (label == "f4") mouseEvent.guiElement = Gui::F4;
    else if (label == "f5") mouseEvent.guiElement = Gui::F5;
    else if (label == "f6") mouseEvent.guiElement = Gui::F6;
    else if (label == "shift") mouseEvent.guiElement = Gui::SHIFT;
    else if (label == "enter") mouseEvent.guiElement = Gui::ENTER;
    else if (label == "undo-seq") mouseEvent.guiElement = Gui::UNDO_SEQ;
    else if (label == "erase") mouseEvent.guiElement = Gui::ERASE;
    else if (label == "after") mouseEvent.guiElement = Gui::AFTER;
    else if (label == "bank-a") mouseEvent.guiElement = Gui::BANK_A;
    else if (label == "bank-b") mouseEvent.guiElement = Gui::BANK_B;
    else if (label == "bank-c") mouseEvent.guiElement = Gui::BANK_C;
    else if (label == "bank-d") mouseEvent.guiElement = Gui::BANK_D;
    else if (label == "0") mouseEvent.guiElement = Gui::NUM_0;
    else if (label == "1") mouseEvent.guiElement = Gui::NUM_1;
    else if (label == "2") mouseEvent.guiElement = Gui::NUM_2;
    else if (label == "3") mouseEvent.guiElement = Gui::NUM_3;
    else if (label == "4") mouseEvent.guiElement = Gui::NUM_4;
    else if (label == "5") mouseEvent.guiElement = Gui::NUM_5;
    else if (label == "6") mouseEvent.guiElement = Gui::NUM_6;
    else if (label == "7") mouseEvent.guiElement = Gui::NUM_7;
    else if (label == "8") mouseEvent.guiElement = Gui::NUM_8;
    else if (label == "9") mouseEvent.guiElement = Gui::NUM_9;
    else if (label == "data-wheel") mouseEvent.guiElement = Gui::DATA_WHEEL;
    else if (label == "cursor")
    {
        juce::Path left, top, bottom, right;

        left.startNewSubPath(0.f, 0.f);
        left.lineTo(0.2f, 0.f);
        left.lineTo(0.25f, 0.5f);
        left.lineTo(0.2f, 1.f);
        left.lineTo(0.f, 1.f);
        left.closeSubPath();

        top.startNewSubPath(0.2f, 0.f);
        top.lineTo(0.8f, 0.f);
        top.lineTo(0.75f, 0.5f);
        top.lineTo(0.25f, 0.5f);
        top.lineTo(0.2f, 0.f);
        top.closeSubPath();

        right = left;
        right.applyTransform(juce::AffineTransform(-1.0f, 0.0f, 1.f, 0.0f, 1.0f, 0.0f));

        bottom = top;
        bottom.applyTransform(juce::AffineTransform().verticalFlip(1.f));

        const auto compWidth = e.eventComponent->getWidth();
        const auto compHeight = e.eventComponent->getHeight();
        juce::AffineTransform scaleTransform = juce::AffineTransform().scaled(compWidth, compHeight);

        left.applyTransform(scaleTransform);
        top.applyTransform(scaleTransform);
        right.applyTransform(scaleTransform);
        bottom.applyTransform(scaleTransform);

        if (left.contains(e.position))
        {
            mouseEvent.guiElement = MouseEvent::CURSOR_LEFT;
        }
        else if (top.contains(e.position))
        {
            mouseEvent.guiElement = MouseEvent::CURSOR_UP;
        }
        else if (right.contains(e.position))
        {
            mouseEvent.guiElement = MouseEvent::CURSOR_RIGHT;
        }
        else if (bottom.contains(e.position))
        {
            mouseEvent.guiElement = MouseEvent::CURSOR_DOWN;
        }
    }

    return hostEvent;
}

void MpcHardwareMouseListener::mouseDown(const juce::MouseEvent &e)
{
    if (showKeyTooltipUponNextClick)
    {
        setKeyTooltipVisibility(e.eventComponent, true);
        showKeyTooltipUponNextClick = false;
        return;
    }

    setKeyTooltipVisibility(e.eventComponent, false);
    hideKeyTooltipUntilAfterMouseExit = true;

    if (auto hostInputEvent = constructHostInputEvent(e, label); hostInputEvent.has_value())
    {
        std::get<mpc::inputlogic::MouseEvent>(hostInputEvent->payload).type = mpc::inputlogic::MouseEvent::BUTTON_DOWN;
        mpc.getHardware2()->dispatchHostInput(*hostInputEvent);
    }
}

void MpcHardwareMouseListener::mouseDoubleClick(const juce::MouseEvent &)
{
    if (label.length() >= 5 && label.substr(0, 5) == "shift")
    {
        showKeyTooltipUponNextClick = true;
    }
}

void MpcHardwareMouseListener::mouseUp(const juce::MouseEvent &e)
{
    if (auto hostInputEvent = constructHostInputEvent(e, label); hostInputEvent.has_value())
    {
        std::get<mpc::inputlogic::MouseEvent>(hostInputEvent->payload).type = mpc::inputlogic::MouseEvent::BUTTON_UP;
        mpc.getHardware2()->dispatchHostInput(*hostInputEvent);
    }
}

void MpcHardwareMouseListener::mouseDrag(const juce::MouseEvent &e)
{
    if (label == "rec_gain" || label == "main_volume")
    {
        auto pot = label == "rec_gain" ? mpc.getHardware()->getRecPot() : mpc.getHardware()->getVolPot();
        auto knob = dynamic_cast<Knob*>(e.eventComponent);
        pot->setValue(knob->getAngleFactor() * 100.f);
    }
    else if (label == "slider")
    {
        syncMpcSliderModelWithUi(e.eventComponent);
    }
}

void MpcHardwareMouseListener::setKeyTooltipVisibility(juce::Component *c, const bool visibleEnabled)
{
    const auto editor = c->findParentComponentOfClass<juce::AudioProcessorEditor>();
    auto tooltipOverlay = getChildComponentOfClass<TooltipOverlay>(editor);
    
    if (tooltipOverlay == nullptr)
    {
        return;
    }
    
    tooltipOverlay->setKeyTooltipVisibility(label, visibleEnabled);
}
void MpcHardwareMouseListener::syncMpcSliderModelWithUi(juce::Component *eventComponent)
{
    const auto sliderComponent = dynamic_cast<Slider*>(eventComponent);

    if (sliderComponent == nullptr)
    {
        return;
    }

    auto hwSlider = mpc.getHardware()->getSlider();
    const auto yPosFraction = sliderComponent->getSliderYPosFraction();
    hwSlider->setValue(127 - (yPosFraction * 127.f));
}

bool MpcHardwareMouseListener::isPad()
{
    return label.length() >= 4 && label.substr(0, 4) == "pad-";
}


