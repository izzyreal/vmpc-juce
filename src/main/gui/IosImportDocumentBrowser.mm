#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE

#include "ImportDocumentUrlProcessor.h"

#include <CoreServices/UTCoreTypes.h>
#include <UIKit/UIKit.h>

/* ------------- */

@interface MyDelegate<UIDocumentBrowserViewControllerDelegate> : NSObject

@property (assign) ImportDocumentUrlProcessor* urlProcessor;
@property (assign) UIProgressView* progressView;
@property (assign) UIViewController* controller;
@property (assign) UIAlertController* alert;
@property (assign) BOOL overwriteNone;
@property (assign) BOOL overwriteAll;

@end

/* ------------- */

@implementation MyDelegate

- (void)showCopyProgressView {
  dispatch_sync(dispatch_get_main_queue(), ^{
    if (!_progressView) {
      [self setProgressView:[[UIProgressView alloc] initWithProgressViewStyle:UIProgressViewStyleDefault]];
      [self progressView].progressTintColor = [UIColor colorWithRed:187.0/255 green:160.0/255 blue:209.0/255 alpha:1.0];
      auto size = [[_controller view]bounds].size;
      [[[self progressView] layer]setFrame:CGRectMake(0, (size.height / 2) - 100, size.width, 200)];
      [[[self progressView] layer]setBorderColor:[UIColor lightGrayColor].CGColor];
      [self progressView].trackTintColor = [UIColor clearColor];
      [[self progressView] setProgress:0.0];
      
      [[[self progressView] layer]setCornerRadius:10];
      [[[self progressView] layer]setBorderWidth:5];
      [[[self progressView] layer]setMasksToBounds:TRUE];
      [self progressView].clipsToBounds = YES;
      
      [[_controller view] addSubview:[self progressView]];
      [[_controller view] bringSubviewToFront:[self progressView]];
      [[_controller view] layoutIfNeeded];
    }
  });
}

- (void)updateProgressView:(float)progress {
  dispatch_async(dispatch_get_main_queue(), ^{
    [[self progressView] setProgress:progress];
  });
}

- (void)handleDir:(NSURL*)url relativeDir:(NSString*)relativeDir {
  NSFileManager *fileManager = [NSFileManager defaultManager];
  NSArray<NSURL*>* urls = [fileManager contentsOfDirectoryAtURL:url includingPropertiesForKeys:[NSArray arrayWithObjects:NSURLNameKey, NSURLIsDirectoryKey, NSURLContentModificationDateKey, nil] options:NSDirectoryEnumerationSkipsHiddenFiles error:nil];
  
  if (urls != nil) {
    [self handleURLs:urls relativeDir:relativeDir];
  }
}

- (BOOL)showOverwriteAlert:(NSString*)relativeFileName {
    
  NSString* msg = [NSString stringWithFormat:@"File %@ exists. Overwrite?", relativeFileName];

  __block BOOL shouldOverwrite = false;
  
  dispatch_sync(dispatch_get_main_queue(), ^{
    
    
    [self setAlert:[UIAlertController alertControllerWithTitle:@"File exists"
                                                       message:msg
                                                preferredStyle:UIAlertControllerStyleAlert]];

    UIAlertAction* noneAction = [UIAlertAction actionWithTitle:@"None" style:UIAlertActionStyleDefault
                                                       handler:^(UIAlertAction*) { shouldOverwrite = false; [self setOverwriteNone:true]; _alert = nil; }];
    
    UIAlertAction* allAction = [UIAlertAction actionWithTitle:@"All" style:UIAlertActionStyleDefault
                                                      handler:^(UIAlertAction*) { shouldOverwrite = true; [self setOverwriteAll:true]; _alert = nil; }];
    
    UIAlertAction* noAction = [UIAlertAction actionWithTitle:@"No" style:UIAlertActionStyleDefault
       handler:^(UIAlertAction*) { shouldOverwrite = false; _alert = nil; }];

    UIAlertAction* yesAction = [UIAlertAction actionWithTitle:@"Yes" style:UIAlertActionStyleDefault
       handler:^(UIAlertAction*) { shouldOverwrite = true; _alert = nil; }];
     
    [_alert addAction:noneAction];
    [_alert addAction:allAction];
    [_alert addAction:noAction];
    [_alert addAction:yesAction];
    [[self controller] presentViewController:_alert animated:NO completion:nil];
  });
  
  while (_alert) {
    usleep(100000);
  }
  
  return shouldOverwrite;
}

