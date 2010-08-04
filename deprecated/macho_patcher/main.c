#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <mach-o/fat.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <mach-o/reloc.h>

/* TODO: real object objc / c ?
 * TOFIX: 32bits only
 */


struct macho
{
  struct mach_header h;
  struct load_command c[1];
};
typedef struct macho macho_t;

struct segment_map_toto
{
	struct segment_command seg;
  struct section sect[1];
};
typedef struct segment_map_toto segment_map_t;


void  plop(void* mapped_file)
{
  macho_t* machobj = (macho_t*) mapped_file;
  //if (machobj->h.magic != FAT_MAGIC) return; // wrong archi
  //if (machobj->h.filetype == MH_EXECUTE) return; // kext
  printf("heeerrreee\n");
  struct load_command* cmd = &machobj->c[0];
	for (unsigned i = 0; i < machobj->h.ncmds; i++)
  {
    if (cmd->cmd == LC_SEGMENT)
    {
      struct segment_command *seg = (struct segment_command *)cmd;
		    if (!strcmp("__TEXT", seg->segname))
        {
          // TOFIX 32bits
          printf("0x%x - 0x%x\n", seg->vmaddr, seg->vmaddr + seg->vmsize);
          segment_map_t* map = (segment_map_t*)seg;
          for (unsigned i = 0, n = seg->nsects; i < n; i++)
          {
            struct section* section = &map->sect[i];
            printf("%s\n", section->sectname);
            printf("0x%x - 0x%x\n", section->addr, section->addr + section->size);
            printf("%d\n", section->offset);
            if (!strcmp("__text", section->sectname))
            {
              printf("patched\n");
              unsigned addr = 0xdfac - 0x2300;
              char* data = (char*) mapped_file;
              data += section->offset + addr;
              for (unsigned j = 0; j < 0xdfcc - 0xdfac + 1; j++)
                data[j] = ~(data[j]);
            }
          }
        }
    }
    cmd = (struct load_command *) ((unsigned char *) cmd + cmd->cmdsize);
	}
}

int main (int argc, const char * argv[])
{
  if (!argv[1]) return 1;
  const char* path = argv[1];
  int fd = open(path, O_RDWR);
  if (fd == -1) return 1;
  struct stat	sb;
  if (fstat(fd, &sb) == -1)
  {
    (void) close(fd);
    return 1;
  }
  printf("size=%d\n", sb.st_size);
  void* data = mmap(0, sb.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if (!data)
  {
    (void) close(fd);
    return 1;
  }
  
  plop(data);
  
  (void) munmap(data, sb.st_size);
  (void) close(fd);
  
  return 0;
}
