#import "CopyFileWindow.h"

@implementation CopyFileWindow

- (instancetype)init {
    NSRect frame = NSMakeRect(0, 0, 400, 100);
    self = [super initWithContentRect:frame
                             styleMask:NSWindowStyleMaskBorderless
                               backing:NSBackingStoreBuffered
                                 defer:NO];
    if (self) {
        [self setReleasedWhenClosed:NO];

        NSButton *cancel = [[NSButton alloc] initWithFrame:NSMakeRect(320, 65, 75, 30)];
        [cancel setTitle:@"Cancel"];
        [cancel setButtonType:NSButtonTypeMomentaryPushIn];
        [cancel setBezelStyle:NSBezelStyleRounded];
        [cancel setTarget:self];
        [cancel setAction:@selector(closeSheet)];
        [[self contentView] addSubview:cancel];
        
        NSTextField *statusLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 50, frame.size.width - 40, 20)];
        [statusLabel setBezeled:NO];
        [statusLabel setDrawsBackground:NO];
        [statusLabel setEditable:NO];
        [statusLabel setSelectable:NO];
        [statusLabel setStringValue:@"Preparing to import..."];
        [[self contentView] addSubview:statusLabel];
        
        [self setProgressIndicator:[[NSProgressIndicator alloc] init]];
        [_progressIndicator setStyle:NSProgressIndicatorBarStyle];
        [_progressIndicator setIndeterminate:NO];
        [_progressIndicator setMinValue:0.0];
        [_progressIndicator setMaxValue:1.0];
        [_progressIndicator setFrame:NSMakeRect(20, 20, frame.size.width - 40, 20)];
        [[self contentView] addSubview:_progressIndicator];
    }
    return self;
}

- (void)setProgressIndicatorDoubleValue:(double)newValue {
    [_progressIndicator setDoubleValue:newValue];
}

- (BOOL)canBecomeKeyWindow {
    return YES;
}

- (void)setFileName:(NSString *)fileName {
    __block NSTextField *statusLabel = (NSTextField *)[[self contentView] subviews][1];
    
    dispatch_async(dispatch_get_main_queue(), ^{
        [statusLabel setStringValue:[NSString stringWithFormat:@"Importing %@", fileName]];
    });
}

- (void)closeSheet {
    [NSApp endSheet:self];
    [self orderOut:nil];

    if (self.onCancel) {
        self.onCancel();
    }
}

@end