- (void)handleFile:(NSURL*)url relativeDir:(NSString*)relativeDir {
  NSArray<NSString*>* allowedExtensions = @[@"wav",@"snd",@"aps",@"pgm",@"all",@"mid"];
  
  if (![allowedExtensions containsObject: (url ? [url pathExtension].lowercaseString : @"")])
  {
    return;
  }
  
  auto filename = [url lastPathComponent];

  if (_urlProcessor->destinationExists(filename.UTF8String, relativeDir.UTF8String))
  {
    NSString* relativeFileName = [NSString stringWithFormat:@"%@%@", relativeDir, filename];
    
    if ([self overwriteNone]) {
      return;
    }
    
    if (![self overwriteAll] && ![self showOverwriteAlert:relativeFileName]) {
      return;
    }
  }
  
  __block NSData *data = nil;
  
  NSFileCoordinator *coordinator = [[NSFileCoordinator alloc] initWithFilePresenter:nil];
  [coordinator coordinateReadingItemAtURL:url options:0 error:nil byAccessor:^(NSURL *newURL) {
    data = [NSData dataWithContentsOfURL:newURL];
  }];
  
  const long BUFFER_LEN = 1024;
  
  uint8_t buffer[BUFFER_LEN];
  
  [self showCopyProgressView];
  
  auto bytesAvailable = static_cast<unsigned long>(data.length);
  unsigned long readPos = 0;
  auto oStream = _urlProcessor->openOutputStream(filename.UTF8String, relativeDir.UTF8String);
  
  while (bytesAvailable > 0)
  {
    auto bytesToWrite = static_cast<unsigned long>(std::min(static_cast<long>(bytesAvailable), BUFFER_LEN));
    
    [data getBytes:&buffer range:NSMakeRange(readPos, bytesToWrite)];
    oStream->write(reinterpret_cast<char*>(buffer), static_cast<std::streamsize>(bytesToWrite));
    
    auto progress = readPos / (float) data.length;
    
    [self updateProgressView:progress];
    
    bytesAvailable -= bytesToWrite;
    readPos += bytesToWrite;
  }
}

- (void)handleURLs:(NSArray<NSURL*>*)urls relativeDir:(NSString*)relativeDir {
  for (id url in urls) {
    @autoreleasepool {
      [url startAccessingSecurityScopedResource];

      NSNumber *isDirectory;
      BOOL success = [url getResourceValue:&isDirectory forKey:NSURLIsDirectoryKey error:nil];

      if (success && [isDirectory boolValue]) {
        NSString* dirName = [url lastPathComponent];
        NSString* newRelativeDir = [NSString stringWithFormat:@"%@%@/", relativeDir, dirName];
        [self handleDir:url relativeDir:newRelativeDir];
      } else {
        [self handleFile:url relativeDir:relativeDir];
      }

      [url stopAccessingSecurityScopedResource];
    }
  }
}

- (void)documentBrowser:(UIViewController*)controller didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls
{
  auto bg_queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0);
  
  dispatch_async(bg_queue, ^{
    
    [self handleURLs:urls relativeDir:@""];
    
    dispatch_async(dispatch_get_main_queue(), ^{
        [controller dismissViewControllerAnimated:true completion:^{
            _urlProcessor->initFiles();
        }];
    });
  });
}

@end

/* ------------- */

@implementation UIWindow (DocumentBrowser)

-(void) cancelButtonAction:(id)sender {
  [[self rootViewController] dismissViewControllerAnimated:true completion:nil];
}

-(void) openIosDocumentBrowser:(ImportDocumentUrlProcessor*)urlProcessor {
  NSArray<NSString*> *utis = @[(NSString*)kUTTypeItem];
  UIDocumentBrowserViewController *picker =
  [[UIDocumentBrowserViewController alloc] initForOpeningFilesWithContentTypes:utis];
  auto cancelButton = [[UIBarButtonItem alloc]initWithCustomView:self];
  cancelButton.title = @"Cancel";
  cancelButton.action = @selector(cancelButtonAction:);
  cancelButton.target = self;
  picker.additionalLeadingNavigationBarButtonItems = [NSArray arrayWithObject:cancelButton];
  picker.allowsDocumentCreation = false;
  picker.allowsPickingMultipleItems = true;
  MyDelegate* delegate = [[MyDelegate alloc]init];
  [delegate setOverwriteAll:false];
  [delegate setOverwriteNone:false];
  [delegate setUrlProcessor:urlProcessor];
  [delegate setController:picker];
  picker.delegate = (id) delegate;
  picker.modalPresentationStyle = UIModalPresentationCurrentContext;
  [[self rootViewController] presentViewController:picker animated:YES completion:nil];
}

@end

/* ------------- */

void doOpenIosImportDocumentBrowser(ImportDocumentUrlProcessor* urlProcessor, void* nativeWindowHandle) {
  auto uiview = (UIView*) nativeWindowHandle;
  auto window = (UIWindow*)[uiview window];
  [window openIosDocumentBrowser:urlProcessor];
}

#endif
#endif
