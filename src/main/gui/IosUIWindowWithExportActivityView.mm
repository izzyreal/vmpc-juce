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

- (NSMutableArray<NSString *> *)writeApsAllAndSnd:(mpc::Mpc *)mpc {
    
    NSString *tempDirectory = NSTemporaryDirectory();
    NSMutableArray<NSString *> *filePathsArray = [NSMutableArray array];

    const std::vector<char>& apsData = mpc::file::aps::ApsParser(*mpc, "ALL_PGMS").saveBytes;
    NSData *data = [NSData dataWithBytes:apsData.data() length:apsData.size()];
    NSString *apsDataFilePath = [tempDirectory stringByAppendingPathComponent:@"ALL_PGMS.APS"];
    if ([data writeToFile:apsDataFilePath atomically:YES]) { [filePathsArray addObject:apsDataFilePath]; }

    const std::vector<char>& allData = mpc::file::all::AllParser(*mpc).saveBytes;
    data = [NSData dataWithBytes:allData.data() length:allData.size()];
    NSString *allDataFilePath = [tempDirectory stringByAppendingPathComponent:@"ALL_SEQS.ALL"];
    if ([data writeToFile:allDataFilePath atomically:YES]) { [filePathsArray addObject:allDataFilePath]; }

    for (auto& sound : mpc->getSampler()->getSounds()) {
        
        const std::vector<char>& sndData = mpc::file::sndwriter::SndWriter(sound.get()).getSndFileArray();
        
        data = [NSData dataWithBytes:sndData.data() length:sndData.size()];
        std::string soundName = sound->getName();
        NSString *soundNameNSString = [NSString stringWithUTF8String:soundName.c_str()];
        NSString *uppercasedSoundName = [soundNameNSString uppercaseString];
        NSString *sndDataFilePath = [tempDirectory stringByAppendingPathComponent:[NSString stringWithFormat:@"%@.SND", uppercasedSoundName]];
        if ([data writeToFile:sndDataFilePath atomically:YES]) { [filePathsArray addObject:sndDataFilePath]; }
    }
    
    return filePathsArray;
}

- (void)createDirectoryZip:(const std::string &)selectedFilePath currentDirectory:(const std::string &)currentDirectory
            filePathsArray:(NSMutableArray<NSString *> *)filePathsArray {
    NSString *tempDirectory = NSTemporaryDirectory();
    NSString *zipFileName = [[NSString stringWithUTF8String:selectedFilePath.c_str()] lastPathComponent];
    NSString *zipFilePath = [tempDirectory stringByAppendingPathComponent:[zipFileName stringByAppendingString:@".zip"]];

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

    [filePathsArray addObject:zipFilePath];
}

- (NSMutableArray<NSString *> *)getSelectedFileOrDirectory:(mpc::Mpc *)mpc {
    const auto selectedFile = mpc->screens->get<mpc::lcdgui::screens::LoadScreen>("load")->getSelectedFile();
    const bool isDirectory = selectedFile->isDirectory();
    const auto selectedFilePath = selectedFile->getPath();
    const auto currentDirectory = mpc->getDisk()->getAbsolutePath();
    
    NSMutableArray<NSString *> *filePathsArray = [NSMutableArray array];

    if (isDirectory) {
        [self createDirectoryZip:selectedFilePath currentDirectory:currentDirectory filePathsArray:filePathsArray];
    } else {
        NSString *filePath = [NSString stringWithUTF8String:selectedFilePath.c_str()];
        [filePathsArray addObject:filePath];
    }

    return filePathsArray;
}

-(UIAlertAction *)createShareApsSndsAllAction:
    (mpc::Mpc*)mpc filePathsArray:(NSMutableArray<NSString *> *)filePathsArray {
    return [UIAlertAction actionWithTitle:@"Share APS, SNDs and ALL of current project" style:UIAlertActionStyleDefault
                                  handler:^(UIAlertAction * _Nonnull /* action */) {
        NSMutableArray<NSString *> *generatedFilePaths = [self writeApsAllAndSnd:mpc];
        [filePathsArray addObjectsFromArray:generatedFilePaths];

        const bool shouldCleanUpAfter = true;
        [self openActivityView:filePathsArray shouldCleanUpAfter:shouldCleanUpAfter];
    }];
}

