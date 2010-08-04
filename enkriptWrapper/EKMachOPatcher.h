//
//  EKMachOPatcher.h
//  enkriptWrapper
//
//  Created by nico on 1/10/08.
//

#import <Cocoa/Cocoa.h>


@interface EKMachOPatcher : NSObject {
  NSString* originalPath;
  void* data;
  unsigned dataSize;
  void* mapData;
  unsigned mapSize;
  int fd;
  
}

+ (EKMachOPatcher*)machOInjectorWithMachOPath:(NSString*)path;
- (id)initWithMachOPath:(NSString*)path;
- (BOOL)injectNBytes:(unsigned)size fromSource:(void*)source inSection:(NSString*)sectionName ofSegment:(NSString*)segmentName; 
- (void*)entryPoint;
- (BOOL)segmentInfo:(NSString*)segmentName baseAddress:(void**)baseAddr andSize:(unsigned*)size;
- (unsigned)sectionCountOfSegment:(NSString*)segmentName;
- (BOOL)sectionInfo:(unsigned)sectionCount ofSegment:(NSString*)segmentName baseAddress:(void**)baseAddr andSize:(unsigned*)size;
- (BOOL)changeEntryPoint:(void*)entryPoint;
- (BOOL)changeEntryPointWithSection:(NSString*)segmentName inSegment:(NSString*)sectionName;
- (BOOL)saveToPath:(NSString*)path;
- (BOOL)encryptSection:(NSString*)sectionName ofSegment:(NSString*)segmentName;
- (BOOL)encryptSegment:(NSString*)segmentName;

@end
