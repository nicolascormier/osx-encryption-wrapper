/*
 *  EKUserClient.cpp
 *  enkriptKext
 *
 *  Created by nico on 27/09/08.
 *
 */

#include "EKUserClient.h"
#include "EKDebug.h"

/* com_enkript_driver_UserClient implementation
 * TODO: Structures aren't automatically swapped by the user client mechanism as it has no knowledge of how the fields
 * structure are laid out. -- needed for user processes running under rosetta
 */
#define super IOUserClient
OSDefineMetaClassAndStructors(com_enkript_driver_UserClient, IOUserClient)


bool com_enkript_driver_UserClient::initWithTask(task_t owningTask, void* securityToken, UInt32 type, OSDictionary* properties)
{
  EKDebugLog("");
  /* initialize super
   */
  if (!super::initWithTask(owningTask, securityToken, type, properties)) return EKErrorLog("super::initWithTask() failed");
  /* initiliaze members
   */
  task = owningTask;
  
  return true;
}

void com_enkript_driver_UserClient::free(void)
{
  EKDebugLog("");
  /* cleanup members
   */
  task = TASK_NULL;
  /* cleanup super
   */
  super::free();
}

bool com_enkript_driver_UserClient::start(IOService* provider)
{
  EKDebugLog("");
  /* test the provider before call super::start()
   */
  if (!OSDynamicCast(com_enkript_driver_Service, provider)) return EKErrorLog("invalid provider type");
  
  return super::start(provider);
}

bool com_enkript_driver_UserClient::didTerminate(IOService* provider, IOOptionBits options, bool* defer)
{
  EKDebugLog("");
  assert(getProvider() == provider); // should always be our original provider
	/* from Apple: If all pending I/O has been terminated, close the provider. If I/O is still outstanding, set defer to true
	 * and the user client will not have stop called on it
   */
  if (provider && provider->isOpen(this)) provider->close(this);
	*defer = false;
	
	return super::didTerminate(provider, options, defer);
}

IOReturn com_enkript_driver_UserClient::clientClose(void)
{
  EKDebugLog("");
  /* close provider
   */ 
  IOService* provider = getProvider();
  if (provider && provider->isOpen(this)) provider->close(this);

  return terminate(); // from Apple: don't call super::clientClose - don't know why it's not a pure virtual method...
}

IOReturn com_enkript_driver_UserClient::externalMethod(uint32_t selector, IOExternalMethodArguments* arguments, IOExternalMethodDispatch* dispatch, OSObject* target, void* reference)
{
  EKDebugLog("");
  /* need this ugly dispatch table, cf apple undocumented stuff...
   */
  static const IOExternalMethodDispatch dispatchTable[] = 
  {
    {(IOExternalMethodAction)&openUserClient, 0, 0, 0 },
    {(IOExternalMethodAction)&closeUserClient, 0, 0, 0 },
    {(IOExternalMethodAction)&hello, 1, 0, 0 },
  };
  if (selector >= sizeof(dispatchTable) / sizeof(IOExternalMethodDispatch))
  {
    (void) EKErrorLog("bad selector");
    return kIOReturnBadArgument;
  }
  dispatch = (IOExternalMethodDispatch*) &dispatchTable[selector];
  target = this;

	return super::externalMethod(selector, arguments, dispatch, target, reference);
}

#pragma mark -
#pragma mark ••• external clients methods •••

IOReturn com_enkript_driver_UserClient::openUserClient(com_enkript_driver_UserClient* self, void* reference, IOExternalMethodArguments* arguments)
{
  EKDebugLog("");
  EKAssert(self); // target should never be null
  IOService* provider = self->getProvider();
  if (!provider || self->isInactive())
  {
    /* no provider attached or user client is inactive
     */
    (void) EKErrorLog("no provider attached");
    return kIOReturnNotAttached;
  }
  if (!provider->open(self))
  {
    return kIOReturnError; // FIXME, no way to get a more detailed error ?
  }
  
  return kIOReturnSuccess;
}

IOReturn com_enkript_driver_UserClient::closeUserClient(com_enkript_driver_UserClient* self, void* reference, IOExternalMethodArguments* arguments)
{
  EKDebugLog("");
  EKAssert(self); // target should never be null
  IOService* provider = self->getProvider();
  if (!provider)
  {
    /* no provider attached
     */
    (void) EKErrorLog("no provider attached");
    return kIOReturnNotAttached;
  }
  if (provider->isOpen(self))
  {
    /* provider has been opened by this object, close it
     */
    provider->close(self);
    return kIOReturnSuccess;
  }
  (void) EKErrorLog("no provider opened");
  
  return kIOReturnNotOpen;
}

IOReturn com_enkript_driver_UserClient::hello(com_enkript_driver_UserClient* self, void* reference, IOExternalMethodArguments* arguments)
{
  EKDebugLog("");
  EKAssert(self); // target should never be null
  IOService* provider = self->getProvider();
  if (!provider || self->isInactive())
  {
    /* no provider attached or user client is inactive
     */
    (void) EKErrorLog("no provider attached");
    return kIOReturnNotAttached;
  }
  if (!provider->isOpen(self))
  {
    /* provider has not been opened by target or is not opened
     */
    (void) EKErrorLog("no provider opened");
    return kIOReturnNotOpen;
  }
  
  return ((com_enkript_driver_Service*)provider)->hello(self->task/*(uint32_t) arguments->scalarInput[0]*/); // provider is necessary of the good type
                                                                                               // TOFIX: check arguments
}
