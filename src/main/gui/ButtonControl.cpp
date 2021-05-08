#include "ButtonControl.hpp"
#include <hardware/Button.hpp>

using namespace juce;
using namespace std;

ButtonControl::ButtonControl(Rectangle<int> _rect, weak_ptr<mpc::hardware::Button> _button)
: rect (_rect), button (_button)
{
}

unordered_map<string, Rectangle<int>*> ButtonControl::rects;
Rectangle<int> ButtonControl::undoseq = Rectangle<int>(212, 652, mw, mh);
Rectangle<int> ButtonControl::erase = Rectangle<int>(291, 651, mw, mh);
Rectangle<int> ButtonControl::rec = Rectangle<int>(208, 808, bw, bh);
Rectangle<int> ButtonControl::overdub = Rectangle<int>(288, 806, bw, bh);
Rectangle<int> ButtonControl::stop = Rectangle<int>(367, 804, bw, bh);
Rectangle<int> ButtonControl::play = Rectangle<int>(446, 802, bw, bh);
Rectangle<int> ButtonControl::playstart = Rectangle<int>(525, 800, bw, bh);
Rectangle<int> ButtonControl::mainscreen = Rectangle<int>(382, 307, mw, mh);
Rectangle<int> ButtonControl::openwindow = Rectangle<int>(510, 308, mw, mh);
Rectangle<int> ButtonControl::taptemponoterepeat = Rectangle<int>(247, 556, bw, bh);
Rectangle<int> ButtonControl::prevstepevent = Rectangle<int>(212, 723, mw, mh);
Rectangle<int> ButtonControl::nextstepevent = Rectangle<int>(292, 721, mw, mh);
Rectangle<int> ButtonControl::gotoRect = Rectangle<int>(372, 719, mw, mh);
Rectangle<int> ButtonControl::prevbarstart = Rectangle<int>(450, 716, mw, mh);
Rectangle<int> ButtonControl::nextbarend = Rectangle<int>(528, 718, mw, mh);
Rectangle<int> ButtonControl::f1 = Rectangle<int>(144, 262, smw, smh);
Rectangle<int> ButtonControl::f2 = Rectangle<int>(225, 261, smw, smh);
Rectangle<int> ButtonControl::f3 = Rectangle<int>(310, 263, smw, smh);
Rectangle<int> ButtonControl::f4 = Rectangle<int>(394, 263, smw, smh);
Rectangle<int> ButtonControl::f5 = Rectangle<int>(474, 261, smw, smh);
Rectangle<int> ButtonControl::f6 = Rectangle<int>(554, 262, smw, smh);
Rectangle<int> ButtonControl::notevariationafter = Rectangle<int>(88, 568, mw, mh);
Rectangle<int> ButtonControl::rect0 = Rectangle<int>(170, 460, smw, smh);
Rectangle<int> ButtonControl::rect1 = Rectangle<int>(91, 413, smw, smh);
Rectangle<int> ButtonControl::rect2 = Rectangle<int>(174, 414, smw, smh);
Rectangle<int> ButtonControl::rect3 = Rectangle<int>(253, 415, smw, smh);
Rectangle<int> ButtonControl::rect4 = Rectangle<int>(91, 365, smw, smh);
Rectangle<int> ButtonControl::rect5 = Rectangle<int>(173, 364, smw, smh);
Rectangle<int> ButtonControl::rect6 = Rectangle<int>(253, 365, smw, smh);
Rectangle<int> ButtonControl::rect7 = Rectangle<int>(88, 315, smw, smh);
Rectangle<int> ButtonControl::rect8 = Rectangle<int>(170, 315, smw, smh);
Rectangle<int> ButtonControl::rect9 = Rectangle<int>(251, 315, smw, smh);
Rectangle<int> ButtonControl::shift = Rectangle<int>(90, 462, smw, smh);
Rectangle<int> ButtonControl::enter = Rectangle<int>(253, 460, smw, smh);
Rectangle<int> ButtonControl::banka = Rectangle<int>(942, 259, mw, mh);
Rectangle<int> ButtonControl::bankb = Rectangle<int>(1026, 260, mw, mh);
Rectangle<int> ButtonControl::bankc = Rectangle<int>(1110, 257, mw, mh);
Rectangle<int> ButtonControl::bankd = Rectangle<int>(1192, 258, mw, mh);
Rectangle<int> ButtonControl::fulllevel = Rectangle<int>(777, 178, mw, mh);
Rectangle<int> ButtonControl::sixteenlevels = Rectangle<int>(861, 180, mw, mh);
Rectangle<int> ButtonControl::nextseq = Rectangle<int>(778, 263, mw, mh);
Rectangle<int> ButtonControl::trackmute = Rectangle<int>(860, 261, mw, mh);
Rectangle<int> ButtonControl::left = Rectangle<int>(416, 581, 29, 76);
Rectangle<int> ButtonControl::up = Rectangle<int>(445, 581, 46, 38);
Rectangle<int> ButtonControl::down = Rectangle<int>(445, 619, 46, 38);
Rectangle<int> ButtonControl::right = Rectangle<int>(491, 581, 29, 76);

void ButtonControl::initRects()
{
    if (rects.size() != 0)
        return;
    
    rects.emplace("rec", &rec);
    rects.emplace("overdub", &overdub);
    rects.emplace("stop", &stop);
    rects.emplace("play", &play);
    rects.emplace("play-start", &playstart);
    rects.emplace("main-screen", &mainscreen);
    rects.emplace("prev-step-event", &prevstepevent);
    rects.emplace("next-step-event", &nextstepevent);
    rects.emplace("go-to", &gotoRect);
    rects.emplace("prev-bar-start", &prevbarstart);
    rects.emplace("next-bar-end", &nextbarend);
    rects.emplace("tap", &taptemponoterepeat);
    rects.emplace("next-seq", &nextseq);
    rects.emplace("track-mute", &trackmute);
    rects.emplace("open-window", &openwindow);
    rects.emplace("full-level", &fulllevel);
    rects.emplace("sixteen-levels", &sixteenlevels);
    rects.emplace("f1", &f1);
    rects.emplace("f2", &f2);
    rects.emplace("f3", &f3);
    rects.emplace("f4", &f4);
    rects.emplace("f5", &f5);
    rects.emplace("f6", &f6);
    rects.emplace("shift", &shift);
    rects.emplace("enter", &enter);
    rects.emplace("undo-seq", &undoseq);
    rects.emplace("erase", &erase);
    rects.emplace("after", &notevariationafter);
    rects.emplace("bank-a", &banka);
    rects.emplace("bank-b", &bankb);
    rects.emplace("bank-c", &bankc);
    rects.emplace("bank-d", &bankd);
    rects.emplace("left", &left);
    rects.emplace("right", &right);
    rects.emplace("up", &up);
    rects.emplace("down", &down);
    rects.emplace("0", &rect0);
    rects.emplace("1", &rect1);
    rects.emplace("2", &rect2);
    rects.emplace("3", &rect3);
    rects.emplace("4", &rect4);
    rects.emplace("5", &rect5);
    rects.emplace("6", &rect6);
    rects.emplace("7", &rect7);
    rects.emplace("8", &rect8);
    rects.emplace("9", &rect9);
    
    for (auto r : rects)
        r.second->setY(r.second->getY() + 55);
}

void ButtonControl::setBounds()
{
    setSize(rect.getWidth(), rect.getHeight());
    Component::setBounds(rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
}

void ButtonControl::mouseDown(const MouseEvent&)
{
    button.lock()->push();
}

void ButtonControl::mouseUp(const MouseEvent&)
{
    button.lock()->release();
}
