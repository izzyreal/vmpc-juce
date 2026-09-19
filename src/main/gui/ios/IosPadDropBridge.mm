#include "IosPadDropBridge.hpp"

#if JUCE_IOS
#include "gui/vector/Pad.hpp"
#include <Logger.hpp>

#import <UIKit/UIKit.h>

#include <optional>

namespace
{
    using Pad = vmpc_juce::gui::vector::Pad;

    struct DropState
    {
        explicit DropState(juce::Component &component) : editor(&component) {}

        ~DropState()
        {
            directory.deleteRecursively();
        }

        juce::Component::SafePointer<juce::Component> editor;
        const juce::File directory =
            juce::File::getSpecialLocation(juce::File::tempDirectory)
                .getChildFile("VMPC2000XL-drop-" + juce::Uuid().toString());

        // Accessed only on the message thread. Background work owns the state
        // until its completion, keeping its directory alive after editor close.
        bool active = true;
        unsigned generation = 0;
    };

    struct OfferedFile
    {
        juce::String name;
        NSString *type;
    };

    bool isSampleName(const juce::String &name)
    {
        return name.endsWithIgnoreCase(".wav") ||
               name.endsWithIgnoreCase(".snd");
    }

    std::optional<OfferedFile> offeredFile(NSItemProvider *provider)
    {
        auto name = juce::String::fromUTF8(provider.suggestedName.UTF8String);
        const bool offersWav = [provider
            hasItemConformingToTypeIdentifier:@"com.microsoft.waveform-audio"];
        if (name.isEmpty())
        {
            name = "Sample";
        }
        if (!name.containsChar('.') && offersWav)
        {
            name += ".wav";
        }
        if (!isSampleName(name) ||
            ![provider hasItemConformingToTypeIdentifier:@"public.data"])
        {
            return std::nullopt;
        }
        return OfferedFile{juce::File::createLegalFileName(name),
                           offersWav && name.endsWithIgnoreCase(".wav")
                               ? @"com.microsoft.waveform-audio"
                               : @"public.data"};
    }

    Pad *padAt(const DropState &state, UIView *view, id<UIDropSession> session)
    {
        auto *editor = state.editor.getComponent();
        if (!state.active || editor == nullptr || session.items.count != 1)
        {
            return nullptr;
        }
        auto *peer = editor->getPeer();
        if (peer == nullptr || peer->getNativeHandle() != view)
        {
            return nullptr;
        }

        const auto location = [session locationInView:view];
        const auto position = editor->getLocalPoint(
            &peer->getComponent(),
            juce::Point<int>(juce::roundToInt(location.x),
                             juce::roundToInt(location.y)));
        for (auto *component = editor->getComponentAt(position);
             component != nullptr && component != editor;
             component = component->getParentComponent())
        {
            if (auto *pad = dynamic_cast<Pad *>(component))
            {
                return pad->isCurrentlyBlockedByAnotherModalComponent()
                           ? nullptr
                           : pad;
            }
        }
        return nullptr;
    }
} // namespace

@interface VMPCPadDropDelegate : NSObject <UIDropInteractionDelegate>
{
@public
    std::shared_ptr<DropState> state;
}
@end

@implementation VMPCPadDropDelegate
- (BOOL)dropInteraction:(UIDropInteraction *)interaction
       canHandleSession:(id<UIDropSession>)session
{
    juce::ignoreUnused(interaction);
    return state->active && session.items.count == 1 &&
           offeredFile(session.items.firstObject.itemProvider).has_value();
}

- (UIDropProposal *)dropInteraction:(UIDropInteraction *)interaction
                   sessionDidUpdate:(id<UIDropSession>)session
{
    const auto file = session.items.count == 1
                          ? offeredFile(session.items.firstObject.itemProvider)
                          : std::nullopt;
    auto *pad = padAt(*state, interaction.view, session);
    const bool accepted =
        file && pad != nullptr && pad->isInterestedInFileDrag({file->name});
    return [[[UIDropProposal alloc]
        initWithDropOperation:accepted ? UIDropOperationCopy
                                       : UIDropOperationForbidden] autorelease];
}

