#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_OSX

#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>
#include <AVFoundation/AVFoundation.h>

#include "Mpc.hpp"

@interface RecordingsViewController : NSViewController
@property (nonatomic) mpc::Mpc *mpc;
@property (nonatomic, strong) NSButton *currentlyPlayingButton;
@property (nonatomic, strong) NSScrollView *scrollView;
@property (nonatomic, strong) NSMutableArray<NSString *> *directoryPaths;
@end

@implementation RecordingsViewController {
    AVAudioPlayer *audioPlayer;
    NSFileManager *fileManager;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    self.directoryPaths = [NSMutableArray new];
    fileManager = [NSFileManager defaultManager];
    [self layoutScrollViewAndContents];
}

- (void)viewWillDisappear {
    [super viewWillDisappear];
    [self stopPlayback];
}

- (void)layoutScrollViewAndContents {
    NSView *contentView = self.view;
    [contentView.subviews makeObjectsPerformSelector:@selector(removeFromSuperview)];

    NSTextField *titleLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(0, contentView.frame.size.height - 40, contentView.frame.size.width, 30)];
    [titleLabel setStringValue:@"Recording Manager"];
    [titleLabel setBezeled:NO];
    [titleLabel setDrawsBackground:NO];
    [titleLabel setEditable:NO];
    [titleLabel setSelectable:NO];
    [titleLabel setAlignment:NSTextAlignmentCenter];
    [contentView addSubview:titleLabel];

    CGFloat margin = 30;
    NSRect scrollViewFrame = NSMakeRect(margin, 10, contentView.frame.size.width - 2 * margin, contentView.frame.size.height - 50);
    self.scrollView = [[NSScrollView alloc] initWithFrame:scrollViewFrame];
    self.scrollView.hasVerticalScroller = YES;

    NSView *scrollContentView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, scrollViewFrame.size.width, 0)];
    [self.scrollView setDocumentView:scrollContentView];
    [contentView addSubview:self.scrollView];

    [self.directoryPaths removeAllObjects];

    CGFloat yPosition = 10;
    int tag = 0;
    for (auto& p : fs::directory_iterator(self.mpc->paths->recordingsPath())) {
        NSString *directoryPath = [NSString stringWithUTF8String:p.path().c_str()];
        [self.directoryPaths addObject:directoryPath];

        NSView *dirView = [[NSView alloc] initWithFrame:NSMakeRect(0, yPosition, scrollViewFrame.size.width, 40)];
        [scrollContentView addSubview:dirView];

        NSTextField *dirLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(10, 10, scrollViewFrame.size.width - 140, 20)];
        [dirLabel setStringValue:[NSString stringWithUTF8String:p.path().filename().string().c_str()]];
        [dirLabel setBezeled:NO];
        [dirLabel setDrawsBackground:NO];
        [dirLabel setEditable:NO];
        [dirLabel setSelectable:NO];
        [dirView addSubview:dirLabel];

        unsigned long long dirSize = [self directorySizeAtPath:directoryPath];
        CGFloat dirSizeMB = (CGFloat)dirSize / (1024 * 1024);
        NSTextField *sizeLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(scrollViewFrame.size.width - 130, 10, 60, 20)];
        [sizeLabel setStringValue:[NSString stringWithFormat:@"%.2f MB", dirSizeMB]];
        [sizeLabel setBezeled:NO];
        [sizeLabel setDrawsBackground:NO];
        [sizeLabel setEditable:NO];
        [sizeLabel setSelectable:NO];
        [dirView addSubview:sizeLabel];

        NSButton *playButton = [[NSButton alloc] initWithFrame:NSMakeRect(scrollViewFrame.size.width - 70, 5, 30, 30)];
        [playButton setTitle:@"▶️"];
        [playButton setTag:tag];
        [playButton setTarget:self];
        [playButton setAction:@selector(playRecording:)];
        [dirView addSubview:playButton];

        NSButton *trashButton = [[NSButton alloc] initWithFrame:NSMakeRect(scrollViewFrame.size.width - 35, 5, 30, 30)];
        [trashButton setTitle:@"🗑"];
        [trashButton setTag:tag];
        [trashButton setTarget:self];
        [trashButton setAction:@selector(deleteRecording:)];
        [dirView addSubview:trashButton];

        yPosition += 50;
        tag++;
    }

    [scrollContentView setFrameSize:NSMakeSize(scrollViewFrame.size.width, yPosition)];
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
    NSArray *possibleFiles = @[@"L.wav", @"L-R.wav"];
    for (NSString *fileName in possibleFiles) {
        NSString *filePath = [directoryPath stringByAppendingPathComponent:fileName];
        if ([fileManager fileExistsAtPath:filePath]) {
            return filePath;
        }
    }
    return nil;
}

@end

@implementation NSWindow (WithRecordingManager)

- (void)presentRecordingManager:(mpc::Mpc*)mpc {
    NSViewController *vc = [[RecordingsViewController alloc] init];
    [(RecordingsViewController *)vc setMpc:mpc];

    NSWindow *window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 500, 400)
                                                   styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable)
                                                     backing:NSBackingStoreBuffered defer:NO];
    [window setContentViewController:vc];
    [window makeKeyAndOrderFront:nil];
}

@end

void doPresentRecordingManager(void* nativeWindowHandle, mpc::Mpc* mpc) {
    auto nsview = (NSView*) nativeWindowHandle;
    auto window = (NSWindow*) [nsview window];
    [window presentRecordingManager:mpc];
}

#endif
#endif
