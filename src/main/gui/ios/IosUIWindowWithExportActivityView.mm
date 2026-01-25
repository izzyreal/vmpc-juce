#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE

#include <vector>

#include <CoreServices/UTCoreTypes.h>
#include <UIKit/UIKit.h>

#include "Mpc.hpp"
#include "sampler/Sampler.hpp"
#include "file/aps/ApsParser.hpp"
#include "file/all/AllParser.hpp"
#include "file/sndwriter/SndWriter.hpp"
#include "lcdgui/screens/LoadScreen.hpp"
#include "disk/AbstractDisk.hpp"
#include "disk/MpcFile.hpp"

#include "miniz.h"

@implementation UIWindow (WithExportActivityView)

- (NSMutableArray<NSURL *> *)writeApsAllAndSnd:(mpc::Mpc *)mpc {
    
    NSURL *tempDirectoryURL = [NSURL fileURLWithPath:NSTemporaryDirectory() isDirectory:YES];
    NSMutableArray<NSURL *> *fileURLsArray = [NSMutableArray array];

    const std::vector<char>& apsData = mpc::file::aps::ApsParser(*mpc, "ALL_PGMS").saveBytes;
    NSData *data = [NSData dataWithBytes:apsData.data() length:apsData.size()];
    NSURL *apsDataFileURL = [tempDirectoryURL URLByAppendingPathComponent:@"ALL_PGMS.APS"];
    if ([data writeToURL:apsDataFileURL atomically:YES]) { [fileURLsArray addObject:apsDataFileURL]; }

    const std::vector<char>& allData = mpc::file::all::AllParser(*mpc).saveBytes;
    data = [NSData dataWithBytes:allData.data() length:allData.size()];
    NSURL *allDataFileURL = [tempDirectoryURL URLByAppendingPathComponent:@"ALL_SEQS.ALL"];
    if ([data writeToURL:allDataFileURL atomically:YES]) { [fileURLsArray addObject:allDataFileURL]; }

    for (auto& sound : mpc->getSampler()->getSounds()) {
        mpc::file::sndwriter::SndWriter sndWriter(sound.get());
        const std::vector<char>& sndData = sndWriter.getSndFileArray();
        
        data = [NSData dataWithBytes:sndData.data() length:sndData.size()];
        std::string soundName = sound->getName();
        NSString *soundNameNSString = [NSString stringWithUTF8String:soundName.c_str()];
        NSString *uppercasedSoundName = [soundNameNSString uppercaseString];
        NSString *sndFileName = [NSString stringWithFormat:@"%@.SND", uppercasedSoundName];
        NSURL *sndDataFileURL = [tempDirectoryURL URLByAppendingPathComponent:sndFileName];
        if ([data writeToURL:sndDataFileURL atomically:YES]) { [fileURLsArray addObject:sndDataFileURL]; }
    }
    
    return fileURLsArray;
}

- (void)createDirectoryZip:(const std::string &)selectedFilePath
          currentDirectory:(const std::string &)currentDirectory
             fileURLsArray:(NSMutableArray<NSURL *> *)fileURLsArray {

    NSURL *tempDirectoryURL = [NSURL fileURLWithPath:NSTemporaryDirectory() isDirectory:YES];
    NSString *zipFileName = [[NSString stringWithUTF8String:selectedFilePath.c_str()] lastPathComponent];
    NSURL *zipFileURL = [tempDirectoryURL URLByAppendingPathComponent:[zipFileName stringByAppendingString:@".zip"]];
    NSString *zipFilePath = zipFileURL.path;

    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    mz_bool status = mz_zip_writer_init_file(&zip_archive, zipFilePath.UTF8String, 0);

    if (!status) {
        NSLog(@"Failed to open zip archive for writing");
        return;
    }

    for (const auto &entry : fs::recursive_directory_iterator(selectedFilePath)) {
        if (!entry.is_directory()) {
            std::string relativePath = fs::relative(entry.path(), currentDirectory).string();
            mz_bool file_added = mz_zip_writer_add_file(&zip_archive, relativePath.c_str(), entry.path().c_str(), "", 0, MZ_BEST_COMPRESSION);
            if (!file_added) {
                NSLog(@"Failed to add file to zip archive: %s", entry.path().c_str());
                mz_zip_writer_end(&zip_archive);
                return;
            }
        }
    }

    mz_zip_writer_finalize_archive(&zip_archive);
    mz_zip_writer_end(&zip_archive);

    [fileURLsArray addObject:zipFileURL];
}

#include "miniz.h"

