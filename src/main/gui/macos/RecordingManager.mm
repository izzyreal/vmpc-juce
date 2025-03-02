#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_OSX

#include <Carbon/Carbon.h>
#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>
#include <AVFoundation/AVFoundation.h>

#include "Mpc.hpp"

@interface RecordingsViewController : NSViewController
@property (nonatomic) mpc::Mpc *mpc;
@property (nonatomic, strong) NSButton *currentlyPlayingButton;
@property (nonatomic, strong) NSScrollView *scrollView;
@property (nonatomic, strong) NSMutableArray<NSString *> *directoryPaths;
@property (nonatomic, assign) NSWindow *modalWindow;
@end

@implementation RecordingsViewController {
    AVAudioPlayer *audioPlayer;
    NSFileManager *fileManager;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    self.directoryPaths = [NSMutableArray new];
    fileManager = [NSFileManager defaultManager];
}

- (void)viewDidAppear {
    [super viewDidAppear];
    [self layoutScrollViewAndContents];
    [self.view.window makeFirstResponder:self];
}

- (void)viewWillDisappear {
    [super viewWillDisappear];
    [self stopPlayback];
}

- (void)keyDown:(NSEvent *)event {
    if (event.keyCode == kVK_Escape) {
        [self closeSheet];
    } else {
        [super keyDown:event];
    }
}

- (void)layoutScrollViewAndContents {
    NSView *contentView = self.view;
    [contentView.subviews makeObjectsPerformSelector:@selector(removeFromSuperview)];

    NSWindow *window = self.view.window;
    if (!window) return;

    NSRect contentRect = window.contentLayoutRect;
    
    NSRect scrollViewFrame = NSMakeRect(0, 40, contentRect.size.width, contentRect.size.height - 40);
    self.scrollView = [[NSScrollView alloc] initWithFrame:scrollViewFrame];
    self.scrollView.hasVerticalScroller = YES;
    
    NSView *scrollContentView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, contentRect.size.width, 0)];
    [self.scrollView setDocumentView:scrollContentView];
    [contentView addSubview:self.scrollView];

    [self.directoryPaths removeAllObjects];

    CGFloat yPosition = 0;
    int tag = 0;

    std::vector<fs::directory_entry> entries(fs::directory_iterator(self.mpc->paths->recordingsPath()), {});

    std::sort(entries.begin(), entries.end(), [](const fs::directory_entry &a, const fs::directory_entry &b) {
        std::string aLower = a.path().filename().string(), bLower = b.path().filename().string();
        std::transform(aLower.begin(), aLower.end(), aLower.begin(), ::tolower);
        std::transform(bLower.begin(), bLower.end(), bLower.begin(), ::tolower);
        return aLower > bLower;
    });

    for (auto& p : entries) {
        if (p.path().filename() == ".DS_Store") {
            continue;
        }
        NSString *directoryPath = [NSString stringWithUTF8String:p.path().c_str()];
        [self.directoryPaths addObject:directoryPath];

        NSView *dirView = [[NSView alloc] initWithFrame:NSMakeRect(0, yPosition, contentRect.size.width, 30)];
        [scrollContentView addSubview:dirView];

        NSTextField *dirLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(10, 5, contentRect.size.width - 140, 20)];
        [dirLabel setStringValue:[NSString stringWithUTF8String:p.path().filename().string().c_str()]];
        [dirLabel setBezeled:NO];
        [dirLabel setDrawsBackground:NO];
        [dirLabel setEditable:NO];
        [dirLabel setSelectable:NO];
        [dirView addSubview:dirLabel];

        unsigned long long dirSize = [self directorySizeAtPath:directoryPath];
        CGFloat dirSizeMB = (CGFloat)dirSize / (1024 * 1024);
        NSTextField *sizeLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(contentRect.size.width - 130, 5, 60, 20)];
        [sizeLabel setStringValue:[NSString stringWithFormat:@"%.2f MB", dirSizeMB]];
        [sizeLabel setBezeled:NO];
        [sizeLabel setDrawsBackground:NO];
        [sizeLabel setEditable:NO];
        [sizeLabel setSelectable:NO];
        [dirView addSubview:sizeLabel];

        NSButton *playButton = [[NSButton alloc] initWithFrame:NSMakeRect(contentRect.size.width - 70, 0, 30, 30)];
        [playButton setTitle:@"▶️"];
        [playButton setTag:tag];
        [playButton setTarget:self];
        [playButton setAction:@selector(playRecording:)];
        [dirView addSubview:playButton];

        NSButton *trashButton = [[NSButton alloc] initWithFrame:NSMakeRect(contentRect.size.width - 35, 0, 30, 30)];
        [trashButton setTitle:@"🗑"];
        [trashButton setTag:tag];
        [trashButton setTarget:self];
        [trashButton setAction:@selector(deleteRecording:)];
        [dirView addSubview:trashButton];

        yPosition += 30;
        tag++;
    }

    // Ensure the scrollContentView has the right height
    scrollContentView.frame = NSMakeRect(0, 0, contentRect.size.width, MAX(yPosition, scrollViewFrame.size.height));

    // Add the Close button at the bottom
    NSButton *closeButton = [[NSButton alloc] initWithFrame:NSMakeRect(10, 10, 80, 30)];
    [closeButton setTitle:@"Close"];
    [closeButton setTarget:self];
    [closeButton setAction:@selector(closeSheet)];
    [contentView addSubview:closeButton];
    
    [self.scrollView.contentView scrollToPoint:NSMakePoint(0, scrollContentView.frame.size.height - self.scrollView.contentView.bounds.size.height)];
    [self.scrollView reflectScrolledClipView:self.scrollView.contentView];
}


