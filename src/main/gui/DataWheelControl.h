#include "VmpcTooltipComponent.hpp"

#include <hardware/DataWheel.hpp>

#include "MouseWheelControllable.hpp"

#include <set>

class DataWheelControl 
	: public VmpcTooltipComponent
{
public:
	DataWheelControl(mpc::Mpc& mpc, std::weak_ptr<mpc::hardware::DataWheel> dataWheel);

	~DataWheelControl() override;
	void setImage(juce::Image image, int numFrames);
	int getFrameWidth() const { return frameWidth; }
	int getFrameHeight() const { return frameHeight; }
	void paint(juce::Graphics& g) override;

	void mouseDrag(const juce::MouseEvent&) override;
	void mouseDown(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

	void updateUI(int increment);

private:
    MouseWheelControllable mouseWheelControllable;
    juce::Image filmStripImage;
	int numFrames;
	int frameWidth, frameHeight;

    std::set<int> mouseDownEventSources;
    juce::Time latestMouseDownTime = juce::Time(0);
	int dataWheelIndex = 0;
	float lastDy = 0;
	double pixelCounter = 0;
	double fineSensitivity = 0.06;
	std::weak_ptr<mpc::hardware::DataWheel> dataWheel;
};
