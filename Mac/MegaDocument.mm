//
//  MegaDocument.m
//  Megacanvas
//
//  Created by Joe Groff on 6/15/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#import "MegaDocument.hh"
#import "MegaCanvasView.hh"

@implementation MegaDocument

- (id)init
{
    self = [super init];
    if (self) {
        canvas = Mega::Canvas::create();
    }
    return self;
}

- (NSString *)windowNibName
{
    return @"MegaDocument";
}

- (void)windowControllerDidLoadNib:(NSWindowController *)aController
{
    [super windowControllerDidLoadNib:aController];
    // ...
}

+ (BOOL)autosavesInPlace
{
    return YES;
}

+ (BOOL)canConcurrentlyReadDocumentsOfType:(NSString *)typeName
{
    return YES;
}

/*
- (BOOL)canAsynchronouslyWriteToURL:(NSURL *)url ofType:(NSString *)typeName forSaveOperation:(NSSaveOperationType)saveOperation
{
    return YES;
}
*/

- (BOOL)readFromURL:(NSURL *)absoluteURL ofType:(NSString *)typeName error:(NSError * __autoreleasing*)outError
{
    using namespace Mega;
    
    if (![absoluteURL isFileURL]) {
        *outError = [NSError errorWithDomain:NSCocoaErrorDomain
                                        code:NSFileWriteUnsupportedSchemeError
                                    userInfo:nil];
        return NO;
    }
    char const *path = [[absoluteURL path] UTF8String];
    
    std::string error;
    Owner<Canvas> newCanvas = Canvas::load(path, &error);
    if (!newCanvas) {
        NSLog(@"%s", error.c_str());
        auto errorInfo = [NSDictionary dictionaryWithObject:[NSString stringWithUTF8String:error.c_str()]
                                                     forKey:NSLocalizedDescriptionKey];
        *outError = [NSError errorWithDomain:NSCocoaErrorDomain
                                        code:NSFileWriteUnknownError 
                                    userInfo:errorInfo];
        return NO;
    }
    
    canvas = std::move(newCanvas);
    return YES;
}

- (BOOL)writeToURL:(NSURL *)absoluteURL ofType:(NSString *)typeName error:(NSError * __autoreleasing*)outError
{
    *outError = [NSError errorWithDomain:NSCocoaErrorDomain code:NSFileWriteUnsupportedSchemeError userInfo:nil];
    return NO;
}

- (NSApplicationPresentationOptions)window:(NSWindow *)window willUseFullScreenPresentationOptions:(NSApplicationPresentationOptions)proposedOptions
{
    return proposedOptions | NSApplicationPresentationAutoHideToolbar;
}

- (Mega::Canvas)canvas
{
    return canvas.get();
}

@end
