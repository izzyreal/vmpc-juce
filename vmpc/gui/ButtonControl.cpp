#include "ButtonControl.hpp"
#include <hardware/Button.hpp>

//#include "FTControl.hpp"
//#include <controls/KbMapping.hpp>

//#include <math.h>
//#include <algorithm>

//#include <Logger.hpp>
//#include <thirdp/wrpkey/key.hxx>

using namespace std;

ButtonControl::ButtonControl(Rectangle<float> rect, std::weak_ptr<mpc::hardware::Button> button, const String& componentName)
	: VmpcComponent(componentName)
{
	this->button = button;
	this->rect = rect;

	//MLOG("button name: " + button.lock()->getLabel());
	//MLOG("Rect coords: " + to_string(rect.getX()) + ", " + to_string(rect.getY()) + ", " + to_string(rect.getRight()) + ", " + to_string(rect.getBottom()));

	/*
	mpc::controls::KbMapping kbMapping;
	auto buttonLabel = button.lock()->getLabel();
	int keyCode = kbMapping.getKeyCodeFromLabel(buttonLabel);
	auto labelString = kbMapping.getKeyCodeString(keyCode);
	std::transform(labelString.begin(), labelString.end(), labelString.begin(), ::toupper);
	int labelWidth = FTControl::getStringWidth(labelString);
	int yMouseCursorOffset = Constants::KBLABEL_FONT_SIZE;
	int labelOffsetX = rect.MW() - (labelWidth / 2) - ((Constants::KBLABEL_OUTLINE_SIZE));
	int labelOffsetY = rect.MH() - (((Constants::KBLABEL_FONT_SIZE / 2) - Constants::KBLABEL_OUTLINE_SIZE + yMouseCursorOffset));
	kbLabel = new FTControl(pPlug, labelOffsetX, labelOffsetY, labelString);
	kbLabel->Hide(true);
	gui->AttachControl(kbLabel);
	*/
}

std::unordered_map<std::string, Rectangle<float>*> ButtonControl::rects;
Rectangle<float> ButtonControl::undoseq = Rectangle<float>(212, 652, mw, mh);
Rectangle<float> ButtonControl::erase = Rectangle<float>(291, 651, mw, mh);
Rectangle<float> ButtonControl::rec = Rectangle<float>(208, 808, bw, bh);
Rectangle<float> ButtonControl::overdub = Rectangle<float>(288, 806, bw, bh);
Rectangle<float> ButtonControl::stop = Rectangle<float>(367, 804, bw, bh);
Rectangle<float> ButtonControl::play = Rectangle<float>(446, 802, bw, bh);
Rectangle<float> ButtonControl::playstart = Rectangle<float>(525, 800, bw, bh);
Rectangle<float> ButtonControl::mainscreen = Rectangle<float>(382, 307, mw, mh);
Rectangle<float> ButtonControl::openwindow = Rectangle<float>(510, 308, mw, mh);
Rectangle<float> ButtonControl::taptemponoterepeat = Rectangle<float>(247, 556, bw, bh);
Rectangle<float> ButtonControl::prevstepevent = Rectangle<float>(212, 723, mw, mh);
Rectangle<float> ButtonControl::nextstepevent = Rectangle<float>(292, 721, mw, mh);
Rectangle<float> ButtonControl::gotoRect = Rectangle<float>(372, 719, mw, mh);
Rectangle<float> ButtonControl::prevbarstart = Rectangle<float>(450, 716, mw, mh);
Rectangle<float> ButtonControl::nextbarend = Rectangle<float>(528, 718, mw, mh);
Rectangle<float> ButtonControl::f1 = Rectangle<float>(144, 262, smw, smh);
Rectangle<float> ButtonControl::f2 = Rectangle<float>(225, 261, smw, smh);
Rectangle<float> ButtonControl::f3 = Rectangle<float>(310, 263, smw, smh);
Rectangle<float> ButtonControl::f4 = Rectangle<float>(394, 263, smw, smh);
Rectangle<float> ButtonControl::f5 = Rectangle<float>(474, 261, smw, smh);
Rectangle<float> ButtonControl::f6 = Rectangle<float>(554, 262, smw, smh);
Rectangle<float> ButtonControl::notevariationafter = Rectangle<float>(88, 568, mw, mh);
Rectangle<float> ButtonControl::rect0 = Rectangle<float>(170, 460, smw, smh);
Rectangle<float> ButtonControl::rect1 = Rectangle<float>(91, 413, smw, smh);
Rectangle<float> ButtonControl::rect2 = Rectangle<float>(174, 414, smw, smh);
Rectangle<float> ButtonControl::rect3 = Rectangle<float>(253, 415, smw, smh);
Rectangle<float> ButtonControl::rect4 = Rectangle<float>(91, 365, smw, smh);
Rectangle<float> ButtonControl::rect5 = Rectangle<float>(173, 364, smw, smh);
Rectangle<float> ButtonControl::rect6 = Rectangle<float>(253, 365, smw, smh);
Rectangle<float> ButtonControl::rect7 = Rectangle<float>(88, 315, smw, smh);
Rectangle<float> ButtonControl::rect8 = Rectangle<float>(170, 315, smw, smh);
Rectangle<float> ButtonControl::rect9 = Rectangle<float>(251, 315, smw, smh);
Rectangle<float> ButtonControl::shift = Rectangle<float>(90, 462, smw, smh);
Rectangle<float> ButtonControl::enter = Rectangle<float>(253, 460, smw, smh);
Rectangle<float> ButtonControl::banka = Rectangle<float>(942, 259, mw, mh);
Rectangle<float> ButtonControl::bankb = Rectangle<float>(1026, 260, mw, mh);
Rectangle<float> ButtonControl::bankc = Rectangle<float>(1110, 257, mw, mh);
Rectangle<float> ButtonControl::bankd = Rectangle<float>(1192, 258, mw, mh);
Rectangle<float> ButtonControl::fulllevel = Rectangle<float>(777, 178, mw, mh);
Rectangle<float> ButtonControl::sixteenlevels = Rectangle<float>(861, 180, mw, mh);
Rectangle<float> ButtonControl::nextseq = Rectangle<float>(778, 263, mw, mh);
Rectangle<float> ButtonControl::trackmute = Rectangle<float>(860, 261, mw, mh);

