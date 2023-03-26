#include "AuxLCDWindow.h"

#include "LCDControl.h"

#include <raw_keyboard_input/src/Keyboard.h>

AuxLCDWindow::AuxLCDWindow(LCDControl *lcdControlToUse, Keyboard *keyboardToUse)
: TopLevelWindow("auxlcdwindow", /*addToDesktop*/true), keyboard(keyboardToUse), lcdControl(lcdControlToUse)
{
    setVisible(true);
    const int margin = 0;
    const int minWidth = 248 + (margin * 2);
    const int minHeight = 60 + (margin * 2);
    const int maxWidth = (248 * 8) + (margin * 2);
    const int maxHeight = (60 * 8) + (margin * 2);

    auxLcd = new AuxLCD(lcdControlToUse);
    addAndMakeVisible(auxLcd);

    resizableCorner = std::make_unique<juce::ResizableCornerComponent>(this, &constrainer);
    addAndMakeVisible(resizableCorner.get());

    setResizeLimits(minWidth, minHeight, maxWidth, maxHeight);
    constrainer.setFixedAspectRatio((float) minWidth / minHeight);
    setBounds(0, 0, (248 * 3) + (margin * 2), (60 * 3) + (margin * 2));

    setAlwaysOnTop(true);
    setWantsKeyboardFocus(true);

    resizableCorner->setAlwaysOnTop(true);
}

bool AuxLCDWindow::keyPressed(const juce::KeyPress &k)
{
    auto desc = k.getTextDescription().toStdString();

    if (desc == "command + Q" || desc == "alt + F4")
    {
        return false;
    }

    return true;
}

void AuxLCDWindow::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colours::aliceblue);
}

void AuxLCDWindow::setResizeLimits (int newMinimumWidth,
                                       int newMinimumHeight,
                                       int newMaximumWidth,
                                       int newMaximumHeight) noexcept
{
    constrainer.setSizeLimits (newMinimumWidth, newMinimumHeight,
                                      newMaximumWidth, newMaximumHeight);

    setBoundsConstrained (getBounds());
}

void AuxLCDWindow::setBoundsConstrained (const juce::Rectangle<int>& newBounds)
{
    constrainer.setBoundsForComponent (this, newBounds, false, false, false, false);
}

void AuxLCDWindow::resized()
{
    auxLcd->setSize(getWidth(), getHeight());
    const int resizerSize = 18;
    resizableCorner->setBounds (getWidth() - resizerSize,
                                getHeight() - resizerSize,
                                resizerSize, resizerSize);
}

void AuxLCDWindow::mouseDown (const juce::MouseEvent& e)
{
    dragStarted = true;
    dragger.startDraggingComponent(this, e);
}

void AuxLCDWindow::mouseDrag (const juce::MouseEvent& e)
{
    if (dragStarted)
        dragger.dragComponent (this, e, &constrainer);
}

void AuxLCDWindow::mouseDoubleClick(const juce::MouseEvent &)
{
    keyboard->setAuxParent(nullptr);
    lcdControl->resetAuxWindow();
}

AuxLCDWindow::~AuxLCDWindow()
{
    delete auxLcd;
}