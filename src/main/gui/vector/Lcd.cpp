#include "gui/vector/Lcd.hpp"

#include "gui/vector/Constants.hpp"
#include "gui/vector/View.hpp"
#include "gui/AuxLCDWindow.hpp"
#include "gui/focus/FocusHelper.hpp"

#include <Mpc.hpp>
#include <lcdgui/Screens.hpp>
#include <lcdgui/Layer.hpp>
#include <lcdgui/screens/OthersScreen.hpp>

using namespace vmpc_juce::gui::vector;

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
    drawPixelsToImg();

    const auto othersScreen = mpc.screens->get<mpc::lcdgui::ScreenId::OthersScreen>();
    othersScreen->addObserver(this);

    setIntervalMs(30);
}

Lcd::~Lcd()
{
    const auto othersScreen = mpc.screens->get<mpc::lcdgui::ScreenId::OthersScreen>();
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

    shadow.setRadius(std::round(static_cast<float>(getWidth()) / 248.f));

    juce::Path p;

    const auto &rawPixels = *layeredScreen->getPixels();

    for (uint8_t y = 0; y < 60; y++)
    {
        for (uint8_t x = 0; x < 248; x++)
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

    const auto othersScreen = mpc.screens->get<mpc::lcdgui::ScreenId::OthersScreen>();
    const auto contrast = othersScreen->getContrast();

    juce::Colour c;

    const auto halfOn =
        Constants::lcdOnLight.darker(static_cast<float>(contrast * 0.02));
    const auto on = Constants::lcdOn.darker(static_cast<float>(contrast * 0.02));
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
        auxWindow = new AuxLCDWindow(resetAuxWindowF, getLcdImage,
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
    getParentComponent()->mouseDown(e);
}

void Lcd::mouseDrag(const juce::MouseEvent &e)
{
    getParentComponent()->mouseDrag(e);
}

juce::AffineTransform Lcd::getMyTransform() const
{
    constexpr auto asp_ratio = 60.f / 248.f;
    const auto w = static_cast<float>(getWidth()) * magicMultiplier;
    const auto h = w * asp_ratio;
    const auto img_scale = w / (248 * 2);
    const auto unused_h_px = static_cast<float>(getWidth()) - w;
    const auto unused_v_px = static_cast<float>(getHeight()) - h;
    const auto x_offset = unused_h_px * 0.5f;
    const auto y_offset = unused_v_px * 0.5f;

    juce::AffineTransform t;
    t = t.scaled(img_scale);
    t = t.translated(x_offset, y_offset);
    return t;
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
