#include "ButtonControl.hpp"
#include "hardware/Button.hpp"

ButtonControl::ButtonControl(mpc::Mpc& mpc, juce::Rectangle<int> _rect,
                             std::weak_ptr<mpc::hardware::Button> _button)
: VmpcTooltipComponent(mpc, _button.lock()), rect(_rect), button(std::move(_button))
{
}

std::unordered_map<std::string, juce::Rectangle<int>*> ButtonControl::rects;

juce::Rectangle<int> ButtonControl::undoseq = juce::Rectangle<int>(212, 652, mw, mh);
juce::Rectangle<int> ButtonControl::erase = juce::Rectangle<int>(291, 651, mw, mh);
juce::Rectangle<int> ButtonControl::rec = juce::Rectangle<int>(208, 808, bw, bh);
juce::Rectangle<int> ButtonControl::overdub = juce::Rectangle<int>(288, 806, bw, bh);
juce::Rectangle<int> ButtonControl::stop = juce::Rectangle<int>(367, 804, bw, bh);
juce::Rectangle<int> ButtonControl::play = juce::Rectangle<int>(446, 802, bw, bh);
juce::Rectangle<int> ButtonControl::playstart = juce::Rectangle<int>(525, 800, bw, bh);
juce::Rectangle<int> ButtonControl::mainscreen = juce::Rectangle<int>(382, 307, mw, mh);
juce::Rectangle<int> ButtonControl::openwindow = juce::Rectangle<int>(510, 308, mw, mh);
juce::Rectangle<int> ButtonControl::taptemponoterepeat = juce::Rectangle<int>(247, 556, bw, bh);
juce::Rectangle<int> ButtonControl::prevstepevent = juce::Rectangle<int>(212, 723, mw, mh);
juce::Rectangle<int> ButtonControl::nextstepevent = juce::Rectangle<int>(292, 721, mw, mh);
juce::Rectangle<int> ButtonControl::gotoRect = juce::Rectangle<int>(372, 719, mw, mh);
juce::Rectangle<int> ButtonControl::prevbarstart = juce::Rectangle<int>(450, 716, mw, mh);
juce::Rectangle<int> ButtonControl::nextbarend = juce::Rectangle<int>(528, 718, mw, mh);
juce::Rectangle<int> ButtonControl::f1 = juce::Rectangle<int>(144, 262, smw, smh);
juce::Rectangle<int> ButtonControl::f2 = juce::Rectangle<int>(225, 261, smw, smh);
juce::Rectangle<int> ButtonControl::f3 = juce::Rectangle<int>(310, 263, smw, smh);
juce::Rectangle<int> ButtonControl::f4 = juce::Rectangle<int>(394, 263, smw, smh);
juce::Rectangle<int> ButtonControl::f5 = juce::Rectangle<int>(474, 261, smw, smh);
juce::Rectangle<int> ButtonControl::f6 = juce::Rectangle<int>(554, 262, smw, smh);
juce::Rectangle<int> ButtonControl::notevariationafter = juce::Rectangle<int>(88, 568, mw, mh);
juce::Rectangle<int> ButtonControl::rect0 = juce::Rectangle<int>(170, 460, smw, smh);
juce::Rectangle<int> ButtonControl::rect1 = juce::Rectangle<int>(91, 413 - 5, smw, smh);
juce::Rectangle<int> ButtonControl::rect2 = juce::Rectangle<int>(174, 414 - 5, smw, smh);
juce::Rectangle<int> ButtonControl::rect3 = juce::Rectangle<int>(253, 415 - 5, smw, smh);
juce::Rectangle<int> ButtonControl::rect4 = juce::Rectangle<int>(91, 365 - 10, smw, smh);
juce::Rectangle<int> ButtonControl::rect5 = juce::Rectangle<int>(173, 364 - 10, smw, smh);
juce::Rectangle<int> ButtonControl::rect6 = juce::Rectangle<int>(253, 365 - 10, smw, smh);
juce::Rectangle<int> ButtonControl::rect7 = juce::Rectangle<int>(88, 315 - 15, smw, smh);
juce::Rectangle<int> ButtonControl::rect8 = juce::Rectangle<int>(170, 315 - 15, smw, smh);
juce::Rectangle<int> ButtonControl::rect9 = juce::Rectangle<int>(251, 315 - 15, smw, smh);
juce::Rectangle<int> ButtonControl::shift = juce::Rectangle<int>(90, 462, smw, smh);
juce::Rectangle<int> ButtonControl::enter = juce::Rectangle<int>(253, 460, smw, smh);
juce::Rectangle<int> ButtonControl::banka = juce::Rectangle<int>(942, 259, mw, mh);
juce::Rectangle<int> ButtonControl::bankb = juce::Rectangle<int>(1026, 260, mw, mh);
juce::Rectangle<int> ButtonControl::bankc = juce::Rectangle<int>(1110, 257, mw, mh);
juce::Rectangle<int> ButtonControl::bankd = juce::Rectangle<int>(1192, 258, mw, mh);
juce::Rectangle<int> ButtonControl::fulllevel = juce::Rectangle<int>(777, 178, mw, mh);
juce::Rectangle<int> ButtonControl::sixteenlevels = juce::Rectangle<int>(861, 180, mw, mh);
juce::Rectangle<int> ButtonControl::nextseq = juce::Rectangle<int>(778, 263, mw, mh);
juce::Rectangle<int> ButtonControl::trackmute = juce::Rectangle<int>(860, 261, mw, mh);
juce::Rectangle<int> ButtonControl::left = juce::Rectangle<int>(416, 581, 29, 76);
juce::Rectangle<int> ButtonControl::up = juce::Rectangle<int>(445, 581, 46, 38);
juce::Rectangle<int> ButtonControl::down = juce::Rectangle<int>(445, 619, 46, 38);
juce::Rectangle<int> ButtonControl::right = juce::Rectangle<int>(491, 581, 29, 76);

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
  Component::setBounds(rect);
}

void ButtonControl::mouseDown(const juce::MouseEvent&)
{
    button.lock()->push();
}

void ButtonControl::mouseUp(const juce::MouseEvent&)
{
    button.lock()->release();
}
