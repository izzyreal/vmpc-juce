#include "AuxLCDWindow.hpp"

void AuxLCDWindowMaximizeButton::paint(juce::Graphics& g)
{
    const int rows = 5;
    const int xOffset = (getWidth() / VmpcAuxLcdLookAndFeel::LCD_PIXEL_SIZE) - (rows + 1);

    for (int i = 0; i < rows; i++)
    {
        VmpcAuxLcdLookAndFeel::drawLcdPixel(g, xOffset + 2, i + 1);

        if (i == 2)
        {
            continue;
        }

        VmpcAuxLcdLookAndFeel::drawLcdPixel(g, i + xOffset, 3);
    }
}

void AuxLCDWindowMaximizeButton::mouseDown(const juce::MouseEvent& e)
{
    dynamic_cast<AuxLCDWindow*>(getParentComponent())->showButtons();
}

void AuxLCDWindowMaximizeButton::mouseEnter(const juce::MouseEvent &e)
{
    dynamic_cast<AuxLCDWindow*>(getParentComponent())->showButtons();
}

void AuxLCDWindowMaximizeButton::paintButton(juce::Graphics&, bool /*shouldDrawButtonAsHighlighted*/,
                                             bool /*shouldDrawButtonAsDown*/)
{

}

AuxLCDWindow::AuxLCDWindow(const std::function<void()> &resetAuxWindowToUse, const std::function<juce::Image&()> &getLcdImageToUse, const std::function<void()> &resetKeyboardAuxParentToUse, const juce::Colour backgroundColourToUse)
: TopLevelWindow("auxlcdwindow", /*addToDesktop*/true), resetKeyboardAuxParent(resetKeyboardAuxParentToUse), resetAuxWindow(resetAuxWindowToUse), backgroundColour(backgroundColourToUse)
{
    setLookAndFeel(&lookAndFeel);
    setVisible(true);
    const int minWidth = LCD_W + MARGIN;
    const int minHeight = LCD_H + MARGIN;
    const int defaultWidth = minWidth * 3;
    const int defaultHeight = minHeight * 3;
    const int maxWidth = minWidth * 16;
    const int maxHeight = minHeight * 16;

    auxLcd = new AuxLCD(getLcdImageToUse);
    addAndMakeVisible(auxLcd);
    resizableCorner = std::make_unique<MyResizableCornerComponent>(this, &constrainer);
    addAndMakeVisible(resizableCorner.get());

    setResizeLimits(minWidth, minHeight, maxWidth, maxHeight);
    setBounds(0, 0, defaultWidth, defaultHeight);

    auxLcd->setCentrePosition(getLocalBounds().getCentre());

    constrainer.setFixedAspectRatio((float)defaultWidth/defaultHeight);

    setAlwaysOnTop(true);
    setWantsKeyboardFocus(true);

    resizableCorner->setAlwaysOnTop(true);
    
    addAndMakeVisible(&maximizeButton);
    maximizeButton.onClick = [this]{
        auto screen = juce::Desktop::getInstance().getDisplays().getDisplayForPoint(maximizeButton.getBounds().getCentre());
        
        if (screen != nullptr)
        {
            const auto ratio = (float) getWidth() / getHeight();
            setTopLeftPosition(0, 0);
            setSize(screen->userArea.getWidth(), screen->userArea.getWidth() / ratio);
        }
    };
    startTimer(100);
}

void AuxLCDWindow::showButtons()
{
    if (areButtonsShowing())
    {
        resetButtonShowTimer();
        return;
    }

    resetButtonShowTimer();
    maximizeButton.setAlpha(1);
    maximizeButton.repaint();
    resizableCorner->setAlpha(1);
}

void AuxLCDWindow::hideButtons()
{
    buttonsHaveBeenShownForMs = -1;
    maximizeButton.setAlpha(0);
    maximizeButton.repaint();
    resizableCorner->setAlpha(0);
}

bool AuxLCDWindow::areButtonsShowing()
{
    return buttonsHaveBeenShownForMs >= 0;
}

void AuxLCDWindow::resetButtonShowTimer()
{
    buttonsHaveBeenShownForMs = 0;
}

void AuxLCDWindow::timerCallback()
{
    if (isMouseButtonDown() || resizableCorner->isMouseButtonDown())
    {
        return;
    }

    if (!areButtonsShowing())
    {
        return;
    }
    
    if (buttonsHaveBeenShownForMs > 2000)
    {
        hideButtons();
        return;
    }

    buttonsHaveBeenShownForMs += getTimerInterval();
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
    g.fillAll(backgroundColour);
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

void AuxLCDWindow::setBoundsConstrained(const juce::Rectangle<int>& newBounds)
{
    constrainer.setBoundsForComponent (this, newBounds, false, false, false, false);
}

void AuxLCDWindow::resized()
{
    auto hRatio = (float) LCD_W / (LCD_W + MARGIN);
    auto vRatio = (float) LCD_H / (LCD_H + MARGIN);
    auxLcd->setSize(getWidth() * hRatio, getHeight() * vRatio);

    auxLcd->setCentrePosition(getLocalBounds().getCentre());

    const int widgetSize = 40;
    resizableCorner->setBounds (getWidth() - widgetSize,
                                getHeight() - widgetSize + 2,
                                widgetSize, widgetSize);

    maximizeButton.setBounds(getWidth() - widgetSize, 0, widgetSize, widgetSize);
}

void AuxLCDWindow::mouseMove(const juce::MouseEvent&)
{
    showButtons();
}

void AuxLCDWindow::mouseEnter(const juce::MouseEvent &e)
{
    showButtons();
}

void AuxLCDWindow::mouseDown(const juce::MouseEvent& e)
{
    dragStarted = true;
    dragger.startDraggingComponent(this, e);
    
    showButtons();
}

void AuxLCDWindow::mouseUp(const juce::MouseEvent&)
{
    dragStarted = false;
    showButtons();
}

void AuxLCDWindow::mouseDrag(const juce::MouseEvent& e)
{
    if (dragStarted)
    {
        dragger.dragComponent (this, e, &constrainer);
    }

    showButtons();
}

void AuxLCDWindow::mouseDoubleClick(const juce::MouseEvent &)
{
    resetKeyboardAuxParent();
    resetAuxWindow();
}

AuxLCDWindow::~AuxLCDWindow()
{
    setLookAndFeel(nullptr);
    delete auxLcd;
}

void AuxLCDWindow::repaintAuxLcdLocalBounds(juce::Rectangle<int> dirtyArea)
{
    auto auxBounds = auxLcd->getLocalBounds();

    auto scale = auxBounds.getWidth() / (248.f);

    auto auxRepaintBounds = juce::Rectangle<int>(dirtyArea.getX() * scale, dirtyArea.getY() * scale, dirtyArea.getWidth() * scale, dirtyArea.getHeight() * scale);

    auxLcd->repaint(auxRepaintBounds.expanded(3));
}
