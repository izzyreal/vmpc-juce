#include "gui/vector/Lcd.hpp"

#include "gui/vector/Constants.hpp"
#include "gui/vector/View.hpp"
#include "gui/AuxLcdWindow.hpp"
#include "gui/focus/FocusHelper.hpp"

#include <Mpc.hpp>
#include <lcdgui/Screens.hpp>
#include <lcdgui/Layer.hpp>
#include <lcdgui/screens/OthersScreen.hpp>

using namespace vmpc_juce::gui::vector;
using mpc::lcdgui::LCD_HEIGHT;
using mpc::lcdgui::LCD_WIDTH;

Lcd::Lcd(mpc::Mpc &mpcToUse) : mpc(mpcToUse)
{
    shadow.setColor(Constants::lcdOffBacklit.brighter().withAlpha(0.4f));
    resetAuxWindowF = [&]
    {
        resetAuxWindow();
    };

    resetKeyboardAuxParent = [&]
    {
        getView()->getFocusHelper()->setAuxComponent(nullptr);
    };

    getLcdImage = [&]() -> juce::Image &
    {
        return img;
    };
    dirtyRect = juce::Rectangle<int>(0, 0, LCD_WIDTH, LCD_HEIGHT);
    drawPixelsToImg();

    const auto othersScreen =
        mpc.screens->get<mpc::lcdgui::ScreenId::OthersScreen>();
    othersScreen->addObserver(this);

    setIntervalMs(30);
}

Lcd::~Lcd()
{
    const auto othersScreen =
        mpc.screens->get<mpc::lcdgui::ScreenId::OthersScreen>();
    othersScreen->deleteObserver(this);
    delete auxWindow;
}

void Lcd::update(mpc::Observable *, const mpc::Message message)
{
    const auto msg = std::get<std::string>(message);

    if (msg == "contrast")
    {
        mpc.getLayeredScreen()
            ->getFocusedLayer()
            ->SetDirty(); // Could be done less invasively by just
                          // redrawing the current pixels of the LCD
                          // screens, but with updated colors
        repaint();
    }
}

void Lcd::paint(juce::Graphics &g)
{
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);

    const auto layeredScreen = mpc.getLayeredScreen();

    const auto t = getMyTransform();

    g.drawImageTransformed(img, t);

    shadow.setRadius(std::round(static_cast<float>(getWidth()) / LCD_WIDTH));

    juce::Path p;

    const auto &rawPixels = *layeredScreen->getPixels();

    for (uint8_t y = 0; y < LCD_HEIGHT; y++)
    {
        for (uint8_t x = 0; x < LCD_WIDTH; x++)
        {
            const bool on = rawPixels[x][y];
            if (!on)
            {
                p.addRectangle(x * 2, y * 2, 2, 2);
            }
        }
    }

    p.applyTransform(t);
    shadow.render(g, p);
}

void Lcd::checkLsDirty()
{
    const auto layeredScreen = mpc.getLayeredScreen();

    if (!layeredScreen->IsDirty())
    {
        return;
    }

    const auto dirtyArea = layeredScreen->getDirtyArea();
    dirtyRect =
        juce::Rectangle(dirtyArea.L, dirtyArea.T, dirtyArea.W(), dirtyArea.H());
    const auto dirtyRectForAuxLcd = dirtyRect;
    const auto dirtyRectT =
        dirtyRect.toFloat()
            .transformedBy(juce::AffineTransform().scaled(2.f))
            .transformedBy(getMyTransform());

    layeredScreen->Draw();
    drawPixelsToImg();

    repaint(dirtyRectT.toNearestInt().expanded(1));

    if (auxWindow != nullptr)
    {
        auxWindow->repaintAuxLcdLocalBounds(dirtyRectForAuxLcd);
    }
}

void Lcd::sharedTimerCallback()
{
    mpc.getLayeredScreen()->timerCallback();
    checkLsDirty();
}

