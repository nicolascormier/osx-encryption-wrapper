//
//  EKDecryptionCompanion.m
//  enkriptWrapper
//
//  Created by nico on 16/10/08.
//

#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <signal.h> // kill
#include <unistd.h> // fork, execve
#include <sys/wait.h> // waitpid

#import "EKMachOPatcher.h"
#import "EKDebug.h"
#import "EKShared.h"


// forward declarations
//
#pragma mark ••• forward declarations •••

extern boolean_t exc_server(mach_msg_header_t *request, mach_msg_header_t *reply); // bsd/uxkern/ux_exception.c
extern char** environ; // execve
static void companion_init(int argc, const char** argv, NSString* companionBinaryPath, NSString* encryptedBinaryPath, mach_vm_address_t protect_begin_addr, mach_vm_address_t protect_end_addr, mach_vm_address_t crypt_begin_addr, mach_vm_address_t crypt_end_addr);
static int companion_loop(void);


// types declarations
//
#pragma mark -
#pragma mark ••• types declarations •••

// structures retrieve from xnu source code
// bsd/uxkern/ux_exception.c
//
struct rep_msg
{
  mach_msg_header_t Head;
  NDR_record_t NDR;
  kern_return_t RetCode;
};
typedef struct rep_msg rep_msg_t;

struct exc_msg
{
  mach_msg_header_t Head;
  /* start of the kernel processed data */
  mach_msg_body_t msgh_body;
  mach_msg_port_descriptor_t thread;
  mach_msg_port_descriptor_t task;
  /* end of the kernel processed data */
  NDR_record_t NDR;
  exception_type_t exception;
  mach_msg_type_number_t codeCnt;
  mach_exception_data_t code;
  /* some times RCV_TO_LARGE probs */
  char pad[512];
};
typedef struct exc_msg exc_msg_t;


// companion entry point
//
#pragma mark -
#pragma mark ••• companion entry point •••

int main (int argc, const char * argv[])
{
  NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
  int ret = 1;
  NSString* companionBinaryPath = [[[NSProcessInfo processInfo] arguments] objectAtIndex:0];
  NSString* encryptedBinaryPath = [companionBinaryPath stringByAppendingString:EKSharedEncryptedExtension];
  // get encryption information
  //
  EKMachOPatcher* injector = [EKMachOPatcher machOInjectorWithMachOPath:encryptedBinaryPath];
  void* seg_vaddr; unsigned seg_size;
  void* sec0_vaddr; unsigned sec0_size;
  void* secn_vaddr; unsigned secn_size;
  if ([injector segmentInfo:@"__TEXT" baseAddress:&seg_vaddr andSize:&seg_size] &&
      [injector sectionInfo:0 ofSegment:@"__TEXT" baseAddress:&sec0_vaddr andSize:&sec0_size]    &&
      [injector sectionInfo:[injector sectionCountOfSegment:@"__TEXT"] - 1
                  ofSegment:@"__TEXT" 
                baseAddress:&secn_vaddr 
                    andSize:&secn_size])
  {
    // initialize companion
    //
    mach_vm_address_t protect_begin_addr = (unsigned) seg_vaddr;
    mach_vm_address_t protect_end_addr = protect_begin_addr + seg_size;
    mach_vm_address_t crypt_begin_addr = (unsigned) sec0_vaddr;
    mach_vm_address_t crypt_end_addr = crypt_begin_addr + (unsigned)secn_vaddr  - (unsigned)sec0_vaddr + secn_size;
    companion_init(argc, argv, companionBinaryPath, encryptedBinaryPath, protect_begin_addr, protect_end_addr, crypt_begin_addr, crypt_end_addr);
    // start target monitoring
    //
    ret = companion_loop();
  }
  else (void) EKErrorLog(@"invalid target binary %@", encryptedBinaryPath);
  [pool drain]; // cleanup
  
  return ret;
}


// private static members
//
#pragma mark -
#pragma mark ••• private static members •••

static BOOL init = NO;
static mach_port_t exception_port = MACH_PORT_DEAD;
static task_t child = MACH_PORT_DEAD;
static pid_t pid = -1;
static mach_vm_address_t protect_begin;
static mach_vm_address_t protect_end;
static mach_vm_address_t crypt_begin;
static mach_vm_address_t crypt_end;
static BOOL* uncrypted_chunk_map;
static mach_vm_size_t uncrypt_chunk_size;