- (unsigned long long)directorySizeAtPath:(NSString *)path {
    unsigned long long size = 0;
    NSArray *contents = [fileManager contentsOfDirectoryAtPath:path error:nil];
    for (NSString *fileName in contents) {
        NSString *filePath = [path stringByAppendingPathComponent:fileName];
        NSDictionary *fileAttributes = [fileManager attributesOfItemAtPath:filePath error:nil];
        size += [fileAttributes fileSize];
    }
    return size;
}

- (void)playRecording:(NSButton *)sender {
    if (self.currentlyPlayingButton == sender) {
        [self stopPlayback];
        return;
    }

    [self stopPlayback];

    NSString *directoryPath = self.directoryPaths[sender.tag];
    NSString *wavFilePath = [self wavFilePathInDirectory:directoryPath];
    if (wavFilePath) {
        NSError *error;
        NSURL *fileURL = [NSURL fileURLWithPath:wavFilePath];
        audioPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:fileURL error:&error];
        if (audioPlayer) {
            [audioPlayer play];
            [sender setTitle:@"⏹️"];
            self.currentlyPlayingButton = sender;
        } else {
            NSLog(@"Error playing file: %@", error.localizedDescription);
        }
    }
}

- (void)stopPlayback {
    if (audioPlayer.isPlaying) {
        [audioPlayer stop];
    }
    [self.currentlyPlayingButton setTitle:@"▶️"];
    self.currentlyPlayingButton = nil;
}

- (void)deleteRecording:(NSButton *)sender {
    NSString *directoryPath = self.directoryPaths[sender.tag];
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Delete Recording"];
    [alert setInformativeText:@"Are you sure you want to delete this recording?"];
    [alert addButtonWithTitle:@"Delete"];
    [alert addButtonWithTitle:@"Cancel"];
    NSInteger response = [alert runModal];

    if (response == NSAlertFirstButtonReturn) {
        NSError *error;
        if ([fileManager removeItemAtPath:directoryPath error:&error]) {
            NSLog(@"Directory deleted successfully.");
            [self.directoryPaths removeObjectAtIndex:sender.tag];
            [self layoutScrollViewAndContents];
        } else {
            NSLog(@"Error deleting directory: %@", error.localizedDescription);
        }
    }
}

- (NSString *)wavFilePathInDirectory:(NSString *)directoryPath {
    NSArray *possibleFiles = @[@"L.wav", @"R.wav", @"L-R.wav", @"1.wav", @"2.wav", @"1-2.wav",@"3.wav", @"4.wav", @"3-4.wav",@"5.wav", @"6.wav", @"5-6.wav",@"7.wav", @"8.wav", @"7-8.wav"];
    for (NSString *fileName in possibleFiles) {
        NSString *filePath = [directoryPath stringByAppendingPathComponent:fileName];
        if ([fileManager fileExistsAtPath:filePath]) {
            return filePath;
        }
    }
    return nil;
}

- (void)closeSheet {
    [self.modalWindow.sheetParent endSheet:self.modalWindow];
}

@end

@implementation NSWindow (WithRecordingManager)

- (void)presentRecordingManager:(mpc::Mpc*)mpc {
    RecordingsViewController *vc = [[RecordingsViewController alloc] init];
    vc.mpc = mpc;

    NSWindow *modalWindow = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 500, 400)
                                                        styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable)
                                                          backing:NSBackingStoreBuffered
                                                            defer:NO];
    modalWindow.contentViewController = vc;
    modalWindow.title = @"Recording Manager";
    [modalWindow center];

    vc.modalWindow = modalWindow;

    NSWindow *mainWindow = [NSApp mainWindow];
    [mainWindow beginSheet:modalWindow completionHandler:^(NSModalResponse returnCode) {
        [modalWindow close];
    }];
}

@end

void doPresentRecordingManager(void* nativeWindowHandle, mpc::Mpc* mpc) {
    auto nsview = (NSView*) nativeWindowHandle;
    auto window = (NSWindow*) [nsview window];
    [window presentRecordingManager:mpc];
}

#endif
#endif
