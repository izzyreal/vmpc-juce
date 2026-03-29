#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE

#include <vector>

#include <CoreServices/UTCoreTypes.h>
#include <UIKit/UIKit.h>

#include "Logger.hpp"
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

- (void)showShareFailedAlert:(NSString *)message {
    UIAlertController *alertController =
        [UIAlertController alertControllerWithTitle:@"Share failed"
                                            message:message
                                     preferredStyle:UIAlertControllerStyleAlert];
    [alertController addAction:[UIAlertAction actionWithTitle:@"OK"
                                                        style:UIAlertActionStyleDefault
                                                      handler:nil]];
    [self.rootViewController presentViewController:alertController
                                          animated:YES
                                        completion:nil];
}

- (void)removeTemporaryURLs:(NSArray<NSURL *> *)fileURLsArray {
    NSFileManager *fileManager = [NSFileManager defaultManager];
    for (NSURL *url in fileURLsArray) {
        NSError *error = nil;
        if (![fileManager removeItemAtURL:url error:&error] &&
            error != nil &&
            error.code != NSFileNoSuchFileError) {
            MLOG("iOS share cleanup failed for '" +
                 std::string(url.path.UTF8String) + "'");
        }
    }
}

- (BOOL)writeData:(NSData *)data
            toURL:(NSURL *)fileURL
      description:(NSString *)description
         produced:(NSMutableArray<NSURL *> *)fileURLsArray {
    if ([data writeToURL:fileURL atomically:YES]) {
        [fileURLsArray addObject:fileURL];
        return YES;
    }

    MLOG("Required file I/O failed during iOS share export write for '" +
         std::string(fileURL.path.UTF8String) + "'");
    [self removeTemporaryURLs:fileURLsArray];
    [self showShareFailedAlert:[NSString stringWithFormat:@"Could not write %@.", description]];
    return NO;
}

- (NSMutableArray<NSURL *> *)writeApsAllAndSnd:(mpc::Mpc *)mpc {
    
    NSURL *tempDirectoryURL = [NSURL fileURLWithPath:NSTemporaryDirectory() isDirectory:YES];
    NSMutableArray<NSURL *> *fileURLsArray = [NSMutableArray array];

    const std::vector<char>& apsData = mpc::file::aps::ApsParser(*mpc, "ALL_PGMS").saveBytes;
    NSData *data = [NSData dataWithBytes:apsData.data() length:apsData.size()];
    NSURL *apsDataFileURL = [tempDirectoryURL URLByAppendingPathComponent:@"ALL_PGMS.APS"];
    if (![self writeData:data toURL:apsDataFileURL description:@"ALL_PGMS.APS" produced:fileURLsArray]) { return nil; }

    const std::vector<char>& allData = mpc::file::all::AllParser(*mpc).saveBytes;
    data = [NSData dataWithBytes:allData.data() length:allData.size()];
    NSURL *allDataFileURL = [tempDirectoryURL URLByAppendingPathComponent:@"ALL_SEQS.ALL"];
    if (![self writeData:data toURL:allDataFileURL description:@"ALL_SEQS.ALL" produced:fileURLsArray]) { return nil; }

    for (auto& sound : mpc->getSampler()->getSounds()) {
        mpc::file::sndwriter::SndWriter sndWriter(sound.get());
        const std::vector<char>& sndData = sndWriter.getSndFileArray();
        
        data = [NSData dataWithBytes:sndData.data() length:sndData.size()];
        std::string soundName = sound->getName();
        NSString *soundNameNSString = [NSString stringWithUTF8String:soundName.c_str()];
        NSString *uppercasedSoundName = [soundNameNSString uppercaseString];
        NSString *sndFileName = [NSString stringWithFormat:@"%@.SND", uppercasedSoundName];
        NSURL *sndDataFileURL = [tempDirectoryURL URLByAppendingPathComponent:sndFileName];
        if (![self writeData:data toURL:sndDataFileURL description:sndFileName produced:fileURLsArray]) { return nil; }
    }
    
    return fileURLsArray;
}

