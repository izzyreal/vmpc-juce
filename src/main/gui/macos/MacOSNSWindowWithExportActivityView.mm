#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_OSX

#include <vector>
#include <AppKit/AppKit.h>

#include "Mpc.hpp"
#include "sampler/Sampler.hpp"
#include "file/aps/ApsParser.hpp"
#include "file/all/AllParser.hpp"
#include "file/sndwriter/SndWriter.hpp"
#include "lcdgui/screens/LoadScreen.hpp"
#include "disk/AbstractDisk.hpp"
#include "disk/MpcFile.hpp"

#include "miniz.h"

@implementation NSWindow (WithExportActivityView)

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
        NSString *soundName = [NSString stringWithUTF8String:sound->getName().c_str()];
        NSString *sndDataFilePath = [tempDirectory stringByAppendingPathComponent:[soundName stringByAppendingString:@".SND"]];
        if ([data writeToFile:sndDataFilePath atomically:YES]) { [filePathsArray addObject:sndDataFilePath]; }
    }
    
    return filePathsArray;
}

- (void)createZip:(fs::path)sourcePath filePathsArray:(NSMutableArray<NSString *> *)filePathsArray {
    NSString *tempDirectory = NSTemporaryDirectory();
    NSString *zipFileName = [[NSString stringWithUTF8String:sourcePath.filename().string().c_str()] stringByAppendingString:@".zip"];
    NSString *zipFilePath = [tempDirectory stringByAppendingPathComponent:zipFileName];

    mz_zip_archive zipArchive;
    memset(&zipArchive, 0, sizeof(zipArchive));
    mz_bool status = mz_zip_writer_init_file(&zipArchive, zipFilePath.UTF8String, 0);

    if (!status) {
        NSLog(@"Failed to initialize zip archive");
        return;
    }

    for (const auto& entry : fs::recursive_directory_iterator(sourcePath)) {
        if (!entry.is_directory()) {
            std::string relativePath = fs::relative(entry.path(), sourcePath).string();
            mz_zip_writer_add_file(&zipArchive, relativePath.c_str(), entry.path().c_str(), "", 0, MZ_BEST_COMPRESSION);
        }
    }

    mz_zip_writer_finalize_archive(&zipArchive);
    mz_zip_writer_end(&zipArchive);

    [filePathsArray addObject:zipFilePath];
}

- (NSMutableArray<NSString *> *)getSelectedFileOrDirectory:(mpc::Mpc *)mpc {
    auto selectedFile = mpc->screens->get<mpc::lcdgui::screens::LoadScreen>("load")->getSelectedFile();
    NSMutableArray<NSString *> *filePathsArray = [NSMutableArray array];

    if (selectedFile->isDirectory()) {
        [self createZip:selectedFile->getPath() filePathsArray:filePathsArray];
    } else {
        NSString *filePath = [NSString stringWithUTF8String:selectedFile->getPath().c_str()];
        [filePathsArray addObject:filePath];
    }

    return filePathsArray;
}

- (void)showSharingOptions:(NSArray<NSString *> *)filePaths {
    NSMutableArray *fileURLs = [NSMutableArray array];
    for (NSString *path in filePaths) {
        [fileURLs addObject:[NSURL fileURLWithPath:path]];
    }

    NSSharingServicePicker *picker = [[NSSharingServicePicker alloc] initWithItems:fileURLs];
    [picker showRelativeToRect:NSMakeRect(0, 0, 200, 100) ofView:self.contentView preferredEdge:NSRectEdgeMinY];
}

- (void)presentShareOptions:(mpc::Mpc *)mpc {
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Share Options"];
    [alert setInformativeText:@"Choose a sharing option."];
    [alert addButtonWithTitle:@"Share APS, SNDs, and ALL"];
    [alert addButtonWithTitle:@"Share Selected File or Directory"];
    [alert addButtonWithTitle:@"Cancel"];

    NSInteger button = [alert runModal];

    NSMutableArray<NSString *> *filePathsArray = [NSMutableArray array];

    if (button == NSAlertFirstButtonReturn) {
        [filePathsArray addObjectsFromArray:[self writeApsAllAndSnd:mpc]];
        [self showSharingOptions:filePathsArray];
    } else if (button == NSAlertSecondButtonReturn) {
        [filePathsArray addObjectsFromArray:[self getSelectedFileOrDirectory:mpc]];
        [self showSharingOptions:filePathsArray];
    }
}

@end

void doPresentShareOptions(void* nativeWindowHandle, mpc::Mpc* mpc) {
    auto nswindow = (NSWindow*) nativeWindowHandle;
    [nswindow presentShareOptions:mpc];
}

#endif
#endif