// private helpers
//
#pragma mark -
#pragma mark ••• private helpers •••
static void die(NSString* str)
{
  (void) EKErrorLog(@"%@", str);
  (void) kill(pid, SIGKILL);
  exit(1);  
}

static void evaluate(NSString* str, kern_return_t ret)
{
  if (ret != KERN_SUCCESS)
  {
    const char* errstr = mach_error_string(ret);
    die([NSString stringWithFormat:@"%@ (mach_err=%s)", str, errstr]);
  }
}

static void debug_print_x86_state(x86_thread_state_t state)
{
  EKDebugLog(@"__eip=%x,"
             "__eax=%x,"
             "__ebx=%x,"
             "__ecx=%x,"
             "__edx=%x,"
             "__edi=%x,"
             "__esi=%x,"
             "__ebp=%x,"
             "__esp=%x,"
             "__ss=%x,"
             "__eflags=%x,"
             "__eip=%x,"
             "__cs=%x,"
             "__ds=%x,"
             "__es=%x,"
             "__fs=%x,"
             "__gs=%x,"
             ,
             state.uts.ts32.__eip,
             state.uts.ts32.__eax,
             state.uts.ts32.__ebx,
             state.uts.ts32.__ecx,
             state.uts.ts32.__edx,
             state.uts.ts32.__edi,
             state.uts.ts32.__esi,
             state.uts.ts32.__ebp,
             state.uts.ts32.__esp,
             state.uts.ts32.__ss,
             state.uts.ts32.__eflags,
             state.uts.ts32.__eip,
             state.uts.ts32.__cs,
             state.uts.ts32.__ds,
             state.uts.ts32.__es,
             state.uts.ts32.__fs,
             state.uts.ts32.__gs         
         );
}


// companion implementation
//
#pragma mark -
#pragma mark ••• companion implementation •••

kern_return_t catch_exception_raise(mach_port_t port, mach_port_t victim, mach_port_t task, exception_type_t exception, exception_data_t code, mach_msg_type_number_t code_count)
{
  #pragma mark TOFIX: (1) should look at exception type ; (2) x86 only
  
  x86_thread_state_t state;
  unsigned count = MACHINE_THREAD_STATE_COUNT;
  if (thread_get_state(victim, MACHINE_THREAD_STATE, (thread_state_t) &state, &count)) return KERN_FAILURE;
#if 0
  debug_print_x86_state(state);
#endif
  // check if an encrypted page raised the fault
  //
  mach_vm_address_t fault;
  if (state.uts.ts32.__eip >= protect_begin && state.uts.ts32.__eip <= protect_end) fault = state.uts.ts32.__eip;
  else if (state.uts.ts32.__eax >= protect_begin && state.uts.ts32.__eax <= protect_end) fault = state.uts.ts32.__eax;
  else if (state.uts.ts32.__ebx >= protect_begin && state.uts.ts32.__ebx <= protect_end) fault = state.uts.ts32.__ebx;
  else if (state.uts.ts32.__ecx >= protect_begin && state.uts.ts32.__ecx <= protect_end) fault = state.uts.ts32.__ecx;
  else if (state.uts.ts32.__edx >= protect_begin && state.uts.ts32.__edx <= protect_end) fault = state.uts.ts32.__edx;
  else if (state.uts.ts32.__edi >= protect_begin && state.uts.ts32.__edi <= protect_end) fault = state.uts.ts32.__edi;
  else if (state.uts.ts32.__esi >= protect_begin && state.uts.ts32.__esi <= protect_end) fault = state.uts.ts32.__esi;
  else if (state.uts.ts32.__ebp >= protect_begin && state.uts.ts32.__ebp <= protect_end) fault = state.uts.ts32.__ebp;
  else if (state.uts.ts32.__esp >= protect_begin && state.uts.ts32.__esp <= protect_end) fault = state.uts.ts32.__esp;
  else return KERN_FAILURE;
  // check if the page is still protected
  //
  mach_vm_address_t page = (mach_vm_address_t) fault - (fault % uncrypt_chunk_size);
  unsigned idx = page / uncrypt_chunk_size;
  if (idx >= sizeof(uncrypted_chunk_map))  return KERN_FAILURE;
  if (uncrypted_chunk_map[idx] == YES)  return KERN_FAILURE;
  // unprotected the page 
  //
  if (mach_vm_protect(child, (mach_vm_address_t) page , uncrypt_chunk_size, FALSE, VM_PROT_ALL)) return KERN_FAILURE;
  uncrypted_chunk_map[idx] = YES;
  // check if the page has been encrypted
  //
  mach_vm_address_t remote_address = page;
  mach_vm_address_t remote_address_end = page + uncrypt_chunk_size;
  if (remote_address_end < crypt_begin) return KERN_SUCCESS; // no decryption needed, just return
  if (remote_address < crypt_begin) remote_address = crypt_begin;
  if (remote_address_end > crypt_end) remote_address_end = crypt_end;
  mach_vm_size_t remote_size = remote_address_end - remote_address;
  // read the encrypted page
  //
  vm_offset_t local_address;
  mach_msg_type_number_t local_size;
  if (mach_vm_read(child, remote_address, remote_size, &local_address, &local_size)) return KERN_FAILURE;
  kern_return_t ret = KERN_FAILURE;
  if (local_size == remote_size) // should be the same size
  {
    // decrypt page
    //
    char* data = (char*)local_address;
    for (unsigned i = 0; i < local_size; i++) 
    {
      data[i] = ~(data[i]); // XOR
    }
    // write modifications
    //
    ret = mach_vm_write(child, remote_address, local_address, local_size);
  }
  
  (void) mach_vm_deallocate(mach_task_self(), local_address, local_size); // clean mach allocation
  return ret;
}