- (NSURL *)createDirectoryZip:(const std::string &)selectedFilePath
             currentDirectory:(const std::string &)currentDirectory {

    NSURL *tempDirectoryURL = [NSURL fileURLWithPath:NSTemporaryDirectory() isDirectory:YES];
    NSString *zipFileName = [[NSString stringWithUTF8String:selectedFilePath.c_str()] lastPathComponent];
    NSURL *zipFileURL = [tempDirectoryURL URLByAppendingPathComponent:[zipFileName stringByAppendingString:@".zip"]];
    NSString *zipFilePath = zipFileURL.path;

    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    mz_bool status = mz_zip_writer_init_file(&zip_archive, zipFilePath.UTF8String, 0);

    if (!status) {
        MLOG("Required file I/O failed during iOS directory share zip initialization for '" +
             selectedFilePath + "'");
        [self showShareFailedAlert:@"Could not create zip archive."];
        return nil;
    }

    const auto recursiveDirItRes = mpc_fs::make_recursive_directory_iterator(selectedFilePath);
    if (!recursiveDirItRes) {
        MLOG("Required file I/O failed during iOS directory share enumeration for '" +
             selectedFilePath + "'");
        mz_zip_writer_end(&zip_archive);
        [self showShareFailedAlert:@"Could not enumerate selected directory."];
        return nil;
    }

    for (auto entry = *recursiveDirItRes; entry != mpc_fs::recursive_directory_end(); ++entry) {
        if (!entry->is_directory()) {
            const auto relativePathRes = mpc_fs::relative(entry->path(), currentDirectory);
            if (!relativePathRes) {
                MLOG("Required file I/O failed during iOS directory share relative-path resolution for '" +
                     entry->path().string() + "'");
                mz_zip_writer_end(&zip_archive);
                [self showShareFailedAlert:@"Could not prepare selected directory for sharing."];
                return nil;
            }
            std::string relativePath = relativePathRes->string();
            mz_bool file_added = mz_zip_writer_add_file(&zip_archive, relativePath.c_str(), entry->path().c_str(), "", 0, MZ_BEST_COMPRESSION);
            if (!file_added) {
                MLOG("Required file I/O failed during iOS directory share zip write for '" +
                     entry->path().string() + "'");
                mz_zip_writer_end(&zip_archive);
                [self showShareFailedAlert:@"Could not add a file to the zip archive."];
                return nil;
            }
        }
    }

    if (!mz_zip_writer_finalize_archive(&zip_archive)) {
        MLOG("Required file I/O failed during iOS directory share zip finalization for '" +
             selectedFilePath + "'");
        mz_zip_writer_end(&zip_archive);
        [self showShareFailedAlert:@"Could not finalize zip archive."];
        return nil;
    }
    mz_zip_writer_end(&zip_archive);

    return zipFileURL;
}

#include "miniz.h"

- (NSURL *)createRecordingZip:(mpc_fs::path)dirPath {

    mz_zip_archive zipArchive;
    memset(&zipArchive, 0, sizeof(zipArchive));

    NSURL *tempDirectoryURL = [NSURL fileURLWithPath:NSTemporaryDirectory() isDirectory:YES];
    NSString *zipFileName = [[NSString stringWithUTF8String:dirPath.filename().c_str()] stringByAppendingString:@".zip"];
    NSURL *zipFileURL = [tempDirectoryURL URLByAppendingPathComponent:zipFileName];
    NSString *zipFilePath = zipFileURL.path;

    mz_bool status = mz_zip_writer_init_file(&zipArchive, zipFilePath.UTF8String, 0);

    if (!status) {
        MLOG("Required file I/O failed during iOS recording share zip initialization for '" +
             dirPath.string() + "'");
        [self showShareFailedAlert:@"Could not create recording zip archive."];
        return nil;
    }

    const auto dirItRes = mpc_fs::make_directory_iterator(dirPath);
    if (!dirItRes) {
        MLOG("Required file I/O failed during iOS recording share enumeration for '" +
             dirPath.string() + "'");
        mz_zip_writer_end(&zipArchive);
        [self showShareFailedAlert:@"Could not enumerate recording directory."];
        return nil;
    }

    for (auto entry = *dirItRes; entry != mpc_fs::directory_end(); ++entry) {
        if (!entry->is_directory()) {
            std::string filePath = entry->path().string();
            if (!mz_zip_writer_add_file(&zipArchive,
                                        entry->path().filename().string().c_str(),
                                        filePath.c_str(), "", 0,
                                        MZ_BEST_COMPRESSION)) {
                MLOG("Required file I/O failed during iOS recording share zip write for '" +
                     entry->path().string() + "'");
                mz_zip_writer_end(&zipArchive);
                [self showShareFailedAlert:@"Could not add a recording file to the zip archive."];
                return nil;
            }
        }
    }

    if (!mz_zip_writer_finalize_archive(&zipArchive)) {
        MLOG("Required file I/O failed during iOS recording share zip finalization for '" +
             dirPath.string() + "'");
        mz_zip_writer_end(&zipArchive);
        [self showShareFailedAlert:@"Could not finalize recording zip archive."];
        return nil;
    }
    mz_zip_writer_end(&zipArchive);

    return zipFileURL;
}