- (void)createRecordingZip:(fs::path)dirPath fileURLsArray:(NSMutableArray<NSURL *> *)fileURLsArray {

    mz_zip_archive zipArchive;
    memset(&zipArchive, 0, sizeof(zipArchive));

    NSURL *tempDirectoryURL = [NSURL fileURLWithPath:NSTemporaryDirectory() isDirectory:YES];
    NSString *zipFileName = [[NSString stringWithUTF8String:dirPath.filename().c_str()] stringByAppendingString:@".zip"];
    NSURL *zipFileURL = [tempDirectoryURL URLByAppendingPathComponent:zipFileName];
    NSString *zipFilePath = zipFileURL.path;

    mz_bool status = mz_zip_writer_init_file(&zipArchive, zipFilePath.UTF8String, 0);

    if (!status) {
        NSLog(@"Failed to initialize zip file");
        return;
    }

    for (const auto& entry : fs::directory_iterator(dirPath)) {
        if (!entry.is_directory()) {
            std::string filePath = entry.path().string();
            mz_zip_writer_add_file(&zipArchive, entry.path().filename().string().c_str(), filePath.c_str(), "", 0, MZ_BEST_COMPRESSION);
        }
    }

    mz_zip_writer_finalize_archive(&zipArchive);
    mz_zip_writer_end(&zipArchive);

    if (status) {
        [fileURLsArray addObject:zipFileURL];
    } else {
        NSLog(@"Failed to create zip file");
    }
}

- (NSMutableArray<NSURL *> *)getSelectedFileOrDirectory:(mpc::Mpc *)mpc {
    const auto selectedFile = mpc->screens->get<mpc::lcdgui::ScreenId::LoadScreen>()->getSelectedFile();
    const bool isDirectory = selectedFile->isDirectory();
    const auto selectedFilePath = selectedFile->getPath();
    const auto currentDirectory = mpc->getDisk()->getAbsolutePath();
    
    NSMutableArray<NSURL *> *fileURLsArray = [NSMutableArray array];

    if (isDirectory) {
        [self createDirectoryZip:selectedFilePath currentDirectory:currentDirectory fileURLsArray:fileURLsArray];
    } else {
        NSString *filePath = [NSString stringWithUTF8String:selectedFilePath.c_str()];
        [fileURLsArray addObject:[NSURL fileURLWithPath:filePath]];
    }

    return fileURLsArray;
}

-(UIAlertAction *)createShareApsSndsAllAction:(mpc::Mpc*)mpc fileURLsArray:(NSMutableArray<NSURL *> *)fileURLsArray {
    return [UIAlertAction actionWithTitle:@"Share APS, SNDs and ALL of current project" style:UIAlertActionStyleDefault
                                  handler:^(UIAlertAction * _Nonnull /* action */) {
        NSMutableArray<NSURL *> *generatedFileURLs = [self writeApsAllAndSnd:mpc];
        [fileURLsArray addObjectsFromArray:generatedFileURLs];

        const bool shouldCleanUpAfter = true;
        [self openActivityView:fileURLsArray shouldCleanUpAfter:shouldCleanUpAfter];
    }];
}

-(UIAlertAction *)createShareSelectedAction:(mpc::Mpc*)mpc fileURLsArray:(NSMutableArray<NSURL *> *)fileURLsArray {
    const auto selectedFile = mpc->screens->get<mpc::lcdgui::ScreenId::LoadScreen>()->getSelectedFile();
    bool isDirectory = selectedFile->isDirectory();
    std::string name = selectedFile->getName();

    NSString *nameNSString = [NSString stringWithUTF8String:name.c_str()];
    NSString *title = isDirectory ? [NSString stringWithFormat:@"Share selected directory (%@)", nameNSString]
                                  : [NSString stringWithFormat:@"Share selected file (%@)", nameNSString];

    return [UIAlertAction actionWithTitle:title style:UIAlertActionStyleDefault
                                  handler:^(UIAlertAction * _Nonnull /* action */) {
        NSMutableArray<NSURL *> *generatedFileURLs = [self getSelectedFileOrDirectory:mpc];
        [fileURLsArray addObjectsFromArray:generatedFileURLs];
        
        const bool shouldCleanUpAfter = isDirectory;
        [self openActivityView:fileURLsArray shouldCleanUpAfter:shouldCleanUpAfter];
    }];
}

-(UIAlertAction *)createShareDirectToDiskRecordingAction:(mpc::Mpc*)mpc fileURLsArray:(NSMutableArray<NSURL *> *)fileURLsArray {

    const fs::path directToDiskRecordingsDirectory = mpc->paths->getDocuments()->recordingsPath();

    UIAlertAction *action = [UIAlertAction actionWithTitle:@"Share Direct to Disk Recordings" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull /* action */) {
        
        UIAlertController *alertController = [UIAlertController alertControllerWithTitle:@"Recordings" message:@"Select a directory" preferredStyle:UIAlertControllerStyleAlert];

        for (const auto& entry : fs::directory_iterator(directToDiskRecordingsDirectory)) {
            if (entry.is_directory()) {
                const auto entryPath = entry.path();
                NSString *dirName = [NSString stringWithUTF8String:entryPath.filename().string().c_str()];
                [alertController addAction:[UIAlertAction actionWithTitle:dirName style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull /* action */) {
                    [self createRecordingZip:entryPath fileURLsArray:fileURLsArray];
                    const bool shouldCleanUpAfter = true;
                    [self openActivityView:fileURLsArray shouldCleanUpAfter:shouldCleanUpAfter];
                }]];
            }
        }

        [alertController addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:nil]];

        [self.rootViewController presentViewController:alertController animated:YES completion:nil];
    }];

    return action;
}