-(UIAlertAction *)createShareSelectedAction:(mpc::Mpc*)mpc filePathsArray:(NSMutableArray<NSString *> *)filePathsArray {
    const auto selectedFile = mpc->screens->get<mpc::lcdgui::screens::LoadScreen>("load")->getSelectedFile();
    bool isDirectory = selectedFile->isDirectory();
    std::string name = selectedFile->getName();

    NSString *nameNSString = [NSString stringWithUTF8String:name.c_str()];
    NSString *title = isDirectory ? [NSString stringWithFormat:@"Share selected directory (%@)", nameNSString]
                                  : [NSString stringWithFormat:@"Share selected file (%@)", nameNSString];

    return [UIAlertAction actionWithTitle:title style:UIAlertActionStyleDefault
                                  handler:^(UIAlertAction * _Nonnull /* action */) {
        NSMutableArray<NSString *> *generatedFilePaths = [self getSelectedFileOrDirectory:mpc];
        [filePathsArray addObjectsFromArray:generatedFilePaths];
        
        const bool shouldCleanUpAfter = isDirectory;
        [self openActivityView:filePathsArray shouldCleanUpAfter:shouldCleanUpAfter];
    }];
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

    NSMutableArray<NSString *> *filePathsArray = [NSMutableArray array];

    UIAlertAction *shareApsSndsAllAction = [self createShareApsSndsAllAction:mpc filePathsArray:filePathsArray];
    [alertController addAction:shareApsSndsAllAction];

    const auto selectedFile = mpc->screens->get<mpc::lcdgui::screens::LoadScreen>("load")->getSelectedFile();
    
    if (selectedFile) {
        UIAlertAction *shareSelectedAction = [self createShareSelectedAction:mpc filePathsArray:filePathsArray];
        [alertController addAction:shareSelectedAction];
    } else {
        UIAlertAction *noFileSelectedAction = [self createNoFileSelectedAction];
        [alertController addAction:noFileSelectedAction];
    }
    
    UIAlertAction *cancelAction = [self createCancelAction];
    [alertController addAction:cancelAction];
    
    [self.rootViewController presentViewController:alertController animated:YES completion:nil];
}


- (UIActivityViewControllerCompletionWithItemsHandler)createActivityViewCompletionHandler:(NSArray<NSString *> *)filePathsArray shouldCleanUpAfter:(BOOL)shouldCleanUpAfter {
    
    UIActivityViewControllerCompletionWithItemsHandler handler = ^(UIActivityType _Nullable /* activityType */, BOOL completed, NSArray * _Nullable /* returnedItems */, NSError * _Nullable /* activityError */) {
        
        NSFileManager *fileManager = [NSFileManager defaultManager];
        
        if (completed && shouldCleanUpAfter) {
            for (NSString *path in filePathsArray) {
                NSError *error;
                if (![fileManager removeItemAtPath:path error:&error]) {
                    NSLog(@"Failed to delete %@: %@", path, error);
                }
            }
        }
    };
    
    return [handler copy];
}

- (UIActivityViewController *)createActivityViewController:(NSArray *)itemsToShare shouldCleanUpAfter:(BOOL)shouldCleanUpAfter {
    
    UIActivityViewController *activityViewController = [[UIActivityViewController alloc] initWithActivityItems:itemsToShare applicationActivities:nil];

    activityViewController.modalPresentationStyle = UIModalPresentationPopover;
    UIPopoverPresentationController *presentationController = [activityViewController popoverPresentationController];
    presentationController.sourceView = self.rootViewController.view;
    presentationController.sourceRect = CGRectMake(self.rootViewController.view.bounds.size.width / 2, self.rootViewController.view.bounds.size.height / 4, 0, 0);

    activityViewController.completionWithItemsHandler = [self createActivityViewCompletionHandler:itemsToShare shouldCleanUpAfter:shouldCleanUpAfter];

    return activityViewController;
}

-(void)openActivityView:(NSArray *)filePathsArray shouldCleanUpAfter:(BOOL)shouldCleanUpAfter {
    
    NSMutableArray *itemsToShare = [[NSMutableArray alloc] init];
    NSFileManager *fileManager = [NSFileManager defaultManager];

    for (NSString *path in filePathsArray) {
        if ([fileManager fileExistsAtPath:path]) {
            NSURL *fileURL = [NSURL fileURLWithPath:path];
            [itemsToShare addObject:fileURL];
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
