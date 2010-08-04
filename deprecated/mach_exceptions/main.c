#include <stdio.h>
#include <stdlib.h>
#include <mach/mach.h>
#include <pthread.h>

#include <unistd.h>
#include <mach/mach_vm.h>


/* structures retrieve from xnu source code
 * bsd/uxkern/ux_exception.c
 * TOFIX: proper way...
 */
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


extern boolean_t exc_server(mach_msg_header_t *request, mach_msg_header_t *reply);

static void evaluate(const char* str, kern_return_t ret)
{
  if (ret != KERN_SUCCESS)
  {
    mach_error("", ret);
    (void) printf(" - %s\n", str);
    exit(1);
  }
}

static mach_port_t exception_port;
static void exception_handler(void)
{
  exc_msg_t msg_recv;  
  msg_recv.Head.msgh_local_port = exception_port;
  msg_recv.Head.msgh_size = sizeof(msg_recv);
  kern_return_t ret = mach_msg(&(msg_recv.Head), MACH_RCV_MSG|MACH_RCV_LARGE|MACH_RCV_TIMEOUT, 0, sizeof(msg_recv), exception_port, 5, MACH_PORT_NULL);
  if (ret == MACH_RCV_TIMED_OUT) return;
  else evaluate("mach_msg_receive() failed", ret);
  
//  printf("received message\n");
//  printf("victim thread is %#lx\n", (long)msg_recv.thread.name);
//  printf("victim thread's task is  %#lx\n", (long)msg_recv.task.name);
  
  rep_msg_t msg_resp;
  exc_server(&msg_recv.Head, &msg_resp.Head);
  // now msg_resp.RetCode contains return value of catch_exception_raise()
//  printf("sending reply\n");
  evaluate("mach_msg_send() failed", mach_msg(&(msg_resp.Head), MACH_SEND_MSG, msg_resp.Head.msgh_size, 0, MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL));
}



typedef void (* funcptr_t)(void);
funcptr_t function_with_bad_instruction;


extern char **environ;
int child_proc(void)
{
  sleep(1);
  char* argv[] = {"/usr/bin/gcc", "-v", "/Users/nico/toto.c", "-o", "/Users/nico/toto", NULL};
  if (execve("/Users/nico/test.inject", argv, environ) == -1)
  {
    (void) printf("execve() failed\n");
    return 1;
  }
  return 0;
}

task_t child;


#define protect_begin 0x00001000
#define protect_end (protect_begin + 0x00001000)
#define crypt_begin 0x00001f6c
#define crypt_end (crypt_begin + 0x00000092)
#define uncrypt_chunk_size 4096

