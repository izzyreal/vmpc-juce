#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_MAC && !TARGET_OS_IPHONE

#include "ImportDocumentUrlProcessor.hpp"

#include <AppKit/AppKit.h>

@interface MyDelegate : NSObject <NSOpenSavePanelDelegate>

@property (assign) vmpc_juce::gui::macos::ImportDocumentUrlProcessor* urlProcessor;
@property (assign) NSProgressIndicator* progressIndicator;
@property (assign) NSWindow* window;
@property (assign) BOOL overwriteNone;
@property (assign) BOOL overwriteAll;

@end

@implementation MyDelegate

- (void)showCopyProgressIndicator {
  dispatch_sync(dispatch_get_main_queue(), ^{
    if (!_progressIndicator) {
      [self setProgressIndicator:[[NSProgressIndicator alloc] init]];
      [_progressIndicator setStyle:NSProgressIndicatorBarStyle];
      [_progressIndicator setIndeterminate:NO];
      [_progressIndicator setMinValue:0.0];
      [_progressIndicator setMaxValue:1.0];
      NSRect frame = [[_window contentView] frame];
      [_progressIndicator setFrame:NSMakeRect(20, frame.size.height - 40, frame.size.width - 40, 20)];
      [[_window contentView] addSubview:_progressIndicator];
    }
  });
}

- (void)updateProgressIndicator:(float)progress {
  dispatch_async(dispatch_get_main_queue(), ^{
    [_progressIndicator setDoubleValue:progress];
  });
}

- (void)handleDir:(NSURL*)url relativeDir:(NSString*)relativeDir {
  NSFileManager *fileManager = [NSFileManager defaultManager];
  NSArray<NSURL*>* urls = [fileManager contentsOfDirectoryAtURL:url includingPropertiesForKeys:@[NSURLNameKey, NSURLIsDirectoryKey, NSURLContentModificationDateKey] options:NSDirectoryEnumerationSkipsHiddenFiles error:nil];
  
  if (urls) {
    [self handleURLs:urls relativeDir:relativeDir];
  }
}

- (BOOL)showOverwriteAlert:(NSString*)relativeFileName {
  NSString* msg = [NSString stringWithFormat:@"File %@ exists. Overwrite?", relativeFileName];
  NSAlert *alert = [[NSAlert alloc] init];
  [alert setMessageText:@"File exists"];
  [alert setInformativeText:msg];
  [alert addButtonWithTitle:@"Yes"];
  [alert addButtonWithTitle:@"No"];
  [alert addButtonWithTitle:@"All"];
  [alert addButtonWithTitle:@"None"];
  NSModalResponse response = [alert runModal];

    if (response == NSAlertThirdButtonReturn) {
        _overwriteAll = YES;
        return YES;  // "All"
    } else if (response == NSAlertSecondButtonReturn) {
        _overwriteAll = NO;
        return NO;   // "No"
    } else if (response == NSAlertFirstButtonReturn) {
        return YES;  // "Yes"
    } else {
        return NO;   // "None" (any other response)
    }
}

- (void)handleFile:(NSURL*)url relativeDir:(NSString*)relativeDir {
  NSArray<NSString*>* allowedExtensions = @[@"wav",@"snd",@"aps",@"pgm",@"all",@"mid"];
  
  if (![allowedExtensions containsObject:url.pathExtension.lowercaseString]) {
    return;
  }
  
  NSString* filename = [url lastPathComponent];

  if (_urlProcessor->destinationExists(filename.UTF8String, relativeDir.UTF8String)) {
    NSString* relativeFileName = [NSString stringWithFormat:@"%@%@", relativeDir, filename];
    
    if (_overwriteNone) {
      return;
    }
    
    if (!_overwriteAll && ![self showOverwriteAlert:relativeFileName]) {
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
  
  [self showCopyProgressIndicator];
  
  unsigned long bytesAvailable = static_cast<unsigned long>(data.length);
  unsigned long readPos = 0;
  auto oStream = _urlProcessor->openOutputStream(filename.UTF8String, relativeDir.UTF8String);
  
  while (bytesAvailable > 0) {
    unsigned long bytesToWrite = std::min(static_cast<long>(bytesAvailable), BUFFER_LEN);
    
    [data getBytes:&buffer range:NSMakeRange(readPos, bytesToWrite)];
    oStream->write(reinterpret_cast<char*>(buffer), static_cast<std::streamsize>(bytesToWrite));
    
    float progress = readPos / (float) data.length;
    
    [self updateProgressIndicator:progress];
    
    bytesAvailable -= bytesToWrite;
    readPos += bytesToWrite;
  }
}

- (void)handleURLs:(NSArray<NSURL*>*)urls relativeDir:(NSString*)relativeDir {
  for (NSURL* url in urls) {
    @autoreleasepool {
      NSNumber *isDirectory;
      BOOL success = [url getResourceValue:&isDirectory forKey:NSURLIsDirectoryKey error:nil];

      if (success && [isDirectory boolValue]) {
        NSString* dirName = [url lastPathComponent];
        NSString* newRelativeDir = [NSString stringWithFormat:@"%@%@/", relativeDir, dirName];
        [self handleDir:url relativeDir:newRelativeDir];
      } else {
        [self handleFile:url relativeDir:relativeDir];
      }
    }
  }
}

- (void)openDocumentPicker:(NSWindow*)window {
  NSOpenPanel *panel = [NSOpenPanel openPanel];
  [panel setAllowsMultipleSelection:YES];
  [panel setCanChooseFiles:YES];
  [panel setCanChooseDirectories:YES];
  [panel setDelegate:self];
  
  if ([panel runModal] == NSModalResponseOK) {
    NSArray<NSURL*>* urls = [panel URLs];
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
      [self handleURLs:urls relativeDir:@""];
      dispatch_async(dispatch_get_main_queue(), ^{
        _urlProcessor->initFiles();
      });
    });
  }
}

@end

@implementation NSWindow (DocumentPicker)

-(void) openMacDocumentPicker:(vmpc_juce::gui::macos::ImportDocumentUrlProcessor*)urlProcessor {
  MyDelegate* delegate = [[MyDelegate alloc] init];
  [delegate setOverwriteAll:NO];
  [delegate setOverwriteNone:NO];
  [delegate setUrlProcessor:urlProcessor];
  [delegate setWindow:self];
  [delegate openDocumentPicker:self];
}

@end

void doOpenMacImportDocumentPicker(vmpc_juce::gui::macos::ImportDocumentUrlProcessor* urlProcessor, void* nativeWindowHandle) {
  NSView* view = (__bridge NSView*)nativeWindowHandle;
  NSWindow* window = [view window];
  [window openMacDocumentPicker:urlProcessor];
}

#endif
#endif

