#include "InputCatcherControl.h"

//#include "Constants.hpp"

#include <Mpc.hpp>

#include <controls/Controls.hpp>
#include <hardware/Hardware.hpp>
#include <hardware/DataWheel.hpp>
#include <hardware/Button.hpp>
#include <hardware/HwPad.hpp>

//#include  "../resource.h"

using namespace mpc::controls;

InputCatcherControl::InputCatcherControl(const String& componentName, mpc::Mpc* mpc)
	: Component(componentName)
{
	this->mpc = mpc;
	//kbMapping = new mpc::controls::KbMapping();
}

void InputCatcherControl::modifierKeysChanged(const ModifierKeys& modifiers) {
	MLOG("ipc mod keys");
	auto c = mpc->getControls().lock();
	auto hw = mpc->getHardware().lock();
	if (modifiers.isShiftDown() && !c->isShiftPressed()) {
		hw->getButton("shift").lock()->push();
	}
	if (!modifiers.isShiftDown() && c->isShiftPressed()) {
		hw->getButton("shift").lock()->release();
	}
	if (modifiers.isCtrlDown() && !c->isCtrlPressed()) {
		c->setCtrlPressed(true);
	}
	if (!modifiers.isCtrlDown() && c->isCtrlPressed()) {
		c->setCtrlPressed(false);
	}
	if (modifiers.isAltDown() && !c->isAltPressed()) {
		c->setAltPressed(true);
	}
	if (!modifiers.isAltDown() && c->isAltPressed()) {
		c->setAltPressed(false);
	}
}

bool InputCatcherControl::keyPressed(const KeyPress &key) {

	MLOG("\nkey press received, keycode " + std::to_string(key.getKeyCode()));
	std::string charStr;
	charStr.push_back(key.getTextCharacter());
	MLOG("Key char " + charStr);
	bool alreadyPressed = false;
	for (int i = 0; i < pressedKeys.size(); i++) {
		if (pressedKeys[i] == key.getKeyCode()) {
			alreadyPressed = true;
			break;
		}
	}

	if (!alreadyPressed) pressedKeys.push_back(key.getKeyCode());

	auto k = key.getKeyCode();
	auto hw = mpc->getHardware().lock();
	if (k == KeyPress::leftKey) {
		hw->getButton("left").lock()->push();
		return true;
	}
	else if (k == KeyPress::rightKey) {
		hw->getButton("right").lock()->push();
		return true;
	}
	else if (k == KeyPress::upKey) {
		hw->getButton("up").lock()->push();
	}
	else if (k == KeyPress::downKey) {
		hw->getButton("down").lock()->push();
		return true;
	}
	else if (k == KeyPress::F1Key) {
		hw->getButton("f1").lock()->push();
		return true;
	}
	else if (k == KeyPress::F2Key) {
		hw->getButton("f2").lock()->push();
		return true;
	}
	else if (k == KeyPress::F3Key) {
		hw->getButton("f3").lock()->push();
		return true;
	}
	else if (k == KeyPress::F4Key) {
		hw->getButton("f4").lock()->push();
		return true;
	}
	else if (k == KeyPress::F5Key) {
		hw->getButton("f5").lock()->push();
		return true;
	}
	else if (k == KeyPress::F6Key) {
		hw->getButton("f6").lock()->push();
		return true;
	}
	else if (k == KeyPress::escapeKey) {
		hw->getButton("mainscreen").lock()->push();
		return true;
	}

	if (k == 'L') {
		hw->getButton("rec").lock()->push();
		return true;
	}
	else if (k == ';') {
		hw->getButton("overdub").lock()->push();
		return true;
	}
	else if (k == '\'') {
		hw->getButton("stop").lock()->push();
		return true;
	}
	else if (k == KeyPress::spaceKey) {
		hw->getButton("play").lock()->push();
		return true;
	}
	else if (k == '\\') {
		hw->getButton("playstart").lock()->push();
		return true;
	}
	
	if (k >= 49 && k <= 58) {
		hw->getButton(std::to_string(k - 48)).lock()->push();
		return true;
	}

    /*
	for (int i = 0; i < KbMapping::padKeys().size(); i++) {
		if (k == KbMapping::padKeys().at(i)) {
			hw->getPad(i).lock()->push(127);
			return true;
		}
	}
*/
	if (k == KeyPress::homeKey) {
		hw->getButton("banka").lock()->push();
		return true;
	}
	else if (k == KeyPress::endKey) {
		hw->getButton("bankb").lock()->push();
		return true;
	}
	else if (k == KeyPress::insertKey) {
		hw->getButton("bankc").lock()->push();
		return true;
	}
	else if (k == KeyPress::deleteKey) {
		hw->getButton("bankd").lock()->push();
		return true;
	}

	int c = key.getTextCharacter();

	if (key == 'I') {
		hw->getButton("openwindow").lock()->push();
		return true;
	}

	if (c == '-' || c == '_') {
		auto controls = mpc->getControls().lock();
		auto increment = -1;
		if (controls->isShiftPressed()) increment *= 10;
		if (controls->isAltPressed()) increment *= 10;
		if (controls->isCtrlPressed()) increment *= 10;
		hw->getDataWheel().lock()->turn(increment);
		return true;
	}
	else if (c == '+' || c == '=') {
		auto controls = mpc->getControls().lock();
		auto increment = 1;
		if (controls->isShiftPressed()) increment *= 10;
		if (controls->isAltPressed()) increment *= 10;
		if (controls->isCtrlPressed()) increment *= 10;
		hw->getDataWheel().lock()->turn(increment);
		return true;
	}

	return false;
}