kern_return_t catch_exception_raise(mach_port_t port, mach_port_t victim, mach_port_t task, exception_type_t exception, exception_data_t code, mach_msg_type_number_t code_count)
{
  // TOFIX: should look at exception type
  static boolean_t uncrypted_chunk_map[((protect_end - protect_begin) / uncrypt_chunk_size) + 1];
  static boolean_t init = FALSE;
  if (!init)
  {
    bzero(uncrypted_chunk_map, sizeof(uncrypted_chunk_map));
    init = TRUE;
  }
  x86_thread_state_t state; // TOFIX: x86 only...
  unsigned count = MACHINE_THREAD_STATE_COUNT;
  evaluate("thread_get_state() failed", thread_get_state(victim, MACHINE_THREAD_STATE, (thread_state_t) &state, &count));
  
#if 0
  printf("__eip=%x,"
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
        "\n",
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
#endif
  
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
  
  mach_vm_address_t page = (mach_vm_address_t) fault - (fault % uncrypt_chunk_size); // tmp  - deg vm_map_trunc/round_page
  
  unsigned idx = page / uncrypt_chunk_size;
  if (idx >= sizeof(uncrypted_chunk_map))  return KERN_FAILURE; //evaluate("invalid idx", KERN_FAILURE);
  if (uncrypted_chunk_map[idx] == TRUE)  return KERN_FAILURE; //evaluate("already unprotected", KERN_FAILURE);;
  
  //printf("unprotect 0x%x", page); printf(" - 0x%x\n", page + uncrypt_chunk_size);
  evaluate("mach_vm_protect() failed", mach_vm_protect(child, (mach_vm_address_t) page , uncrypt_chunk_size, FALSE, VM_PROT_ALL));
  uncrypted_chunk_map[idx] = TRUE;

  mach_vm_address_t remote_address = page;
  mach_vm_address_t remote_address_end = page + uncrypt_chunk_size;
  if (remote_address_end < crypt_begin) return KERN_SUCCESS; // no decryption needed
  if (remote_address < crypt_begin) remote_address = crypt_begin;
  if (remote_address_end > crypt_end) remote_address_end = crypt_end;
  mach_vm_size_t remote_size = remote_address_end - remote_address;

  vm_offset_t local_address;
  mach_msg_type_number_t local_size;
  //printf("remote_address=%x", remote_address); printf(" - remote_size=%x\n", remote_size);
  evaluate("mach_vm_read() failed", mach_vm_read(child, remote_address, remote_size, &local_address, &local_size));
  //printf("local_address=%x, local_size=%x\n", local_address, local_size);
  if (local_size != remote_size) evaluate("assert", KERN_FAILURE);
  char* data = (char*)local_address;
  for (unsigned i = 0; i < local_size; i++) 
  {
    data[i] = ~(data[i]);
  }
  evaluate("mach_vm_write() failed", mach_vm_write(child, remote_address, local_address, local_size));
  // dealloc !!
  
  
  return KERN_SUCCESS;
}

int main (int argc, const char * argv[])
{
  pid_t pid = fork();
  if (!pid) return child_proc();
  evaluate("task_for_pid() failed", task_for_pid(mach_task_self(), pid, &child));
  evaluate("mach_port_allocate() failed", mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &exception_port));
  evaluate("mach_port_insert_right() failed", mach_port_insert_right(mach_task_self(), exception_port, exception_port, MACH_MSG_TYPE_MAKE_SEND));
  evaluate("task_set_exception_ports() failed", task_set_exception_ports(child, EXC_MASK_ALL, exception_port, EXCEPTION_DEFAULT, THREAD_STATE_NONE));
  sleep(3); // synchro
  while (1)
  {
    exception_handler();
    int stat_loc;
    if (waitpid(pid, &stat_loc, WNOHANG) == pid) break;
  }
}
      
      
      
      
      //evaluate("task_suspend() failed", task_suspend(mach_task_self())); 
      
      //  printf("0x%x\n", child_proc);
      //
      //  sleep(2);
      //
      //  evaluate("task_suspend() failed", task_suspend(child));  
    
      //evaluate("mach_vm_protect() failed", mach_vm_protect(child, (mach_vm_address_t) 0xd000 , 4096, FALSE, VM_PROT_NONE));    
      //  evaluate("task_resume() failed", task_resume(child));  
      
      //  pthread_t exception_thread;
      
      //  (void) pthread_create(&exception_thread, (pthread_attr_t *)0, (void *(*)(void *))exception_handler, (void *)0);
      //  (void) pthread_detach(exception_thread);
      //  
      //  function_with_bad_instruction = (funcptr_t)exception_thread;
      //  function_with_bad_instruction();
      
      
    
    
      //  if (count_boulet == 0)
      //  {
      //    count_boulet = 1;
      //    x86_thread_state_t state;
      //    unsigned count = MACHINE_THREAD_STATE_COUNT;
      //    evaluate("thread_get_state() failed", thread_get_state(victim, MACHINE_THREAD_STATE, (thread_state_t) &state, &count));
      //    mach_vm_address_t page = (mach_vm_address_t) state.uts.ts32.__eip - (state.uts.ts32.__eip % 4096); // tmp  - deg vm_map_trunc/round_page
      //    printf("eip %x, page %x\n", state.uts.ts32.__eip, page);
      //    printf("%x - %x - %x - %x - %x - %x - %x - %x\n", state.uts.ts32.__eax,
      //           state.uts.ts32.__ebx,
      //           state.uts.ts32.__ecx,
      //           state.uts.ts32.__edx,
      //           state.uts.ts32.__edi,
      //           state.uts.ts32.__esi,
      //           state.uts.ts32.__ebp,
      //           state.uts.ts32.__esp);
      //
      //    state.uts.ts32.__eip = 0x00001f74;
      //    evaluate("thread_set_state() failed", thread_set_state(victim, MACHINE_THREAD_STATE, (thread_state_t) &state, MACHINE_THREAD_STATE_COUNT));
      //    
      //    
      //    evaluate("mach_vm_protect() failed", mach_vm_protect(child, (mach_vm_address_t) 0x00001f74 , 0x00001f74 + 0x00000088, FALSE, VM_PROT_NONE));    
      //
      //    
      //    
      //    return KERN_SUCCESS;
      //  }  
      
      
      //  if (count_boulet == 0)
      //  {
      //    count_boulet = 1;
      //    evaluate("mach_vm_protect() failed", mach_vm_protect(child, (mach_vm_address_t) 0x1000 , 0x23000 - 0x1000, FALSE, VM_PROT_NONE));
      //  }
      //  else



      //  }
      
      //  printf("eip %x\n", state.uts.ts32.__eip);
      //  
      //  state.uts.ts32.__eip = 0x22f57;
      //
      //  evaluate("thread_set_state() failed", thread_set_state(victim, MACHINE_THREAD_STATE, (thread_state_t) &state, MACHINE_THREAD_STATE_COUNT));
