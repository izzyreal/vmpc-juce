#import <Cocoa/Cocoa.h>

@interface CopyFileWindow : NSWindow

@property (copy, nonatomic) void (^onCancel)(void);
@property (assign) NSProgressIndicator* progressIndicator;

- (void)setProgressIndicatorDoubleValue:(double)newValue;

- (void)setFileName:(NSString *)fileName;

@end
