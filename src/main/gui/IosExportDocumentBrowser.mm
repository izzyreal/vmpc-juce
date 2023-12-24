#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE

#include <vector>

#include "IosExportDocumentBrowser.h"

#include <CoreServices/UTCoreTypes.h>
#include <UIKit/UIKit.h>

#include "Mpc.hpp"
#include "sampler/Sampler.hpp"
#include "file/aps/ApsParser.hpp"
#include "file/all/AllParser.hpp"
#include "file/sndwriter/SndWriter.hpp"
#include "lcdgui/screens/LoadScreen.hpp"
#include "disk/MpcFile.hpp"

@implementation UIWindow (DocumentBrowser)

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

- (NSMutableArray<NSString *> *)getSelectedFileOrDirectory:(mpc::Mpc *)mpc {
    const auto selectedFile = mpc->screens->get<mpc::lcdgui::screens::LoadScreen>("load")->getSelectedFile();
    const bool isDirectory = selectedFile->isDirectory();
    const auto selectedFilePath = selectedFile->getPath();
    
    NSMutableArray<NSString *> *filePathsArray = [NSMutableArray array];

    if (isDirectory) {
        for (const auto& entry : fs::recursive_directory_iterator(selectedFilePath)) {
            if (!entry.is_directory()) {
                NSString *filePath = [NSString stringWithUTF8String:entry.path().c_str()];
                [filePathsArray addObject:filePath];
            }
        }
    } else {
        NSString *filePath = [NSString stringWithUTF8String:selectedFilePath.c_str()];
        [filePathsArray addObject:filePath];
    }

    return filePathsArray;
}

-(void)presentShareOptions:(mpc::Mpc*)mpc {
    UIAlertController *alertController = [UIAlertController alertControllerWithTitle:@"Share Options" message:nil
                                                                      preferredStyle:UIAlertControllerStyleAlert];

    NSMutableArray<NSString *> *filePathsArray = [NSMutableArray array];

    UIAlertAction *shareApsSndsAllAction = [UIAlertAction actionWithTitle:@"Share APS, SNDs and ALL of current project" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {

        NSMutableArray<NSString *> *generatedFilePaths = [self writeApsAllAndSnd:mpc];
        [filePathsArray addObjectsFromArray:generatedFilePaths];

        [self openActivityView:filePathsArray];
    }];
    
    [alertController addAction:shareApsSndsAllAction];

    const auto selectedFile = mpc->screens->get<mpc::lcdgui::screens::LoadScreen>("load")->getSelectedFile();
    
    if (selectedFile) {
        const bool isDirectory = selectedFile->isDirectory();
        const std::string name = selectedFile->getName();

        NSString *nameNSString = [NSString stringWithUTF8String:name.c_str()];

        NSString *title = isDirectory ? [NSString stringWithFormat:@"Share selected directory (%@)", nameNSString]
                                      : [NSString stringWithFormat:@"Share selected file (%@)", nameNSString];

        UIAlertAction *shareSelectedAction = [UIAlertAction actionWithTitle:title style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
            NSMutableArray<NSString *> *generatedFilePaths = [self getSelectedFileOrDirectory:mpc];
            [filePathsArray addObjectsFromArray:generatedFilePaths];
            [self openActivityView:filePathsArray];
        }];
        
        [alertController addAction:shareSelectedAction];
    } else {
        UIAlertAction *noFileSelectedAction = [UIAlertAction actionWithTitle:@"Share selected file or directory" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
            UIAlertController *noFileAlertController = [UIAlertController alertControllerWithTitle:@"No selected file or directory available"
                                                                                           message:@"Select a file or directory in the LOAD screen to use this option."
                                                                                    preferredStyle:UIAlertControllerStyleAlert];

            UIAlertAction *okAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil];

            [noFileAlertController addAction:okAction];
            [self.rootViewController presentViewController:noFileAlertController animated:YES completion:nil];
        }];

        [alertController addAction:noFileSelectedAction];
    }
    
    UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:^(UIAlertAction *action){}];
    
    [alertController addAction:cancelAction];
    
    [self.rootViewController presentViewController:alertController animated:YES completion:nil];
}

-(void)openActivityView:(NSArray *)filePathsArray {
    // The rest of your code remains the same, starting from handling multiple files/directories
    NSMutableArray *itemsToShare = [[NSMutableArray alloc] init];
    NSFileManager *fileManager = [NSFileManager defaultManager];
    for (NSString *path in filePathsArray) {
        BOOL isDirectory;
        if ([fileManager fileExistsAtPath:path isDirectory:&isDirectory]) {
            if (isDirectory) {
                // If it's a directory, recursively add its contents
                NSDirectoryEnumerator *enumerator = [fileManager enumeratorAtPath:path];
                NSString *subpath;
                while (subpath = [enumerator nextObject]) {
                    NSString *fullSubpath = [path stringByAppendingPathComponent:subpath];
                    NSURL *fileURL = [NSURL fileURLWithPath:fullSubpath];
                    [itemsToShare addObject:fileURL];
                }
            } else {
                // If it's a file, just add it directly
                NSURL *fileURL = [NSURL fileURLWithPath:path];
                [itemsToShare addObject:fileURL];
            }
        }
    }
    
    // Initialize the activity view controller with the array of items
    UIActivityViewController *activityViewController = [[UIActivityViewController alloc] initWithActivityItems:itemsToShare applicationActivities:nil];
    
    // Configure the activity view controller for presentation on iPad
    activityViewController.modalPresentationStyle = UIModalPresentationPopover;
    UIPopoverPresentationController *presentationController = [activityViewController popoverPresentationController];
    presentationController.sourceView = self.rootViewController.view;
    presentationController.sourceRect = CGRectMake(self.rootViewController.view.bounds.size.width / 2, self.rootViewController.view.bounds.size.height / 4, 0, 0);
    
    // Present the activity view controller
    [activityViewController setCompletionWithItemsHandler:^(UIActivityType _Nullable activityType, BOOL completed, NSArray * _Nullable returnedItems, NSError * _Nullable activityError) {
        if (completed) {
            // Cleanup routine to delete all items in filePathsArray
            for (NSString *path in filePathsArray) {
                NSError *error;
                if (![fileManager removeItemAtPath:path error:&error]) {
                    NSLog(@"Failed to delete %@: %@", path, error);
                }
            }
        }
    }];
    
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
