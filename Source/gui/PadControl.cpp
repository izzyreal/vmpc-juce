#include "PadControl.hpp"
#include <hardware/HwPad.hpp>

#include <Mpc.hpp>
#include <sequencer/Sequencer.hpp>
#include <sequencer/Track.hpp>
#include <sampler/Sampler.hpp>
#include <sampler/Pad.hpp>
#include <sampler/NoteParameters.hpp>
#include <disk/SoundLoader.hpp>
#include <disk/MpcFile.hpp>

#include <mpc/MpcSoundPlayerChannel.hpp>

#include <file/File.hpp>
#include <lang/StrUtil.hpp>

#include <Logger.hpp>

#include <math.h>

using namespace std;
using namespace moduru::lang;

PadControl::PadControl(mpc::Mpc& mpc, Rectangle <float> rect, std::weak_ptr<mpc::hardware::HwPad> pad, Image padhit, const String &componentName)
	: VmpcComponent(componentName), mpc(mpc)
{
	this->pad = pad;
	this->padhitImg = padhit;
	this->rect = rect;
	pad.lock()->addObserver(this);
}

bool PadControl::isInterestedInFileDrag(const StringArray& files)
{
	if (files.size() != 1)
	{
		return false;
	}

	for (auto& s : files)
	{
		if (StrUtil::hasEnding(StrUtil::toLower(s.toStdString()), ".snd") || StrUtil::hasEnding(StrUtil::toLower(s.toStdString()), ".wav"))
		{
			return true;
		}
	}
	return false;
}

void PadControl::filesDropped(const StringArray& files, int x, int y)
{
	if (files.size() != 1)
	{
		return;
	}

	const auto padIndex = pad.lock()->getIndex();

	for (auto& s : files)
	{
		if (StrUtil::hasEnding(StrUtil::toLower(s.toStdString()), ".snd") || StrUtil::hasEnding(StrUtil::toLower(s.toStdString()), ".wav"))
		{
			auto sampler = mpc.getSampler().lock();

			auto soundLoader = mpc::disk::SoundLoader(mpc, sampler->getSounds(), false);
			soundLoader.setPreview(false);
			soundLoader.setPartOfProgram(false);
			bool hasNotBeenLoadedAlready = true;

			auto compatiblePath = StrUtil::replaceAll(s.toStdString(), '\\', string("\\"));
			auto moduruFile = dynamic_pointer_cast<moduru::file::FsNode>(make_shared<moduru::file::File>(compatiblePath, nullptr));

			mpc::disk::MpcFile file(moduruFile);

			auto layeredScreen = mpc.getLayeredScreen().lock();

			try
			{
				hasNotBeenLoadedAlready = soundLoader.loadSound(&file) == -1;
			}
			catch (const exception& exception)
			{
				MLOG("A problem occurred when trying to load " + moduruFile->getName() + ": " + string(exception.what()));
				layeredScreen->openScreen(layeredScreen->getPreviousScreenName());
				return;
			}

			if (hasNotBeenLoadedAlready)
			{
				auto drumIndex = mpc.getSequencer().lock()->getActiveTrack().lock()->getBus() - 1;
				
				if (drumIndex == -1)
				{
					layeredScreen->openScreen(layeredScreen->getPreviousScreenName());
					return;
				}

				auto mpcSoundPlayerChannel = mpc.getDrum(drumIndex);

				auto programIndex = mpcSoundPlayerChannel->getProgram();
				auto program = dynamic_pointer_cast<mpc::sampler::Program>(mpc.getSampler().lock()->getProgram(programIndex).lock());
				auto soundIndex = mpc.getSampler().lock()->getSoundCount() - 1;
				auto programPad = program->getPad(padIndex);
				auto padNote = programPad->getNote();
				
				auto noteParameters = dynamic_cast<mpc::sampler::NoteParameters*>(program->getNoteParameters(padNote));

				if (noteParameters == nullptr)
				{
					layeredScreen->openScreen(layeredScreen->getPreviousScreenName());
					return;
				}

				noteParameters->setSoundIndex(soundIndex);
				layeredScreen->openScreen(layeredScreen->getPreviousScreenName());
			}
		}
	}
}

void PadControl::timerCallback()
{
	if (fading)
	{
		padhitBrightness -= 20;
	}

	if (padhitBrightness < 0)
	{
		padhitBrightness = 0;
		repaint();
		fading = false;
		stopTimer();
	}
	else
	{
		repaint();
	}
}

void PadControl::update(moduru::observer::Observable* o, nonstd::any arg)
{
	int velocity = nonstd::any_cast<int>(arg);

	if (velocity == 255)
	{
		fading = true;
		pressed = false;
	}
	else
	{
		padhitBrightness = velocity + 25;
		pressed = true;
		fading = false;
		startTimer(100);
	}
}

int PadControl::getVelo(int x, int y)
{
	float centX = rect.getCentreX() - rect.getX();
	float centY = rect.getCentreY() - rect.getY();
	float distX = x - centX;
	float distY = y - centY;
	float powX = pow(distX, 2);
	float powY = pow(distY, 2);
	float dist = sqrt(powX + powY);
	if (dist > 46) dist = 46;
	int velo = 127 - (dist * (127.0 / 48.0));
	return velo;
}

void PadControl::mouseDown(const MouseEvent& event)
{
	pad.lock()->push(getVelo(event.x, event.y));
}

void PadControl::mouseDoubleClick(const MouseEvent& event)
{
}

void PadControl::mouseUp(const MouseEvent& event)
{
	pad.lock()->release();
}

void PadControl::setBounds()
{
	setSize(rect.getWidth(), rect.getHeight());
	Component::setBounds(rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
}

void PadControl::paint(Graphics& g)
{
	auto img = padhitImg.createCopy();
	float mult = (float)(padhitBrightness) / 150.0;
	img.multiplyAllAlphas(mult);
	g.drawImageAt(img, 0, 0);
}

PadControl::~PadControl()
{
	pad.lock()->deleteObserver(this);
}
