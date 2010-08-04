/*
 *  EKService.cpp
 *  enkriptKext
 *
 *  Created by nico on 26/09/08.
 *
 */

#include <mach/mach_vm.h>

#include "EKService.h"
#include "EKDebug.h"

#if 0 // use local xnu source
# define XNU_KERNEL_PRIVATE
# include "/Users/nico/darwin_src/xnu-1228.7.58/osfmk/vm/vm_protos.h"
# include "/Users/nico/darwin_src/xnu-1228.7.58/osfmk/ipc/ipc_types.h"
#else // forward declaration declared in xnu private headers
extern "C"
{
  extern ipc_space_t  get_task_ipcspace(task_t t);
  struct ipc_object;
  extern kern_return_t ipc_object_copyin(ipc_space_t space, mach_port_name_t name, mach_msg_type_name_t msgt_name, struct ipc_object **objectp);
  extern kern_return_t ipc_port_alloc(ipc_space_t		space, mach_port_name_t	*namep, ipc_port_t *portp);
  extern kern_return_t ipc_port_dealloc_kernel(ipc_port_t	portp);  
  extern mach_msg_return_t mach_msg_receive(mach_msg_header_t *msg, mach_msg_option_t option, mach_msg_size_t rcv_size, mach_port_name_t rcv_name, mach_msg_timeout_t rcv_timeout, void (*continuation)(mach_msg_return_t), mach_msg_size_t slist_size);
  extern mach_msg_return_t mach_msg_send(mach_msg_header_t *msg, mach_msg_option_t option, mach_msg_size_t send_size, mach_msg_timeout_t send_timeout, mach_port_name_t notify);
  kern_return_t ipc_port_alloc(ipc_space_t space, mach_port_name_t	*namep, ipc_port_t *portp);
  void ipc_port_destroy(ipc_port_t	port);
  extern ipc_space_t	ipc_space_kernel;
  extern ipc_port_t ipc_port_make_send(ipc_port_t	port);
  extern thread_t		kernel_thread(task_t task,void (*start)(void));
}
#endif

/* com_enkript_driver_FilterScheme implementation
 */
#define super IOService
OSDefineMetaClassAndStructors(com_enkript_driver_Service, IOService)


bool com_enkript_driver_Service::init(OSDictionary* properties /*= 0*/)
{
  EKDebugLog("");
  if (!super::init(properties)) return EKErrorLog("super::init() failed");
  /* initialize members
   */
  /* ... */
  
  return true;
}

void com_enkript_driver_Service::free(void)
{
  /* cleanup members
   */
  /* ... */
  /* cleanup super
   */
  super::free();
}

bool com_enkript_driver_Service::start(IOService* provider)
{
  EKDebugLog("");
  /* start super
   */
  if (!super::start(provider)) return EKErrorLog("super::start() failed");
  /* register the service in order to be found be clients
   */
  registerService();  
  
  return true;
}

void com_enkript_driver_Service::stop(IOService* provider)
{
  EKDebugLog("");
  /* stop super
   */
  super::stop(provider);
}

namespace
{
  /* exception_port
   */
  ipc_port_t exception_port;
  mach_port_name_t exception_port_name = MACH_PORT_NULL;
  /* structures retrieve from xnu source code
   * bsd/uxkern/ux_exception.c
   * TODO: proper way...
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
  
  
  void exception_handler(void)
  {
    EKDebugLog("exception_handler thread started");
    kern_return_t ret = KERN_SUCCESS;
    while (true)
    {
      exc_msg_t msg_recv;  
      msg_recv.Head.msgh_local_port = exception_port;
      msg_recv.Head.msgh_size = sizeof(msg_recv);
      //kern_return_t ret = mach_msg(&(msg_recv.Head), MACH_RCV_MSG|MACH_RCV_LARGE|MACH_RCV_TIMEOUT, 0, sizeof(msg_recv), exception_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
      ret = mach_msg_receive(&msg_recv.Head, MACH_RCV_MSG, sizeof(msg_recv), (mach_port_name_t)exception_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL, 0);
      if (ret != KERN_SUCCESS) break;
      EKDebugLog("received exception");
      rep_msg_t msg_resp;
      exc_server(&msg_recv.Head, &msg_resp.Head);
      //if (mach_msg(&(msg_resp.Head), MACH_SEND_MSG, msg_resp.Head.msgh_size, 0, MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL) != KERN_SUCCESS) break;
      ret = mach_msg_send(&msg_resp.Head, MACH_SEND_MSG, sizeof(msg_resp), MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
      if (ret != KERN_SUCCESS) break;
    }
    EKDebugLog("exception_handler thread finished (ret=%d)", ret);
    /* clean up
     */
    ipc_port_destroy(exception_port);
  }
}


IOReturn com_enkript_driver_Service::hello(task_t target)
{
  EKDebugLog("hello task 0x%x", target);
  kern_return_t ret = KERN_SUCCESS;
  /* allocate receive port
   */
  ret = ipc_port_alloc(get_task_ipcspace(current_task()), &exception_port_name, &exception_port);
  EKDebugLog("ipc_port_alloc() ret=%d", ret);
  if (ret) return kIOReturnError;  
  /* set task exception port
   */
  ret = task_set_exception_ports(target, EXC_MASK_ALL, ipc_port_make_send(exception_port), EXCEPTION_DEFAULT, THREAD_STATE_NONE);
  EKDebugLog("task_set_exception_ports() ret=%d", ret);
  if (ret) return kIOReturnError;  
  /* create exception handler thread
   */
  (void) kernel_thread(kernel_task, exception_handler);

  return kIOReturnSuccess;
}

//  task_t self = current_task();
//  ipc_space_t ipc_space = get_task_ipcspace(self);
//  /* allocate receive port
//   */
//  mach_port_name_t exc_port_name;
//  if (mach_port_allocate(ipc_space, MACH_PORT_RIGHT_RECEIVE, &exc_port_name) != MACH_MSG_SUCCESS) return kIOReturnError;
//  /* allocate a port set and put receive port in it
//   */
//  mach_port_name_t exc_set_name;
//  if (mach_port_allocate(ipc_space, MACH_PORT_RIGHT_PORT_SET,  &exc_set_name) != MACH_MSG_SUCCESS) return kIOReturnError;
//  if (mach_port_move_member(ipc_space, exc_port_name,  exc_set_name) != MACH_MSG_SUCCESS) return kIOReturnError;
//  /* allocate send port
//   */
//  mach_port_name_t exception_port;
//  if (ipc_object_copyin(ipc_space, exc_port_name, MACH_MSG_TYPE_MAKE_SEND, (struct ipc_object**) &exception_port) != MACH_MSG_SUCCESS) return kIOReturnError;
