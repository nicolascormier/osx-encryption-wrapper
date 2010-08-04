/*
 *  EKUserClient.h
 *  enkriptKext
 *
 *  Created by nico on 27/09/08.
 *
 */

#ifndef __ENKRIPTKEXT_EKUSERCLIENT_H
# define __ENKRIPTKEXT_EKUSERCLIENT_H 

# include <IOKit/IOService.h>
# include <IOKit/IOUserClient.h>
# include "EKService.h"


class com_enkript_driver_UserClient : public IOUserClient
  {
    OSDeclareDefaultStructors(com_enkript_driver_UserClient)
    
  protected:
    task_t task;
    
  public:
    /* Override IOService
     */
    virtual void free(void);
    virtual bool start(IOService* provider);
    virtual bool didTerminate(IOService* provider, IOOptionBits options, bool* defer);
    /* Override IOUserClient
     */
    virtual bool initWithTask(task_t owningTask, void* securityToken, UInt32 type, OSDictionary* properties);
    virtual IOReturn clientClose(void);
    
  protected:
    /* Override IOUserClient
     */
    virtual IOReturn externalMethod(uint32_t selector, IOExternalMethodArguments* arguments, IOExternalMethodDispatch* dispatch, OSObject* target, void* reference);
    /* Client methods
     */
    static IOReturn openUserClient(com_enkript_driver_UserClient* target, void* reference, IOExternalMethodArguments* arguments);
    static IOReturn closeUserClient(com_enkript_driver_UserClient* target, void* reference, IOExternalMethodArguments* arguments);
    static IOReturn hello(com_enkript_driver_UserClient* target, void* reference, IOExternalMethodArguments* arguments);
  };

#endif // __ENKRIPTKEXT_EKUSERCLIENT_H
