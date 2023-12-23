#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE

#include "IosExportDocumentBrowser.h"

#include <CoreServices/UTCoreTypes.h>
#include <UIKit/UIKit.h>

@implementation UIWindow (DocumentBrowser)

-(void)presentShareOptions {
    NSString *tempDirectory = NSTemporaryDirectory();
     NSString *fooFilePath = [tempDirectory stringByAppendingPathComponent:@"foo.txt"];
     NSURL *fooFileURL = [NSURL fileURLWithPath:fooFilePath];
     
     // Write the binary data "Hello" to "foo.txt"
     NSData *fooData = [@"Hello" dataUsingEncoding:NSUTF8StringEncoding];
     [fooData writeToFile:fooFilePath atomically:YES];
     
     // Create a file URL for "bar.txt" in the temporary directory
     NSString *barFilePath = [tempDirectory stringByAppendingPathComponent:@"bar.txt"];
     NSURL *barFileURL = [NSURL fileURLWithPath:barFilePath];
     
     // Write the binary data "World!" to "bar.txt"
     NSData *barData = [@"World!" dataUsingEncoding:NSUTF8StringEncoding];
     [barData writeToFile:barFilePath atomically:YES];
     
     // Initialize filePathsArray and add the paths of foo.txt and bar.txt
     NSMutableArray *filePathsArray = [NSMutableArray arrayWithObjects:fooFilePath, barFilePath, nil];
    
    UIAlertController *alertController = [UIAlertController alertControllerWithTitle:@"Share Options" message:@"Choose an option to share" preferredStyle:UIAlertControllerStyleAlert];

    UIAlertAction *shareAllAction = [UIAlertAction actionWithTitle:@"Share APS and ALL of current project" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
        [self openActivityView:filePathsArray];
    }];

    UIAlertAction *shareSelectedAction = [UIAlertAction actionWithTitle:@"Share selected file or directory" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
        [self openActivityView:filePathsArray];
    }];

    [alertController addAction:shareAllAction];
    [alertController addAction:shareSelectedAction];

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

void doPresentShareOptions(void* nativeWindowHandle) {
  auto uiview = (UIView*) nativeWindowHandle;
  auto window = (UIWindow*)[uiview window];
  [window presentShareOptions];
}

#endif
#endif
