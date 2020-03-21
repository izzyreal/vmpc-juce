#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "../gui/VmpcComponent.h"

#include <string>
#include <memory>
#include <unordered_map>

namespace mpc {
	namespace hardware {
		class Button;
	}
}

class ButtonControl
	: public VmpcComponent
	{

	private:
		Rectangle<float> rect;
	std::weak_ptr<mpc::hardware::Button> button;
	//FTControl* kbLabel;
	bool mouseEntered = false;
	static const int smw = 30;
	static const int smh = 15;
	static const int mw = 40;
	static const int mh = 20;
	static const int bw = 48;
	static const int bh = 35;
	

	public:
		static std::unordered_map<std::string, Rectangle<float>*> rects;
		static void initRects();

	private:
		static Rectangle<float> undoseq;
		static Rectangle<float> erase;
		static Rectangle<float> rec;
		static Rectangle<float> overdub;
		static Rectangle<float> stop;
		static Rectangle<float> play;
		static Rectangle<float> playstart;
		static Rectangle<float> mainscreen;
		static Rectangle<float> openwindow;
		static Rectangle<float> taptemponoterepeat;
		static Rectangle<float> prevstepevent;
		static Rectangle<float> nextstepevent;
		static Rectangle<float> gotoRect;
		static Rectangle<float> prevbarstart;
		static Rectangle<float> nextbarend;
		static Rectangle<float> f1;
		static Rectangle<float> f2;
		static Rectangle<float> f3;
		static Rectangle<float> f4;
		static Rectangle<float> f5;
		static Rectangle<float> f6;
		static Rectangle<float> notevariationafter;
		static Rectangle<float> rect0;
		static Rectangle<float> rect1;
		static Rectangle<float> rect2;
		static Rectangle<float> rect3;
		static Rectangle<float> rect4;
		static Rectangle<float> rect5;
		static Rectangle<float> rect6;
		static Rectangle<float> rect7;
		static Rectangle<float> rect8;
		static Rectangle<float> rect9;
		static Rectangle<float> shift;
		static Rectangle<float> enter;
		static Rectangle<float> banka;
		static Rectangle<float> bankb;
		static Rectangle<float> bankc;
		static Rectangle<float> bankd;
		static Rectangle<float> fulllevel;
		static Rectangle<float> sixteenlevels;
		static Rectangle<float> nextseq;
		static Rectangle<float> trackmute;
		static Rectangle<float> left;
		static Rectangle<float> up;
		static Rectangle<float> down;
		static Rectangle<float> right;

public:
	void mouseDown(const MouseEvent& event) override;
	void mouseUp(const MouseEvent& event) override;
	//void mouseOver(const MouseEvent& event) override;
	void setBounds();
	void paint(Graphics& g) override;

public:
	ButtonControl(Rectangle<float> rect, std::weak_ptr<mpc::hardware::Button> button, const String& componentName);
	~ButtonControl();

};