- (NSMutableArray<NSURL *> *)getSelectedFileOrDirectory:(mpc::Mpc *)mpc {
    const auto selectedFile = mpc->screens->get<mpc::lcdgui::ScreenId::LoadScreen>()->getSelectedFile();
    const bool isDirectory = selectedFile->isDirectory();
    const auto selectedFilePath = selectedFile->getPath();
    const auto currentDirectory = mpc->getDisk()->getAbsolutePath();
    
    NSMutableArray<NSURL *> *fileURLsArray = [NSMutableArray array];

    if (isDirectory) {
        NSURL *zipFileURL =
            [self createDirectoryZip:selectedFilePath currentDirectory:currentDirectory];
        if (zipFileURL == nil) {
            return nil;
        }
        [fileURLsArray addObject:zipFileURL];
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
        if (generatedFileURLs == nil || generatedFileURLs.count == 0) {
            return;
        }
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
        if (generatedFileURLs == nil || generatedFileURLs.count == 0) {
            return;
        }
        [fileURLsArray addObjectsFromArray:generatedFileURLs];
        
        const bool shouldCleanUpAfter = isDirectory;
        [self openActivityView:fileURLsArray shouldCleanUpAfter:shouldCleanUpAfter];
    }];
}

-(UIAlertAction *)createShareDirectToDiskRecordingAction:(mpc::Mpc*)mpc fileURLsArray:(NSMutableArray<NSURL *> *)fileURLsArray {

    const mpc_fs::path directToDiskRecordingsDirectory = mpc->paths->getDocuments()->recordingsPath();

    UIAlertAction *action = [UIAlertAction actionWithTitle:@"Share Direct to Disk Recordings" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull /* action */) {
        
        UIAlertController *alertController = [UIAlertController alertControllerWithTitle:@"Recordings" message:@"Select a directory" preferredStyle:UIAlertControllerStyleAlert];

        const auto dirItRes = mpc_fs::make_directory_iterator(directToDiskRecordingsDirectory);
        if (!dirItRes) {
            MLOG("Required file I/O failed during iOS recording share directory enumeration for '" +
                 directToDiskRecordingsDirectory.string() + "'");
            [self showShareFailedAlert:@"Could not inspect the Direct to Disk recordings directory."];
            return;
        }

        for (auto entry = *dirItRes; entry != mpc_fs::directory_end(); ++entry) {
            if (entry->is_directory()) {
                const auto entryPath = entry->path();
                NSString *dirName = [NSString stringWithUTF8String:entryPath.filename().string().c_str()];
                [alertController addAction:[UIAlertAction actionWithTitle:dirName style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull /* action */) {
                    NSURL *zipFileURL = [self createRecordingZip:entryPath];
                    if (zipFileURL == nil) {
                        return;
                    }
                    [fileURLsArray addObject:zipFileURL];
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

    if (itemsToShare.count == 0) {
        [self showShareFailedAlert:@"Could not prepare any files for sharing."];
        return;
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
