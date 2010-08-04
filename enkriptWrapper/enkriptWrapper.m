//
//  enkriptWrapper.m
//  enkriptWrapper
//
//  Created by nico on 16/10/08.
//

#import <Foundation/Foundation.h>

#import "EKDebug.h"
#import "EKMachOPatcher.h"
#import "EKShared.h"


int main (int argc, const char * argv[])
{
  NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
  BOOL ret = NO;
  if (argc > 1)
  {
    NSString* targetPath = [NSString stringWithUTF8String:argv[1]];
    NSString* companionPath = [targetPath stringByAppendingString:@".trampoline"];
    NSString* encryptedTargetPath = [companionPath stringByAppendingString:EKSharedEncryptedExtension]; //[companionPath stringByAppendingPathComponent:@"rsrc"];
    EKDebugLog(@"target binary: %@", targetPath);
    EKDebugLog(@"encrypted save @ %@ (%@)", companionPath, encryptedTargetPath);
    EKMachOPatcher* injector = [EKMachOPatcher machOInjectorWithMachOPath:targetPath];
    NSFileManager* fileManager = [NSFileManager defaultManager];
    ret = [injector encryptSegment:@"__TEXT"];
    if (ret)
    {
      NSString* wrapperPath = [[[NSProcessInfo processInfo] arguments] objectAtIndex:0];
      NSString* companionSrcPath = [[wrapperPath stringByDeletingLastPathComponent] stringByAppendingPathComponent:@"enKriptCompanion"];
      ret = [fileManager fileExistsAtPath:companionSrcPath] && ![fileManager fileExistsAtPath:companionPath];
      if (ret) ret = [fileManager copyPath:companionSrcPath toPath:companionPath handler:nil];
      else (void) EKErrorLog(@"can't copy companion from %@ to %@", companionSrcPath, companionPath);
    }
    if (ret) ret = [injector saveToPath:encryptedTargetPath];
    else (void) EKErrorLog(@"can't save encrypted binary to path %@", encryptedTargetPath);
    if (ret) ret = [fileManager changeFileAttributes:[NSDictionary dictionaryWithObject:[NSNumber numberWithUnsignedLong:0700UL] forKey:NSFilePosixPermissions] 
                                              atPath:encryptedTargetPath];
    else (void) EKErrorLog(@"can't change %@ attributes to 0700", encryptedTargetPath);
  }
  else (void) EKErrorLog(@"usage: enrkiptWrapper binary_to_encrypt");
  [pool drain];
  
  return ret ? 0 : 1;
}
