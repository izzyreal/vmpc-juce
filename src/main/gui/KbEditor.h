#pragma once
#include "VmpcComponent.h"

namespace mpc { class Mpc; }

class KbEditor
	: public VmpcComponent
	, public Timer
{
private:
    mpc::Mpc& mpc;
    Image lcd;

    void checkDirty();

public:
	void paint(Graphics& g) override;
	void timerCallback() override;

public:
    KbEditor(mpc::Mpc& mpc);

};