- (void)dropInteraction:(UIDropInteraction *)interaction
            performDrop:(id<UIDropSession>)session
{
    auto *pad = padAt(*state, interaction.view, session);
    if (pad == nullptr)
    {
        return;
    }
    NSItemProvider *provider = session.items.firstObject.itemProvider;
    const auto file = offeredFile(provider);
    if (!file || !pad->isInterestedInFileDrag({file->name}))
    {
        return;
    }

    const auto transferState = state;
    const auto generation = state->generation;
    const juce::Component::SafePointer<Pad> target(pad);
    const auto destination =
        state->directory.getChildFile(juce::Uuid().toString())
            .getChildFile(file->name);

    auto completion = ^(NSURL *url, NSError *error) {
      // The provider removes its temporary file when
      // this block returns. Do not defer the copy to
      // the message thread.
      juce::String failure;
      if (error != nil)
      {
          failure =
              juce::String::fromUTF8(error.localizedDescription.UTF8String);
      }
      else if (url == nil || !url.isFileURL)
      {
          failure =
              "The source did not provide a readable "
              "sample file.";
      }
      else
      {
          const bool scoped = [url startAccessingSecurityScopedResource];
          const juce::File source(juce::String::fromUTF8(url.path.UTF8String));
          if (!isSampleName(destination.getFileName()) ||
              !source.existsAsFile() ||
              !destination.getParentDirectory().createDirectory().wasOk() ||
              !source.copyFileTo(destination))
          {
              failure =
                  "The dropped sample could not be "
                  "copied.";
          }
          if (scoped)
          {
              [url stopAccessingSecurityScopedResource];
          }
      }

      juce::MessageManager::callAsync(
          [transferState, generation, target, destination, failure]
          {
              auto *editor = transferState->editor.getComponent();
              auto *destinationPad = target.getComponent();
              if (!transferState->active ||
                  generation != transferState->generation ||
                  editor == nullptr || destinationPad == nullptr ||
                  !editor->isParentOf(destinationPad))
              {
                  destination.getParentDirectory().deleteRecursively();
                  return;
              }
              if (failure.isNotEmpty())
              {
                  destination.getParentDirectory().deleteRecursively();
                  MLOG("Sample drop failed: " + failure.toStdString());
                  juce::NativeMessageBox::showMessageBoxAsync(
                      juce::MessageBoxIconType::WarningIcon,
                      "Sample drop failed", failure, editor);
                  return;
              }
              if (destinationPad->isCurrentlyBlockedByAnotherModalComponent())
              {
                  destination.getParentDirectory().deleteRecursively();
                  return;
              }
              destinationPad->filesDropped({destination.getFullPathName()}, 0,
                                           0);
          });
    };
    [provider loadFileRepresentationForTypeIdentifier:file->type
                                    completionHandler:completion];
}
@end

namespace vmpc_juce::gui::ios
{
    struct IosPadDropBridge::Impl
    {
        explicit Impl(juce::Component &editor)
            : state(std::make_shared<DropState>(editor))
        {
            delegate = [[VMPCPadDropDelegate alloc] init];
            delegate->state = state;
        }

        ~Impl()
        {
            state->active = false;
            detach();
            [delegate release];
        }

        void detach()
        {
            ++state->generation;
            if (interaction != nil)
            {
                [view removeInteraction:interaction];
                [interaction release];
                interaction = nil;
            }
            [view release];
            view = nil;
        }

        void refreshPeer()
        {
            auto *editor = state->editor.getComponent();
            auto *peer = editor != nullptr ? editor->getPeer() : nullptr;
            UIView *newView =
                peer != nullptr ? (UIView *)peer->getNativeHandle() : nil;
            if (view == newView)
            {
                return;
            }
            detach();
            view = [newView retain];
            if (view != nil)
            {
                interaction =
                    [[UIDropInteraction alloc] initWithDelegate:delegate];
                interaction.allowsSimultaneousDropSessions = NO;
                [view addInteraction:interaction];
            }
        }

        std::shared_ptr<DropState> state;
        VMPCPadDropDelegate *delegate = nil;
        UIView *view = nil;
        UIDropInteraction *interaction = nil;
    };

    IosPadDropBridge::IosPadDropBridge(juce::Component &editor)
        : impl(std::make_unique<Impl>(editor))
    {
        refreshPeer();
    }

    IosPadDropBridge::~IosPadDropBridge() = default;

    void IosPadDropBridge::refreshPeer()
    {
        impl->refreshPeer();
    }
} // namespace vmpc_juce::gui::ios
#endif
