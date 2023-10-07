#include "LCDControl.h"

#include <lcdgui/Layer.hpp>
#include <lcdgui/screens/OthersScreen.hpp>
#include "Constants.h"

#include "ContentComponent.h"
#include "AuxLCDWindow.h"

#include "lcdgui/BasicStructs.hpp"

#include <raw_keyboard_input/src/Keyboard.h>

#include "avir.h"

using namespace mpc::lcdgui;
using namespace mpc::lcdgui::screens;

juce::Image applyResize (const juce::Image& src, int width, int height)
{
    juce::Image dst (src.getFormat(), width, height, true);

    juce::Image::BitmapData srcData (src, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData dstData (dst, juce::Image::BitmapData::readWrite);

    int channels = 0;
    if (src.getFormat() == juce::Image::ARGB)               channels = 4;
    else if (src.getFormat() == juce::Image::RGB)           channels = 3;
    else if (src.getFormat() == juce::Image::SingleChannel) channels = 1;
    else                                                    return {};

    // JUCE images may have padding at the end of each scan line.
    // Avir expects the image data to be packed. So we need to
    // pack and unpack the image data before and after resizing.
    juce::HeapBlock<uint8_t> srcPacked (src.getWidth() * src.getHeight() * channels);
    juce::HeapBlock<uint8_t> dstPacked (dst.getWidth() * dst.getHeight() * channels);

    uint8_t* rawSrc = srcPacked.getData();
    uint8_t* rawDst = dstPacked.getData();

    for (int y = 0; y < src.getHeight(); y++)
        memcpy (rawSrc + y * src.getWidth() * channels,
               srcData.getLinePointer (y),
               (size_t) (src.getWidth() * channels));

#if USE_SSE
    avir::CImageResizer<avir::fpclass_float4> imageResizer (8);
    imageResizer.resizeImage (rawSrc, src.getWidth(), src.getHeight(), 0,
                             rawDst, dst.getWidth(), dst.getHeight(), channels, 0);
#else
    avir::CImageResizer<> imageResizer (8);
    imageResizer.resizeImage (rawSrc, src.getWidth(), src.getHeight(), 0,
                             rawDst, dst.getWidth(), dst.getHeight(), channels, 0);
#endif

    for (int y = 0; y < dst.getHeight(); y++)
        memcpy (dstData.getLinePointer (y),
               rawDst + y * dst.getWidth() * channels,
               (size_t) (dst.getWidth() * channels));

    return dst;
}

LCDControl::LCDControl(mpc::Mpc& _mpc)
    : mpc(_mpc), ls(_mpc.getLayeredScreen())
{
    lcd = juce::Image(juce::Image::RGB, 496, 120, true);
    auto othersScreen = mpc.screens->get<OthersScreen>("others");
    othersScreen->addObserver(this);
}

void LCDControl::update(mpc::Observable *, mpc::Message message)
{
    const auto msg = std::get<std::string>(message);

    if (msg == "contrast")
    {
        ls->getFocusedLayer()->SetDirty(); // Could be done less invasively by just redrawing the current pixels of the LCD screens, but with updated colors
        repaint();
    }
}

void LCDControl::drawPixelsToImg()
{
    auto pixels = ls->getPixels();

    auto othersScreen = mpc.screens->get<OthersScreen>("others");
    auto contrast = othersScreen->getContrast();

    juce::Colour c;

    auto halfOn = Constants::LCD_HALF_ON.darker(static_cast<float>(contrast * 0.02));
    auto on = Constants::LCD_ON.darker(static_cast<float>(contrast * 0.02));
    auto off = Constants::LCD_OFF.brighter(static_cast<float>(contrast * 0.01428));

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

            if ((*pixels)[x][y])
            {
                c = halfOn;
                lcd.setPixelAt(x_x2, y_x2, on);
            }
            else
            {
                c = off;
                lcd.setPixelAt(x_x2, y_x2, c);
            }

            lcd.setPixelAt(x_x2 + 1, y_x2, c);
            lcd.setPixelAt(x_x2 + 1, y_x2 + 1, c);
            lcd.setPixelAt(x_x2, y_x2 + 1, c);
        }
    }

    dirtyRect = juce::Rectangle<int>();
}

void LCDControl::resetAuxWindow()
{
    if (auxWindow != nullptr)
    {
        auxWindow->removeFromDesktop();
        delete auxWindow; auxWindow = nullptr;
    }
}

void LCDControl::checkLsDirty()
{
    if (ls->IsDirty())
    {
        auto dirtyArea = ls->getDirtyArea();
        dirtyRect = juce::Rectangle<int>(dirtyArea.L, dirtyArea.T, dirtyArea.W(), dirtyArea.H());
        ls->Draw();
        drawPixelsToImg();
        auto dirtyRect_x2 = juce::Rectangle<int>(dirtyArea.L * 2, dirtyArea.T * 2, dirtyArea.W() * 2, dirtyArea.H() * 2);
        repaint(dirtyRect_x2.expanded(1));

        if (auxWindow != nullptr)
        {
            auto auxBounds = auxWindow->auxLcd->getLocalBounds();

            auto scale = auxBounds.getWidth() / (248.f);

            auto auxRepaintBounds = juce::Rectangle<int>(dirtyArea.L * scale, dirtyArea.T * scale, dirtyArea.W() * scale, dirtyArea.H() * scale);

            auxWindow->auxLcd->repaint(auxRepaintBounds.expanded(3));
        }
    }
}

void LCDControl::timerCallback()
{
    checkLsDirty();
}

void paintRescaledImage(juce::Graphics& g, juce::Rectangle<int> src, juce::Rectangle<int> dest, juce::Image originalImgToDraw)
{
    const auto pxFactor = g.getInternalContext().getPhysicalPixelScaleFactor();
    const int width = juce::roundToInt(pxFactor*dest.getWidth());
    const int height = juce::roundToInt(pxFactor*dest.getHeight());
    auto cutImage = originalImgToDraw.getClippedImage(src);
    auto finalImgToDraw = applyResize(cutImage, width, height);
    juce::Graphics::ScopedSaveState ph(g);
    g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
    g.drawImageTransformed(finalImgToDraw, juce::AffineTransform::scale(1.f/pxFactor).translated(dest.getX(), dest.getY()));
}

void LCDControl::paint(juce::Graphics &g)
{
    paintRescaledImage(g, lcd.getBounds(), getLocalBounds(), lcd);
}

void LCDControl::mouseDoubleClick(const juce::MouseEvent &)
{
    if (auxWindow == nullptr)
    {
        auto contentComponent = dynamic_cast<ContentComponent *>(getParentComponent());
        auxWindow = new AuxLCDWindow(this, contentComponent->keyboard);
        contentComponent->keyboard->setAuxParent(auxWindow);
    }
    else
    {
        auto contentComponent = dynamic_cast<ContentComponent *>(getParentComponent());
        contentComponent->keyboard->setAuxParent(nullptr);
        delete auxWindow;
        auxWindow = nullptr;
    }
}

LCDControl::~LCDControl()
{
    auto othersScreen = mpc.screens->get<OthersScreen>("others");
    othersScreen->deleteObserver(this);

    if (auxWindow != nullptr)
    {
        delete auxWindow;
    }
}
