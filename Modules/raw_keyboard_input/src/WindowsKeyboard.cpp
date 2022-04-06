#include "WindowsKeyboard.h"

#include <functional>

std::set<WindowsKeyboard*> WindowsKeyboard::thisses;

union KeyInfo
{
	// LPARAM
	LPARAM lParam;

	struct {
		unsigned nRepeatCount : 16;
		unsigned nScanCode : 8;
		unsigned nExtended : 1;
		unsigned nReserved : 4;
		unsigned nContext : 1;
		unsigned nPrev : 1;
		unsigned nTrans : 1;
	};
};

LRESULT CALLBACK WindowsKeyboard::keyHandler2(int keyCode, WPARAM w, LPARAM l) {
	KeyInfo i;
	i.lParam = l;
	
	juce::ComponentPeer* focusedPeer = nullptr;

	for (int i = 0; i < juce::ComponentPeer::getNumPeers(); i++)
		if (juce::ComponentPeer::getPeer(i)->isFocused()) {
			focusedPeer = juce::ComponentPeer::getPeer(i);
		}

	if (focusedPeer != nullptr) {
		for (auto t : thisses) {
			if (t->peer == focusedPeer) {
				if (i.nTrans == 0)
					t->addPressedKey(w);
				else
					t->removePresedKey(w);
			}
		}
	}
	else {
		return CallNextHookEx(nullptr, keyCode, w, l);
	}

	return 0;
}

WindowsKeyboard::WindowsKeyboard()
{
	thisses.emplace(this);
	SetWindowsHookA(WH_KEYBOARD, (HOOKPROC) keyHandler2);
}

WindowsKeyboard::~WindowsKeyboard()
{
	thisses.erase(this);
}
