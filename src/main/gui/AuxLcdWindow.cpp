#include "AuxLcdWindow.hpp"

using namespace vmpc_juce::gui;

void AuxLcdWindowMaximizeButton::paint(juce::Graphics &g)
{
    constexpr int rows = 5;
    const int xOffset =
        getWidth() / VmpcAuxLcdLookAndFeel::LCD_PIXEL_SIZE - (rows + 1);

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

void AuxLcdWindowMaximizeButton::mouseDown(const juce::MouseEvent &)
{
    dynamic_cast<AuxLcdWindow *>(getParentComponent())->showButtons();
}

void AuxLcdWindowMaximizeButton::mouseEnter(const juce::MouseEvent &)
{
    dynamic_cast<AuxLcdWindow *>(getParentComponent())->showButtons();
}

void AuxLcdWindowMaximizeButton::paintButton(
    juce::Graphics &, bool /*shouldDrawButtonAsHighlighted*/,
    bool /*shouldDrawButtonAsDown*/)
{
}

AuxLcdWindow::AuxLcdWindow(
    const std::function<void()> &resetAuxWindowToUse,
    const std::function<juce::Image &()> &getLcdImage,
    const std::function<void()> &resetKeyboardAuxParentToUse,
    const juce::Colour backgroundColourToUse)
    : TopLevelWindow("auxlcdwindow", /*addToDesktop*/ true),
      resetKeyboardAuxParent(resetKeyboardAuxParentToUse),
      resetAuxWindow(resetAuxWindowToUse),
      backgroundColour(backgroundColourToUse)
{
    setLookAndFeel(&lookAndFeel);
    constexpr int minWidth = LCD_W + MARGIN;
    constexpr int minHeight = LCD_H + MARGIN;
    constexpr int defaultWidth = minWidth * 3;
    constexpr int defaultHeight = minHeight * 3;
    constexpr int maxWidth = minWidth * 16;
    constexpr int maxHeight = minHeight * 16;

    auxLcd = new AuxLcd(getLcdImage);
    addAndMakeVisible(auxLcd);
    resizableCorner =
        std::make_unique<MyResizableCornerComponent>(this, &constrainer);
    addAndMakeVisible(resizableCorner.get());

    constrainer.setSizeLimits(minWidth, minHeight, maxWidth, maxHeight);

    setBounds(0, 0, defaultWidth, defaultHeight);

    auxLcd->setCentrePosition(getLocalBounds().getCentre());

    constrainer.setFixedAspectRatio(static_cast<float>(defaultWidth) /
                                    defaultHeight);

    setAlwaysOnTop(true);
    setWantsKeyboardFocus(true);

    resizableCorner->setAlwaysOnTop(true);

    addAndMakeVisible(&maximizeButton);
    maximizeButton.onClick = [this]
    {
        const auto screen =
            juce::Desktop::getInstance().getDisplays().getDisplayForPoint(
                maximizeButton.getBounds().getCentre());

        if (screen != nullptr)
        {
            const auto aspectRatio = getBounds().toFloat().getAspectRatio();
            setTopLeftPosition(0, 0);
            setSize(screen->userArea.getWidth(),
                    static_cast<int>(screen->userArea.toFloat().getWidth() /
                                     aspectRatio));
        }
    };
    startTimer(100);
}

void AuxLcdWindow::showButtons()
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

void AuxLcdWindow::hideButtons()
{
    buttonsHaveBeenShownForMs = -1;
    maximizeButton.setAlpha(0);
    maximizeButton.repaint();
    resizableCorner->setAlpha(0);
}

bool AuxLcdWindow::areButtonsShowing() const
{
    return buttonsHaveBeenShownForMs >= 0;
}

void AuxLcdWindow::resetButtonShowTimer()
{
    buttonsHaveBeenShownForMs = 0;
}

void AuxLcdWindow::timerCallback()
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

bool AuxLcdWindow::keyPressed(const juce::KeyPress &k)
{
    const auto desc = k.getTextDescription().toStdString();

    if (desc == "command + Q" || desc == "alt + F4")
    {
        return false;
    }

    return true;
}

void AuxLcdWindow::paint(juce::Graphics &g)
{
    g.fillAll(backgroundColour);
}

void AuxLcdWindow::resized()
{
    constexpr auto hRatio = static_cast<float>(LCD_W) / (LCD_W + MARGIN);
    constexpr auto vRatio = static_cast<float>(LCD_H) / (LCD_H + MARGIN);

    const auto bounds = getBounds().toFloat();

    auxLcd->setSize(static_cast<int>(bounds.getWidth() * hRatio),
                    static_cast<int>(bounds.getHeight() * vRatio));

    auxLcd->setCentrePosition(getLocalBounds().getCentre());

    constexpr int widgetSize = 40;
    resizableCorner->setBounds(getWidth() - widgetSize,
                               getHeight() - widgetSize + 2, widgetSize,
                               widgetSize);

    maximizeButton.setBounds(getWidth() - widgetSize, 0, widgetSize,
                             widgetSize);
}

void AuxLcdWindow::mouseMove(const juce::MouseEvent &)
{
    showButtons();
}

void AuxLcdWindow::mouseEnter(const juce::MouseEvent &)
{
    showButtons();
}

void AuxLcdWindow::mouseDown(const juce::MouseEvent &e)
{
    dragStarted = true;
    dragger.startDraggingComponent(this, e);

    showButtons();
}

void AuxLcdWindow::mouseUp(const juce::MouseEvent &)
{
    dragStarted = false;
    showButtons();
}

void AuxLcdWindow::mouseDrag(const juce::MouseEvent &e)
{
    if (dragStarted)
    {
        dragger.dragComponent(this, e, &constrainer);
    }

    showButtons();
}

void AuxLcdWindow::mouseDoubleClick(const juce::MouseEvent &)
{
    resetKeyboardAuxParent();
    resetAuxWindow();
}

AuxLcdWindow::~AuxLcdWindow()
{
    setLookAndFeel(nullptr);
    delete auxLcd;
}

void AuxLcdWindow::repaintAuxLcdLocalBounds(
    const juce::Rectangle<int> dirtyArea) const
{
    const auto auxBounds = auxLcd->getLocalBounds().toFloat();

    const auto scale = auxBounds.getWidth() / LCD_W;

    const auto dirtyAreaF = dirtyArea.toFloat();

    const auto auxRepaintBounds = juce::Rectangle(
        dirtyAreaF.getX() * scale, dirtyAreaF.getY() * scale,
        dirtyAreaF.getWidth() * scale, dirtyAreaF.getHeight() * scale);

    auxLcd->repaint(auxRepaintBounds.expanded(3).toNearestInt());
}
