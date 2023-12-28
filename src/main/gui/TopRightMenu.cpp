#include "TopRightMenu.hpp"

#include "../ResourceUtil.h"

#include "Mpc.hpp"

#if JUCE_IOS

void doPresentShareOptions(void* nativeWindowHandle, mpc::Mpc*);

void doOpenIosImportDocumentBrowser(ImportDocumentUrlProcessor*, void* nativeWindowHandle);

void doPresentRecordingManager(void* nativeWindowHandle, mpc::Mpc*);

#endif

TopRightMenu::TopRightMenu(mpc::Mpc& mpc, std::function<void()>& showAudioSettingsDialog)
{
    auto transparentWhite = juce::Colours::transparentWhite;

    keyboardImg = vmpc::ResourceUtil::loadImage("img/keyboard.png");

    keyboardButton.setImages(false, true, true, keyboardImg, 0.5, transparentWhite, keyboardImg, 1.0, transparentWhite,
                             keyboardImg, 0.25, transparentWhite);

#if JUCE_IOS
    importDocumentUrlProcessor.mpc = &mpc;
    
    importImg = vmpc::ResourceUtil::loadImage("img/import.png");

    importButton.setImages(false, true, true, importImg, 0.5, transparentWhite, importImg, 1.0, transparentWhite,
                           importImg, 0.25, transparentWhite);

    importButton.setTooltip("Import files or folders");

    importButton.onClick = [&]() {
        auto uiView = getPeer()->getNativeHandle();
        doOpenIosImportDocumentBrowser(&importDocumentUrlProcessor, uiView);
    };

    addAndMakeVisible(importButton);
    
    exportImg = vmpc::ResourceUtil::loadImage("img/export.png");

    exportButton.setImages(false, true, true, exportImg, 0.5, transparentWhite, exportImg, 1.0, transparentWhite,
                           exportImg, 0.25, transparentWhite);

    exportButton.setTooltip("Export files or folders");

    exportButton.onClick = [&]() {
        auto uiView = getPeer()->getNativeHandle();
        doPresentShareOptions(uiView, &mpc);
    };

    addAndMakeVisible(exportButton);
    
    recordingManagerImg = vmpc::ResourceUtil::loadImage("img/folder.png");
    
    recordingManagerButton.setImages(false, true, true, recordingManagerImg, 0.5, transparentWhite, recordingManagerImg, 1.0, transparentWhite, recordingManagerImg, 0.25, transparentWhite);

    recordingManagerButton.setTooltip("Manage recordings");

    recordingManagerButton.onClick = [&]() {
        auto uiView = getPeer()->getNativeHandle();
        doPresentRecordingManager(uiView, &mpc);
    };

    addAndMakeVisible(recordingManagerButton);
#endif
    helpImg = vmpc::ResourceUtil::loadImage("img/help.png");
    helpButton.setImages(false, true, true, helpImg, 0.5, transparentWhite, helpImg, 1.0, transparentWhite,
                         helpImg, 0.25, transparentWhite);
    helpButton.setTooltip("Browse online documentation");
    helpButton.onClick = [] {
        juce::URL url("https://vmpcdocs.izmar.nl");
        url.launchInDefaultBrowser();
    };
    helpButton.setWantsKeyboardFocus(false);
    addAndMakeVisible(helpButton);

#if JUCE_STANDALONE_APPLICATION
    gearImg = vmpc::ResourceUtil::loadImage("img/gear.png");
    gearButton.setImages(false, true, true, gearImg, 0.5, transparentWhite, gearImg, 1.0, transparentWhite,
                         gearImg, 0.25, transparentWhite);
    gearButton.setTooltip("Audio/MIDI Settings");
    gearButton.onClick = [&showAudioSettingsDialog]() {
        showAudioSettingsDialog();
    };
    gearButton.setWantsKeyboardFocus(false);
    addAndMakeVisible(gearButton);
#endif
    
    keyboardButton.setTooltip("Configure computer keyboard");
    keyboardButton.onClick = [&]() {
        mpc.getLayeredScreen()->openScreen("vmpc-keyboard");
    };

    keyboardButton.setWantsKeyboardFocus(false);
    addAndMakeVisible(keyboardButton);

#ifndef JUCE_IOS
    resetWindowSizeImg = vmpc::ResourceUtil::loadImage("img/reset-window-size.png");

    resetWindowSizeButton.setImages(false, true, true, resetWindowSizeImg, 0.5, transparentWhite, resetWindowSizeImg,
                                    1.0, transparentWhite, resetWindowSizeImg, 0.25, transparentWhite);

    resetWindowSizeButton.setTooltip("Reset window size");
    resetWindowSizeButton.onClick = [&]() {
        getParentComponent()->getParentComponent()->getParentComponent()->getParentComponent()->setSize(1298 / 2, 994 / 2);
    };
    resetWindowSizeButton.setWantsKeyboardFocus(false);
    addAndMakeVisible(resetWindowSizeButton);
#endif
}

void TopRightMenu::resized()
{
    juce::FlexBox flexBox;
    flexBox.flexDirection = juce::FlexBox::Direction::row;
    flexBox.justifyContent = juce::FlexBox::JustifyContent::flexEnd;
    flexBox.alignItems = juce::FlexBox::AlignItems::center;
    flexBox.flexWrap = juce::FlexBox::Wrap::noWrap;
    
#if JUCE_IOS
    flexBox.items.add(juce::FlexItem(recordingManagerButton).withMinWidth(50).withMinHeight(60));
    flexBox.items.add(juce::FlexItem(exportButton).withMinWidth(50).withMinHeight(60));
    flexBox.items.add(juce::FlexItem(importButton).withMinWidth(60).withMinHeight(60));
#endif
    
#if JUCE_STANDALONE_APPLICATION
        flexBox.items.add(juce::FlexItem(gearButton).withMinWidth(50).withMinHeight(50));
#endif

#ifndef JUCE_IOS
    flexBox.items.add(juce::FlexItem(resetWindowSizeButton).withMinWidth(50).withMinHeight(50));
#endif
    flexBox.items.add(juce::FlexItem(helpButton).withMinWidth(50).withMinHeight(50));
    flexBox.items.add(juce::FlexItem(keyboardButton).withMinWidth(80).withMinHeight(60));
    
    flexBox.performLayout(getLocalBounds());
}
