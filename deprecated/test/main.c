/*
 *  main.c
 *  test
 *
 *  Created by nico on 28/09/08.
 *
 */

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <unistd.h>

/* kern_return_t
 *  IOConnectCallMethod(
 *                        mach_port_t	 connection,		// In
 *                        uint32_t	 selector,        // In
 *                        const uint64_t	*input,			// In
 *                        uint32_t	 inputCnt,        // In
 *                        const void *inputStruct,		// In
 *                        size_t		 inputStructCnt,	// In
 *                        uint64_t	*output,          // Out
 *                        uint32_t	*outputCnt,       // In/Out
 *                        void		*outputStruct,      // Out
 *                        size_t		*outputStructCnt)	// In/Out
 */


static inline void debug(const char* str)
{
  CFStringRef cfstr = CFStringCreateWithCString(NULL, str, kCFStringEncodingASCII);
  CFShow(cfstr);
  CFRelease(cfstr);
  CFShow(CFSTR("\n"));
}

static inline void die(const char* str)
{
  debug(str);
  exit(1);
}

#include <mach/mach.h>

void hello(void)
{
  while (1);
}

extern void enkript_prologue (void) __attribute__ ((section ("__enkript,prologue")));
void enkript_prologue(void)
{
  if (IOConnectCallMethod == NULL) die("IOConnectCallMethod unavailable, require version >= 10.5");
  /* get iterator to browse drivers of the chosen class
   */
  io_iterator_t iterator;
  if (IOServiceGetMatchingServices(kIOMasterPortDefault, IOServiceMatching("com_enkript_driver_Service"), &iterator) != KERN_SUCCESS) die("IOServiceGetMatchingServices failed");
  /* browse drivers
   */
  for (io_service_t service = IOIteratorNext(iterator); service != IO_OBJECT_NULL; service = IOIteratorNext(iterator))
  {
    debug("com_enkript_driver_Service instance found!");
    /* open service
     */
    io_connect_t connect = IO_OBJECT_NULL;
    if (IOServiceOpen(service, mach_task_self(), 0, &connect) != KERN_SUCCESS) die("IOServiceOpen failed");
    /* call driver's open method
     */
    if (IOConnectCallMethod(connect, 0 /* open, TOFIX */, NULL, 0, NULL, 0, NULL, NULL, NULL, NULL) != KERN_SUCCESS) die("IOConnectCallMethod failed");
    /* call driver's hello method
     */
    uint64_t buf = getpid();
    if (IOConnectCallMethod(connect, 2 /* hello, TOFIX */, &buf, 1, NULL, 0, NULL, NULL, NULL, NULL) != KERN_SUCCESS) die("IOConnectCallMethod failed");
    /* call driver's close method
     */
    if (IOConnectCallMethod(connect, 1 /* close, TOFIX */, NULL, 0, NULL, 0, NULL, NULL, NULL, NULL) != KERN_SUCCESS) die("IOConnectCallMethod failed");
    /* close service
     */
    if (IOServiceClose(connect) != KERN_SUCCESS) die("IOServiceClose failed");
	}
  IOObjectRelease(iterator);
}

int main()
{
  enkript_prologue();
  printf("hello :o)\n");
}