void Lcd::drawPixelsToImg()
{
    const auto layeredScreen = mpc.getLayeredScreen();

    const auto pixels = layeredScreen->getPixels();

    const auto othersScreen =
        mpc.screens->get<mpc::lcdgui::ScreenId::OthersScreen>();
    const auto contrast = othersScreen->getContrast();

    juce::Colour c;

    const auto halfOn =
        Constants::lcdOnLight.darker(static_cast<float>(contrast * 0.02));
    const auto on =
        Constants::lcdOn.darker(static_cast<float>(contrast * 0.02));
    const auto off =
        Constants::lcdOff.brighter(static_cast<float>(contrast * 0.01428));

    const auto rectX = dirtyRect.getX();
    const auto rectY = dirtyRect.getY();
    const auto rectRight = dirtyRect.getRight();
    const auto rectBottom = dirtyRect.getBottom();

    for (int x = rectX; x < rectRight; x++)
    {
        for (int y = rectY; y < rectBottom; y++)
        {
            const auto x_x2 = x * 2;
            const auto y_x2 = y * 2;

            if ((*pixels)[static_cast<size_t>(x)][static_cast<size_t>(y)])
            {
                c = halfOn;
                img.setPixelAt(x_x2, y_x2, on);
            }
            else
            {
                c = off;
                img.setPixelAt(x_x2, y_x2, c);
            }

            img.setPixelAt(x_x2 + 1, y_x2, c);
            img.setPixelAt(x_x2 + 1, y_x2 + 1, c);
            img.setPixelAt(x_x2, y_x2 + 1, c);
        }
    }

    dirtyRect = juce::Rectangle<int>();
}

void Lcd::mouseDoubleClick(const juce::MouseEvent &)
{
    const auto view = getView();

    if (auxWindow == nullptr)
    {
        auxWindow = new AuxLcdWindow(mpc, resetAuxWindowF, getLcdImage,
                                     resetKeyboardAuxParent, Constants::lcdOff);
        auxWindow->setVisible(true);
        view->getFocusHelper()->setAuxComponent(auxWindow);
    }
    else
    {
        view->getFocusHelper()->setAuxComponent(nullptr);
        delete auxWindow;
        auxWindow = nullptr;
    }
}

void Lcd::mouseDown(const juce::MouseEvent &e)
{
    if (vmpc_juce::gui::dispatchLcdPointerEvent(
            mpc, e, getRenderedLcdBounds(),
            ::mpc::input::GestureEvent::Type::BEGIN) ==
        ::mpc::input::HostInputResult::Ignored)
    {
        getParentComponent()->mouseDown(e);
    }
}

void Lcd::mouseUp(const juce::MouseEvent &e)
{
    vmpc_juce::gui::dispatchLcdPointerEvent(
        mpc, e, getRenderedLcdBounds(), ::mpc::input::GestureEvent::Type::END);
}

void Lcd::mouseDrag(const juce::MouseEvent &e)
{
    if (vmpc_juce::gui::dispatchLcdPointerEvent(
            mpc, e, getRenderedLcdBounds(),
            ::mpc::input::GestureEvent::Type::UPDATE) ==
        ::mpc::input::HostInputResult::Ignored)
    {
        getParentComponent()->mouseDrag(e);
    }
}

void Lcd::mouseWheelMove(const juce::MouseEvent &e,
                         const juce::MouseWheelDetails &wheel)
{
    if (vmpc_juce::gui::dispatchLcdWheelEvent(mpc, e, wheel,
                                              getRenderedLcdBounds()) ==
        ::mpc::input::HostInputResult::Ignored)
    {
        juce::Component::mouseWheelMove(e, wheel);
    }
}

juce::AffineTransform Lcd::getMyTransform() const
{
    constexpr auto asp_ratio =
        static_cast<float>(LCD_HEIGHT) / static_cast<float>(LCD_WIDTH);
    const auto w = static_cast<float>(getWidth()) * magicMultiplier;
    const auto h = w * asp_ratio;
    const auto img_scale = w / static_cast<float>(LCD_WIDTH * 2);
    const auto unused_h_px = static_cast<float>(getWidth()) - w;
    const auto unused_v_px = static_cast<float>(getHeight()) - h;
    const auto x_offset = unused_h_px * 0.5f;
    const auto y_offset = unused_v_px * 0.5f;

    juce::AffineTransform t;
    t = t.scaled(img_scale);
    t = t.translated(x_offset, y_offset);
    return t;
}

juce::Rectangle<float> Lcd::getRenderedLcdBounds() const
{
    return juce::Rectangle<float>(0.f, 0.f, static_cast<float>(img.getWidth()),
                                  static_cast<float>(img.getHeight()))
        .transformedBy(getMyTransform());
}

void Lcd::resetAuxWindow()
{
    if (auxWindow != nullptr)
    {
        auxWindow->removeFromDesktop();
        delete auxWindow;
        auxWindow = nullptr;
    }
}

View *Lcd::getView() const
{
    Component *ancestor = getParentComponent();

    while (dynamic_cast<View *>(ancestor) == nullptr)
    {
        ancestor = ancestor->getParentComponent();
    }

    return dynamic_cast<View *>(ancestor);
}
