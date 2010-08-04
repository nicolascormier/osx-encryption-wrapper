//
//  EKMachOPatcher.m
//  enkriptWrapper
//
//  Created by nico on 1/10/08.
//

#pragma mark TOFIX: IA32 only
// TOFIX: only operates on 32 bit mach-o files
//        - segment_command_64
//        only operates on x86
//        - thread_state
//


#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <mach-o/loader.h>
#include <mach-o/arch.h>
#include <mach-o/fat.h>
#include <mach/mach.h>
#include <mach/machine.h>

#import "EKMachOPatcher.h"
#import "EKDebug.h"


// private declarations
//
#pragma mark -
#pragma mark ••• private declarations •••

// structures masks (helper)
//
struct macho_mask
{
  struct mach_header h;
  struct load_command c[1];
};
typedef struct macho_mask macho_mask_t;

struct segment_map_mask
{
	struct segment_command seg;
  struct section sect[1];
};
typedef struct segment_map_mask segment_map_mask_t;

struct thread_command_mask
{
  struct thread_command cmd;
  uint32_t flavor;
	uint32_t count;
};
typedef struct thread_command_mask thread_command_mask_t;

// private methods definition
//
@interface  EKMachOPatcher (private)

- (struct segment_command*)segmentWithName:(NSString*)segmentName;
- (struct section*)sectionWithName:(NSString*)sectionName inSegment:(struct segment_command*)segment;
- (struct section*)sectionWithCount:(unsigned)sectionCount inSegment:(struct segment_command*)segment;

@end


// public methods implementation
//
#pragma mark -
#pragma mark ••• EKMachOPatcher (public) •••

@implementation EKMachOPatcher

+ (EKMachOPatcher*)machOInjectorWithMachOPath:(NSString*)path
{
  return [[[EKMachOPatcher alloc] initWithMachOPath:path] autorelease];
}

- (id)initWithMachOPath:(NSString*)path
{
  if ((self = [super init]) != nil)
  {
    // initialize members
    //
    originalPath = [path copy];
    fd = -1;
    data = NULL;
    dataSize = 0;
    // map mach-O file
    //
    struct stat	sb;
    if ((fd = open([originalPath UTF8String], O_RDWR)) == -1 || 
      fstat(fd, &sb) == -1 ||
      (mapData = mmap(0, sb.st_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0)) == (void*)-1)
    {
      if (fd != -1) (void) close(fd);
      [originalPath release];
      [super dealloc];
       (void) EKErrorLog(@"initialzation failed - path=%@", [path description]);
      return nil;
    }
    mapSize =  sb.st_size;
    // select architecture
    //
    unsigned magic = ((const struct mach_header*) mapData)->magic;
    struct fat_header* fat = (struct fat_header*) mapData;
    if (magic == FAT_CIGAM)
    {
      fat->nfat_arch = OSSwapBigToHostInt32(fat->nfat_arch);
      struct fat_arch* arch = (struct fat_arch *) &fat[1];
      for (unsigned i = 0; i < fat->nfat_arch; i++)
      {
        arch[i].cputype = OSSwapBigToHostInt32(arch[i].cputype);
        arch[i].cpusubtype = OSSwapBigToHostInt32(arch[i].cpusubtype);
        arch[i].offset = OSSwapBigToHostInt32(arch[i].offset);
        arch[i].size = OSSwapBigToHostInt32(arch[i].size);
        arch[i].align = OSSwapBigToHostInt32(arch[i].align);
      }
      magic = OSSwapBigToHostInt32(fat->magic);
    }
    if (magic == FAT_MAGIC)
    {
      const NXArchInfo* archInfo = NULL;
      struct fat_arch* arch = NULL;
      if (!(archInfo = NXGetLocalArchInfo()) || 
          !(arch = NXFindBestFatArch(archInfo->cputype, archInfo->cpusubtype, (struct fat_arch*)(&fat[1]), fat->nfat_arch)))
      {
        [self dealloc];
        (void) EKErrorLog(@"initialzation failed - path=%@", [path description]);
        return nil;
      }
      // update members
      //
      data = mapData + arch->offset;
      dataSize = arch->size;
    }
    else // not a fat binary
    {
      data = mapData;
      dataSize = mapSize;
    }
  }
  
  return self;
}

