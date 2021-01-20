#pragma once
#include "VmpcComponent.h"

#include <string>
#include <memory>
#include <unordered_map>

namespace mpc ::hardware {
class Button;
}

class ButtonControl
: public VmpcComponent
{
    
private:
    juce::Rectangle<float> rect;
    std::weak_ptr<mpc::hardware::Button> button;
    bool mouseEntered = false;
    static const int smw = 30;
    static const int smh = 15;
    static const int mw = 40;
    static const int mh = 20;
    static const int bw = 48;
    static const int bh = 35;
    
    
public:
    static std::unordered_map<std::string, juce::Rectangle<float>*> rects;
    static void initRects();
    
private:
    static juce::Rectangle<float> undoseq;
    static juce::Rectangle<float> erase;
    static juce::Rectangle<float> rec;
    static juce::Rectangle<float> overdub;
    static juce::Rectangle<float> stop;
    static juce::Rectangle<float> play;
    static juce::Rectangle<float> playstart;
    static juce::Rectangle<float> mainscreen;
    static juce::Rectangle<float> openwindow;
    static juce::Rectangle<float> taptemponoterepeat;
    static juce::Rectangle<float> prevstepevent;
    static juce::Rectangle<float> nextstepevent;
    static juce::Rectangle<float> gotoRect;
    static juce::Rectangle<float> prevbarstart;
    static juce::Rectangle<float> nextbarend;
    static juce::Rectangle<float> f1;
    static juce::Rectangle<float> f2;
    static juce::Rectangle<float> f3;
    static juce::Rectangle<float> f4;
    static juce::Rectangle<float> f5;
    static juce::Rectangle<float> f6;
    static juce::Rectangle<float> notevariationafter;
    static juce::Rectangle<float> rect0;
    static juce::Rectangle<float> rect1;
    static juce::Rectangle<float> rect2;
    static juce::Rectangle<float> rect3;
    static juce::Rectangle<float> rect4;
    static juce::Rectangle<float> rect5;
    static juce::Rectangle<float> rect6;
    static juce::Rectangle<float> rect7;
    static juce::Rectangle<float> rect8;
    static juce::Rectangle<float> rect9;
    static juce::Rectangle<float> shift;
    static juce::Rectangle<float> enter;
    static juce::Rectangle<float> banka;
    static juce::Rectangle<float> bankb;
    static juce::Rectangle<float> bankc;
    static juce::Rectangle<float> bankd;
    static juce::Rectangle<float> fulllevel;
    static juce::Rectangle<float> sixteenlevels;
    static juce::Rectangle<float> nextseq;
    static juce::Rectangle<float> trackmute;
    static juce::Rectangle<float> left;
    static juce::Rectangle<float> up;
    static juce::Rectangle<float> down;
    static juce::Rectangle<float> right;
    
public:
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void setBounds();
    
public:
    ButtonControl(juce::Rectangle<float> rect, std::weak_ptr<mpc::hardware::Button> button);
    
};