bool InputCatcherControl::keyStateChanged(bool isKeyDown) {
	if (isKeyDown) return false;
	
	int k = -1;
	for (int i = 0; i < pressedKeys.size(); i++) {
		int candidate = pressedKeys[i];
		if (!KeyPress::isKeyCurrentlyDown(candidate)) {
			k = candidate;
			pressedKeys.erase(pressedKeys.begin() + i);
			break;
		}
	}
	if (k == -1) return false;
	MLOG("\nkey release received, keycode " + std::to_string(k));
	auto hw = mpc->getHardware().lock();
/*
	for (int i = 0; i < KbMapping::padKeys().size(); i++) {
		auto padkey = KbMapping::padKeys().at(i);
		if (padkey == k) {
			hw->getPad(i).lock()->release();
			return true;
		}
	}
*/
	if (k == KeyPress::F1Key) {
		hw->getButton("f1").lock()->release();
		return true;
	}
	else if (k == KeyPress::F2Key) {
		hw->getButton("f2").lock()->release();
		return true;
	}
	else if (k == KeyPress::F3Key) {
		hw->getButton("f3").lock()->release();
		return true;
	}
	else if (k == KeyPress::F4Key) {
		hw->getButton("f4").lock()->release();
		return true;
	}
	else if (k == KeyPress::F5Key) {
		hw->getButton("f5").lock()->release();
		return true;
	}
	else if (k == KeyPress::F6Key) {
		hw->getButton("f6").lock()->release();
		return true;
	}

	if (k == 'L') {
		hw->getButton("rec").lock()->release();
		return true;
	}
	else if (k == ';') {
		hw->getButton("overdub").lock()->release();
		return true;
	}
	else if (k == '\'') {
		hw->getButton("stop").lock()->release();
		return true;
	}
	else if (k == KeyPress::spaceKey) {
		hw->getButton("play").lock()->release();
		return true;
	}
	else if (k == '\\') {
		hw->getButton("playstart").lock()->release();
		return true;
	}

	/*
	auto label = kbMapping->getLabelFromKeyCode(c);
	if (label.compare("") != 0) {
		hw->getButton(label).lock()->release();
		return true;
	}
	*/
	return false;
}

InputCatcherControl::~InputCatcherControl() {
}