-(UIAlertAction *)createCancelAction {
    return [UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:^(UIAlertAction * /* action */){}];
}

-(UIAlertAction *)createNoFileSelectedAction {
    return [UIAlertAction actionWithTitle:@"Share selected file or directory"
                                    style:UIAlertActionStyleDefault
                                  handler:^(UIAlertAction * _Nonnull /* action */) {
        UIAlertController *noFileAlertController =
        [UIAlertController alertControllerWithTitle:@"No selected file or directory available"
                                            message:@"Select a file or directory in the LOAD screen to use this option."
                                     preferredStyle:UIAlertControllerStyleAlert];

        UIAlertAction *okAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil];
        [noFileAlertController addAction:okAction];
        [self.rootViewController presentViewController:noFileAlertController animated:YES completion:nil];
    }];
}

-(void)presentShareOptions:(mpc::Mpc*)mpc {
    UIAlertController *alertController = [UIAlertController alertControllerWithTitle:@"Share Options"
                                                                             message:nil
                                                                      preferredStyle:UIAlertControllerStyleAlert];

    NSMutableArray<NSURL *> *fileURLsArray = [NSMutableArray array];

    UIAlertAction *shareApsSndsAllAction = [self createShareApsSndsAllAction:mpc fileURLsArray:fileURLsArray];
    [alertController addAction:shareApsSndsAllAction];

    const auto selectedFile = mpc->screens->get<mpc::lcdgui::ScreenId::LoadScreen>()->getSelectedFile();
    
    if (selectedFile) {
        UIAlertAction *shareSelectedAction = [self createShareSelectedAction:mpc fileURLsArray:fileURLsArray];
        [alertController addAction:shareSelectedAction];
    } else {
        UIAlertAction *noFileSelectedAction = [self createNoFileSelectedAction];
        [alertController addAction:noFileSelectedAction];
    }
    
    UIAlertAction *shareDirectToDiskRecordingAction = [self createShareDirectToDiskRecordingAction:mpc fileURLsArray:fileURLsArray];

    [alertController addAction:shareDirectToDiskRecordingAction];
    
    [alertController addAction:[self createCancelAction]];
    
    [self.rootViewController presentViewController:alertController animated:YES completion:nil];
}


- (UIActivityViewControllerCompletionWithItemsHandler)createActivityViewCompletionHandler:(NSArray<NSURL *> *)fileURLsArray shouldCleanUpAfter:(BOOL)shouldCleanUpAfter {
    
    UIActivityViewControllerCompletionWithItemsHandler handler = ^(UIActivityType _Nullable /* activityType */, BOOL completed, NSArray * _Nullable /* returnedItems */, NSError * _Nullable /* activityError */) {
        
        NSFileManager *fileManager = [NSFileManager defaultManager];
        
        if (completed && shouldCleanUpAfter) {
            for (NSURL *url in fileURLsArray) {
                NSError *error = nil;
                if (![fileManager removeItemAtURL:url error:&error]) {
                    NSLog(@"Failed to delete %@: %@", url, error);
                }
            }
        }
    };
    
    return [handler copy];
}

- (UIActivityViewController *)createActivityViewController:(NSArray<NSURL *> *)itemsToShare shouldCleanUpAfter:(BOOL)shouldCleanUpAfter {
    
    UIActivityViewController *activityViewController = [[UIActivityViewController alloc] initWithActivityItems:itemsToShare applicationActivities:nil];

    activityViewController.modalPresentationStyle = UIModalPresentationPopover;
    UIPopoverPresentationController *presentationController = [activityViewController popoverPresentationController];
    presentationController.sourceView = self.rootViewController.view;
    presentationController.sourceRect = CGRectMake(self.rootViewController.view.bounds.size.width / 2, self.rootViewController.view.bounds.size.height / 4, 0, 0);

    activityViewController.completionWithItemsHandler = [self createActivityViewCompletionHandler:itemsToShare shouldCleanUpAfter:shouldCleanUpAfter];

    return activityViewController;
}

-(void)openActivityView:(NSArray<NSURL *> *)fileURLsArray shouldCleanUpAfter:(BOOL)shouldCleanUpAfter {
    
    NSMutableArray<NSURL *> *itemsToShare = [[NSMutableArray alloc] init];
    NSFileManager *fileManager = [NSFileManager defaultManager];

    for (NSURL *url in fileURLsArray) {
        if ([fileManager fileExistsAtPath:url.path]) {
            [itemsToShare addObject:url];
        }
    }

    UIActivityViewController *activityViewController = [self createActivityViewController:itemsToShare shouldCleanUpAfter:shouldCleanUpAfter];

    [self.rootViewController presentViewController:activityViewController animated:YES completion:nil];
}

@end

void doPresentShareOptions(void* nativeWindowHandle, mpc::Mpc* mpc) {
  auto uiview = (UIView*) nativeWindowHandle;
  auto window = (UIWindow*)[uiview window];
  [window presentShareOptions:mpc];
}

#endif
#endif
