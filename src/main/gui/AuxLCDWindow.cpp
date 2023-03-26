#include "AuxLCDWindow.h"

#include "LCDControl.h"
#include "Constants.h"

#include <raw_keyboard_input/src/Keyboard.h>

AuxLCDWindow::AuxLCDWindow(LCDControl *lcdControlToUse, Keyboard *keyboardToUse)
: TopLevelWindow("auxlcdwindow", /*addToDesktop*/true), keyboard(keyboardToUse), lcdControl(lcdControlToUse)
{
    setLookAndFeel(&lookAndFeel);
    setVisible(true);
    const int minWidth = 248 + MARGIN;
    const int minHeight = 60 + MARGIN;
    const int defaultWidth = minWidth * 3;
    const int defaultHeight = minHeight * 3;
    const int maxWidth = minWidth * 8;
    const int maxHeight = minHeight * 8;

    auxLcd = new AuxLCD(lcdControlToUse);
    addAndMakeVisible(auxLcd);
    resizableCorner = std::make_unique<juce::ResizableCornerComponent>(this, &constrainer);
    addAndMakeVisible(resizableCorner.get());

    setResizeLimits(minWidth, minHeight, maxWidth, maxHeight);
    setBounds(0, 0, defaultWidth, defaultHeight);

    auxLcd->setCentrePosition(getLocalBounds().getCentre());

    constrainer.setFixedAspectRatio((float)defaultWidth/defaultHeight);

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
    g.fillAll(Constants::LCD_OFF);
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
    auto hRatio = (float) 248 / (248 + MARGIN);
    auto vRatio = (float) 60 / (60 + MARGIN);
    auxLcd->setSize(getWidth() * hRatio, getHeight() * vRatio);

    auxLcd->setCentrePosition(getLocalBounds().getCentre());

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
    setLookAndFeel(nullptr);
    delete auxLcd;
}