- (void) dealloc
{
  // cleanup
  //
  (void) munmap(mapData, mapSize);
  (void) close(fd);
  [originalPath release];
  // cleanup super
  //
  [super dealloc];
}

- (BOOL)injectNBytes:(unsigned)size fromSource:(void*)source inSection:(NSString*)sectionName ofSegment:(NSString*)segmentName; 
{
  return NO;
#if 0 // dead code - check mapSize/mapData if code needed
  struct segment_command* pagezero = [self segmentWithName:[NSString stringWithUTF8String:SEG_PAGEZERO]];
  if (!pagezero) return EKErrorLog(@"pagezero segment not found");
  // need adapt mapping size
  //
  void* newData = mmap(0, dataSize + size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
  if (newData == (void*)-1) return EKErrorLog(@"mmap() failed");
  (void*) memcpy(newData, data, dataSize);
  // inject data at the end of the file
  //
  unsigned injectAtOffset = dataSize;
  (void*) memcpy((char*)newData + injectAtOffset, source, size);
  // update members
  //
  if (munmap(data, dataSize) == -1) return EKErrorLog(@"munmap() failed");
  dataSize += size;
  data = newData;
  // need page zero segment
  //
  struct segment_command* pagezero = [self segmentWithName:[NSString stringWithUTF8String:SEG_PAGEZERO]];
  if (!pagezero) return EKErrorLog(@"pagezero segment not found");
  // modify __PAGEZERO segment
  //
  pagezero->fileoff = injectAtOffset;
  pagezero->filesize = size;
  pagezero->maxprot = VM_PROT_READ|VM_PROT_EXECUTE;
  pagezero->initprot = VM_PROT_READ|VM_PROT_EXECUTE;
  
  return YES;  
#endif
}

- (void*)entryPoint
{
  macho_mask_t* machobj = (macho_mask_t*) data;
  struct load_command* cmd = &machobj->c[0];
	for (unsigned i = 0; i < machobj->h.ncmds; i++)
  {
    if (cmd->cmd == LC_UNIXTHREAD)
    {
      thread_command_mask_t* thmsk = (thread_command_mask_t*) cmd;
      if (thmsk->flavor == i386_THREAD_STATE && thmsk->count == i386_THREAD_STATE_COUNT)
      {
        i386_thread_state_t* th = (i386_thread_state_t*)(thmsk + 1);
        return (void*) th->__eip;
      }      
    }
    cmd = (struct load_command *) ((unsigned char *) cmd + cmd->cmdsize);
  }
  
  
  return NULL;
}

- (unsigned)sectionCountOfSegment:(NSString*)segmentName
{
  struct segment_command* segment = [self segmentWithName:segmentName];
  if (segment) return segment->nsects;
  
  return 0;
}

- (BOOL)segmentInfo:(NSString*)segmentName baseAddress:(void**)baseAddr andSize:(unsigned*)size
{
  struct segment_command* segment = [self segmentWithName:segmentName];
  if (segment)
  {
    *baseAddr = (void*)segment->vmaddr;
    *size = segment->vmsize;
  }
  
  return segment != NULL;
}

- (BOOL)sectionInfo:(unsigned)sectionCount ofSegment:(NSString*)segmentName baseAddress:(void**)baseAddr andSize:(unsigned*)size
{
  struct section* sec = [self sectionWithCount:sectionCount inSegment:[self segmentWithName:segmentName]];
  if (sec)
  {
    *baseAddr = (void*) sec->addr;
    *size = sec->size;
  }
  
  return sec != NULL;
}

- (BOOL)changeEntryPoint:(void*)entryPoint
{
  macho_mask_t* machobj = (macho_mask_t*) data;
  struct load_command* cmd = &machobj->c[0];
	for (unsigned i = 0; i < machobj->h.ncmds; i++)
  {
    if (cmd->cmd == LC_UNIXTHREAD)
    {
      thread_command_mask_t* thmsk = (thread_command_mask_t*) cmd;
      if (thmsk->flavor == i386_THREAD_STATE && thmsk->count == i386_THREAD_STATE_COUNT)
      {
        i386_thread_state_t* th = (i386_thread_state_t*)(thmsk + 1);
        th->__eip = (unsigned int) entryPoint;
        return YES;
      }      
    }
    cmd = (struct load_command *) ((unsigned char *) cmd + cmd->cmdsize);
  }
  
  return NO;
}

- (BOOL)changeEntryPointWithSection:(NSString*)segmentName inSegment:(NSString*)sectionName
{
  struct section* sec = [self sectionWithName:sectionName inSegment:[self segmentWithName:segmentName]];
  if (!sec) return EKErrorLog(@"unknown segment");
  
  return [self changeEntryPoint:(void*)sec->addr];
}

- (BOOL)saveToPath:(NSString*)path
{
  NSData* nsdata = [NSData dataWithBytesNoCopy:data length:dataSize freeWhenDone:NO];
  return [nsdata writeToFile:path atomically:NO];
}

- (BOOL)encryptSegment:(NSString*)segmentName
{
  struct segment_command* segment = [self segmentWithName:segmentName];
  // encrypt all sections
  //
  segment_map_mask_t* map = (segment_map_mask_t*)segment;
  for (unsigned i = 0, n = segment->nsects; i < n; i++)
  {
    struct section* section = &map->sect[i];
    if (![self encryptSection:[NSString stringWithCString:section->sectname encoding:NSUTF8StringEncoding] ofSegment:segmentName])  return EKErrorLog(@"encrypt failed");
  }
  // change section's security attributes
  //
  segment->initprot = PROT_NONE;
  
  return YES;
}

- (BOOL)encryptSection:(NSString*)sectionName ofSegment:(NSString*)segmentName
{
  struct section* sec = [self sectionWithName:sectionName inSegment:[self segmentWithName:segmentName]];
  if (!sec) return EKErrorLog(@"unknown segment");
  char* sectionData = (char*)data + sec->offset;
  for (unsigned i = 0, n = sec->size; i < n; i++)
    sectionData[i] = ~(sectionData[i]);
  return YES;
}

@end

// private methods implementation
//
#pragma mark -
#pragma mark ••• EKMachOPatcher (pvivate) •••

@implementation EKMachOPatcher (private)

- (struct segment_command*)segmentWithName:(NSString*)segmentName
{
  macho_mask_t* machobj = (macho_mask_t*) data;
  const char* segmentNameCStr = [segmentName UTF8String];
  struct load_command* cmd = &machobj->c[0];
	for (unsigned i = 0; i < machobj->h.ncmds; i++)
  {
    if (cmd->cmd == LC_SEGMENT)
    {
      struct segment_command* seg = (struct segment_command *)cmd;
      if (!strcmp(segmentNameCStr, seg->segname)) return seg;
    }
    cmd = (struct load_command *) ((unsigned char *) cmd + cmd->cmdsize);
  }
  
  (void) EKErrorLog(@"segment not found");
  return NULL;
}

- (struct section*)sectionWithCount:(unsigned)sectionCount inSegment:(struct segment_command*)segment
{
  segment_map_mask_t* map = (segment_map_mask_t*)segment;
  for (unsigned i = 0, n = segment->nsects; i < n; i++)
  {
    struct section* section = &map->sect[i];
    if (i == sectionCount) return section;
  }
  
  return NULL;
}

- (struct section*)sectionWithName:(NSString*)sectionName inSegment:(struct segment_command*)segment
{
  if (!segment) return NULL;
  const char* sectionNameCStr = [sectionName UTF8String];
  for (unsigned i = 0; YES; i++)
  {
    struct section* section = [self sectionWithCount:i inSegment:segment];
    if (!section) break;
    if (!strcmp(sectionNameCStr, section->sectname)) return section;
  }
  
  (void) EKErrorLog(@"section not found");
  return NULL;
}

@end