static void companion_init(int argc, const char** argv,  NSString* companionBinaryPath, NSString* encryptedBinaryPath, mach_vm_address_t protect_begin_addr, mach_vm_address_t protect_end_addr, mach_vm_address_t crypt_begin_addr, mach_vm_address_t crypt_end_addr)
{
  if (init) return;
  init = YES;
  // initialize members
  //
  protect_begin = protect_begin_addr;
  protect_end = protect_end_addr;
  crypt_begin = crypt_begin_addr;
  crypt_end = crypt_end_addr;
  uncrypt_chunk_size = PAGE_SIZE;
  uncrypted_chunk_map = calloc(1 + ((protect_end - protect_begin) / uncrypt_chunk_size), sizeof(BOOL)); // alloc/init map
  if (!uncrypted_chunk_map) die(@"calloc() failed");
  // create a child for our encrypted process
  //
  NSDistributedLock* lock = [NSDistributedLock lockWithPath:[companionBinaryPath stringByAppendingString:@".lock"]];
  pid = fork();
  if (!pid) // child process
  {
    while (![lock tryLock]); // spin lock
    // launch encrypted process
    //
    if (execve([encryptedBinaryPath UTF8String], (char**) argv, environ) == -1)
    {
      (void) EKErrorLog(@"execve failed (errno=%d)", errno);
      exit(1); 
    }
  }
  // father process
   //
  evaluate(@"task_for_pid() failed", task_for_pid(mach_task_self(), pid, &child));
  evaluate(@"mach_port_allocate() failed", mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &exception_port));
  evaluate(@"mach_port_insert_right() failed", mach_port_insert_right(mach_task_self(), exception_port, exception_port, MACH_MSG_TYPE_MAKE_SEND));
  evaluate(@"task_set_exception_ports() failed", task_set_exception_ports(child, EXC_MASK_ALL, exception_port, EXCEPTION_DEFAULT, THREAD_STATE_NONE));
  // release child exec
   //
  [lock unlock];
}

static int companion_loop(void)
{
  if (!init) return 0;
  // monitoring loop
  //
  int stat_loc;
  while (YES)
  {
    // check child status
    //
    if (waitpid(pid, &stat_loc, WNOHANG) == pid) break;
    // listen exception_port
    //
    exc_msg_t msg_recv;  
    msg_recv.Head.msgh_local_port = exception_port;
    msg_recv.Head.msgh_size = sizeof(msg_recv);
    kern_return_t ret = mach_msg(&(msg_recv.Head), MACH_RCV_MSG|MACH_RCV_LARGE|MACH_RCV_TIMEOUT, 0, sizeof(msg_recv), exception_port, 5, MACH_PORT_NULL);
    if (ret == MACH_RCV_TIMED_OUT) continue;
    else evaluate(@"mach_msg_receive() failed", ret);
    rep_msg_t msg_resp;
    exc_server(&msg_recv.Head, &msg_resp.Head);
    evaluate(@"mach_msg_send() failed", mach_msg(&(msg_resp.Head), MACH_SEND_MSG, msg_resp.Head.msgh_size, 0, MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL));
  }
  if (WIFEXITED(stat_loc)) return WEXITSTATUS(stat_loc);
  
  return 0;
}
