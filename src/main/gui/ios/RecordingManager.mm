#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE

#include <Foundation/Foundation.h>
#include <UIKit/UIKit.h>
#include <AVFoundation/AVFoundation.h>

#include "Mpc.hpp"

@interface RecordingsViewController : UIViewController
@property (nonatomic) mpc::Mpc *mpc;
@property (nonatomic, strong) UIButton *currentlyPlayingButton;
@property (nonatomic, strong) UIScrollView *scrollView;
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
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    [self layoutScrollViewAndContents];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    [self stopPlayback];
}

- (void)layoutScrollViewAndContents {
    [self.scrollView removeFromSuperview];
    self.scrollView = nil;
    

    // Title Label for Header
    UILabel *titleLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, self.view.bounds.size.width, 40)];
    titleLabel.text = @"Recording Manager";
    titleLabel.textAlignment = NSTextAlignmentCenter;
    [self.view addSubview:titleLabel];

    CGFloat margin = 30;
    CGRect scrollViewFrame = CGRectMake(margin - 5, titleLabel.frame.size.height, self.view.bounds.size.width - 2 * margin, self.view.bounds.size.height - titleLabel.frame.size.height);
    self.scrollView = [[UIScrollView alloc] initWithFrame:scrollViewFrame];
    [self.view addSubview:self.scrollView];

    CGFloat buttonWidth = 30;
    CGFloat sizeLabelWidth = 3 * buttonWidth;
    CGFloat spacing = 5;
    CGFloat labelMaxWidth = scrollViewFrame.size.width - 2 * buttonWidth - sizeLabelWidth - 4 * spacing;

    [self.directoryPaths removeAllObjects];
    int yPosition = 20;
    int tag = 0;
    for (auto& p : fs::directory_iterator(self.mpc->paths->recordingsPath())) {
        NSString *directoryPath = [NSString stringWithUTF8String:p.path().c_str()];
        [self.directoryPaths addObject:directoryPath];

        UIView *dirView = [[UIView alloc] initWithFrame:CGRectMake(0, yPosition, scrollViewFrame.size.width, 40)];
        [self.scrollView addSubview:dirView];

        UILabel *dirLabel = [[UILabel alloc] initWithFrame:CGRectMake(10, 10, labelMaxWidth, 20)];
        dirLabel.text = [NSString stringWithUTF8String:p.path().filename().string().c_str()];
        dirLabel.lineBreakMode = NSLineBreakByTruncatingTail;
        [dirView addSubview:dirLabel];

        unsigned long long dirSize = [self directorySizeAtPath:directoryPath];
        CGFloat dirSizeMB = (CGFloat)dirSize / (1024 * 1024);
        UILabel *sizeLabel = [[UILabel alloc] initWithFrame:CGRectMake(CGRectGetMaxX(dirLabel.frame) + spacing, 10, sizeLabelWidth, 20)];
        sizeLabel.text = [NSString stringWithFormat:@"%.2f MB", dirSizeMB];
        sizeLabel.textAlignment = NSTextAlignmentRight;
        [dirView addSubview:sizeLabel];
        
        if (@available(iOS 13.0, *)) {
            dirLabel.textColor = [UIColor labelColor];
            sizeLabel.textColor = [UIColor labelColor];
        } else {
            dirLabel.textColor = [UIColor blackColor];
            sizeLabel.textColor = [UIColor blackColor];
        }

        UIButton *playButton = [UIButton buttonWithType:UIButtonTypeSystem];
        playButton.frame = CGRectMake(CGRectGetMaxX(sizeLabel.frame) + spacing, 5, buttonWidth, 30);
        [playButton setTitle:@"▶️" forState:UIControlStateNormal];
        playButton.tag = tag;
        [playButton addTarget:self action:@selector(playRecording:) forControlEvents:UIControlEventTouchUpInside];
        [dirView addSubview:playButton];

        UIButton *trashButton = [UIButton buttonWithType:UIButtonTypeSystem];
        trashButton.frame = CGRectMake(CGRectGetMaxX(playButton.frame) + spacing, 5, buttonWidth, 30);
        [trashButton setTitle:@"🗑" forState:UIControlStateNormal];
        trashButton.tag = tag;
        [trashButton addTarget:self action:@selector(deleteRecording:) forControlEvents:UIControlEventTouchUpInside];
        [dirView addSubview:trashButton];

        yPosition += 50;
        tag++;
    }
    
    
    if (@available(iOS 13.0, *)) {
        self.view.backgroundColor = [UIColor systemBackgroundColor];
        titleLabel.backgroundColor = [UIColor secondarySystemBackgroundColor];
        titleLabel.textColor = [UIColor labelColor];
    } else {
        self.view.backgroundColor = [UIColor whiteColor];
        titleLabel.backgroundColor = [UIColor lightGrayColor];
        titleLabel.textColor = [UIColor blackColor];
    }

    self.scrollView.contentSize = CGSizeMake(scrollViewFrame.size.width, yPosition);
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

- (void)playRecording:(UIButton *)sender {
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
            [sender setTitle:@"⏹️" forState:UIControlStateNormal];
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
    [self.currentlyPlayingButton setTitle:@"▶️" forState:UIControlStateNormal];
    self.currentlyPlayingButton = nil;
}

- (void)deleteRecording:(UIButton *)sender {
    NSString *directoryPath = self.directoryPaths[sender.tag];
    NSError *error;
    if ([fileManager removeItemAtPath:directoryPath error:&error]) {
        NSLog(@"Directory deleted successfully.");
        [self.directoryPaths removeObjectAtIndex:sender.tag];
        [self layoutScrollViewAndContents];
    } else {
        NSLog(@"Error deleting directory: %@", error.localizedDescription);
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

@end

@implementation UIWindow (WithRecordingManager)

- (void)presentRecordingManager:(mpc::Mpc*)mpc {
    RecordingsViewController *recordingsVC = [[RecordingsViewController alloc] init];
    recordingsVC.mpc = mpc;
    [self.rootViewController presentViewController:recordingsVC animated:YES completion:nil];
}

@end

void doPresentRecordingManager(void* nativeWindowHandle, mpc::Mpc* mpc) {
  auto uiview = (UIView*) nativeWindowHandle;
  auto window = (UIWindow*)[uiview window];
  [window presentRecordingManager:mpc];
}

#endif
#endif