void ButtonControl::initRects() {
	if (rects.size() != 0) return;

	rects.emplace("rec", &rec);
	rects.emplace("overdub", &overdub);
	rects.emplace("stop", &stop);
	rects.emplace("play", &play);
	rects.emplace("playstart", &playstart);
	rects.emplace("mainscreen", &mainscreen);
	rects.emplace("prevstepevent", &prevstepevent);
	rects.emplace("nextstepevent", &nextstepevent);
	rects.emplace("goto", &gotoRect);
	rects.emplace("prevbarstart", &prevbarstart);
	rects.emplace("nextbarend", &nextbarend);
	rects.emplace("tap", &taptemponoterepeat);
	rects.emplace("nextseq", &nextseq);
	rects.emplace("trackmute", &trackmute);
	rects.emplace("openwindow", &openwindow);
	rects.emplace("fulllevel", &fulllevel);
	rects.emplace("sixteenlevels", &sixteenlevels);
	rects.emplace("f1", &f1);
	rects.emplace("f2", &f2);
	rects.emplace("f3", &f3);
	rects.emplace("f4", &f4);
	rects.emplace("f5", &f5);
	rects.emplace("f6", &f6);
	rects.emplace("shift", &shift);
	rects.emplace("enter", &enter);
	rects.emplace("undoseq", &undoseq);
	rects.emplace("erase", &erase);
	rects.emplace("after", &notevariationafter);
	rects.emplace("banka", &banka);
	rects.emplace("bankb", &bankb);
	rects.emplace("bankc", &bankc);
	rects.emplace("bankd", &bankd);
	for (auto r : rects) {
		auto rc = r.second;
		r.second->setY(r.second->getY() + 55);
	}
}

void ButtonControl::setBounds() {
	setSize(rect.getWidth(), rect.getHeight());
	Component::setBounds(rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
}

void ButtonControl::paint(Graphics& g) {
	//g.drawRect(Rectangle<float>(0, 0, rect.getWidth(), rect.getHeight()));
	//g.fillRect(Rectangle<float>(0, 0, rect.getWidth(), rect.getHeight()));
}

void ButtonControl::mouseDown(const MouseEvent& event) {
	button.lock()->push();
	//kbLabel->Hide(true);
}

void ButtonControl::mouseUp(const MouseEvent& event) {
	button.lock()->release();
}

//void ButtonControl::mouseOver(const MouseEvent& event) {
	//if (mouseEntered) return;
	//mouseEntered = true;
	//kbLabel->Hide(false);
//}

/*
void ButtonControl::OnMouseOut() {
	mouseEntered = false;
	kbLabel->Hide(true);
}
*/

ButtonControl::~ButtonControl() {
}
