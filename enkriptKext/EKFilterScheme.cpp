/*
 *  EKFilterScheme.cpp
 *  enkriptKext
 *
 *  Created by nico on 19/09/08.
 *
 */

#include "EKFilterScheme.h"
#include "EKDebug.h"
#include "EKString.h"


/* com_enkript_driver_FilterScheme implementation
 */
#define super IOStorage
OSDefineMetaClassAndStructors(com_enkript_driver_FilterScheme, IOStorage)


/* Protected methods implementation
 */
#pragma mark -
#pragma mark ••• protected •••

bool com_enkript_driver_FilterScheme::handleOpen(IOService* client, IOOptionBits options, void* access)
{
	return getProvider()->open(this, options, (IOStorageAccess) access);
}

bool com_enkript_driver_FilterScheme::handleIsOpen(const IOService* client) const
{
	return getProvider()->isOpen(this);
}

void com_enkript_driver_FilterScheme::handleClose(IOService* client, IOOptionBits options)
{
	getProvider()->close(this, options);
}

/* Public methods implementation
 */
#pragma mark -
#pragma mark ••• public •••

bool com_enkript_driver_FilterScheme::init(OSDictionary* properties /*= 0*/)
{
	bool ret = super::init(properties); // super object initialization
  if (ret)
  {
    /* initialize properties
     */
    uncryptedMedia = NULL;
  }
  
  return ret;
}

void com_enkript_driver_FilterScheme::free(void)
{
  /* cleanup properties
   */
  if (uncryptedMedia) uncryptedMedia->release();
  /* cleanup super object
   */
  super::free();
}

namespace // read completion callback implementation
{
  struct readCallbackContext
  {
    IOMemoryDescriptor* buffer;
    IOStorageCompletion originalCompletion;
  };
  
  void   readCallback(void* /* target */, readCallbackContext* context, IOReturn status, UInt64 actualByteCount)
  {
    EKDebugLog("");
    EKAssert(context); // context should never be null
    EKAssert(context->buffer); // buffer should never be null
    IOMemoryDescriptor* buffer = context->buffer;
    unsigned length = buffer->getLength();
    EKAssert(!(length % 512)); // length should always be multiple of 512
    /* decrypt data
     */
    buffer->prepare(kIODirectionInOut); // prepare for reading and writting
    for (unsigned i = 0, n = length / 512; i < n; i++)
    {
      /* read, uncrypt and write 64 bytes
       */
      UInt64 byteBlock[64];
      (void) buffer->readBytes(i * 512, (UInt8*) byteBlock, 512);
      for (unsigned j = 0; j < 64; j++) byteBlock[j] = ~(byteBlock[j]);
      (void) buffer->writeBytes(i * 512, (UInt8*) byteBlock, 512);
    }
    buffer->complete();
    buffer->prepare(kIODirectionOut); // TOFIX: necessary??
    /* run the original completion routine
     */
    IOStorage::complete(context->originalCompletion, status, actualByteCount);
    /* cleanup
     */
    delete context;
  }
}

void com_enkript_driver_FilterScheme::read(IOService* client, UInt64 byteStart, IOMemoryDescriptor* buffer, IOStorageCompletion completion)
{
  /* need to setup a callback after provider read
   */
  readCallbackContext* context = new readCallbackContext;
  if (context)
  {
    context->buffer = buffer; // no need to retain, the buffer will be retained for the duration of the read (Apple)
    context->originalCompletion = completion;
    /* setup the new completion
     */
    completion.action = (IOStorageCompletionAction) readCallback;
    completion.target = NULL;
    completion.parameter = (void*) context;
  }
  else (void) EKErrorLog("allocation failed");
  /* hand over provider
   */
	getProvider()->read(this, byteStart, buffer, completion);
}

namespace // write completion callback implementation
{
  struct writeCallbackContext
  {
    IOStorageCompletion originalCompletion;
    void* data;
    unsigned length;
    IOMemoryDescriptor* writeBuffer;
    IOMemoryDescriptor* readBuffer;
  };
  
  void   writeCallback(void* /* target */, writeCallbackContext* context, IOReturn status, UInt64 actualByteCount)
  {
    EKDebugLog("");
    EKAssert(context); // context should never be null
    /* run the original completion routine
     */
    IOStorage::complete(context->originalCompletion, status, actualByteCount);
    /* clean up context
     */
    if (context->writeBuffer) context->writeBuffer->release();
    if (context->readBuffer) context->readBuffer->release();
    if (context->data) IOFree(context->data, context->length);
    delete context;
  }
}

void com_enkript_driver_FilterScheme::write(IOService* client, UInt64 byteStart, IOMemoryDescriptor* buffer, IOStorageCompletion completion)
{
  EKDebugLog("");
  EKAssert(buffer); // should never be null
  unsigned length = buffer->getLength();
  EKAssert(!(length % 512)); // should always be multiple of 512
  /* need to setup a callback after provider write
   */
  writeCallbackContext* context = new writeCallbackContext;
  if (context)
  {
    /* initialize context
     */
    context->data = NULL;
    context->length = length;
    context->readBuffer = NULL;
    context->writeBuffer = NULL;
    context->originalCompletion = completion;
    /* setup the new completion
     */
    completion.action = (IOStorageCompletionAction) writeCallback;
    completion.target = NULL;
    completion.parameter = (void*) context;
    /* we need to create a buffer for storing provider's crypted data
     */
    context->data = IOMalloc(length);
    if (context->data)
    {
      context->writeBuffer = IOMemoryDescriptor::withAddress(context->data, context->length, kIODirectionIn);
      context->readBuffer = IOMemoryDescriptor::withAddress(context->data, context->length, kIODirectionOut);
      if (context->writeBuffer && context->readBuffer)
      {
        buffer->prepare(kIODirectionOut);
        context->writeBuffer->prepare(kIODirectionIn);
        for (unsigned i = 0, n = context->length / 512; i < n; i++)
        {
          /* read, crypt and write 64 bytes
           */
          UInt64 byteBlock[64];
          (void) buffer->readBytes(i * 512, (UInt8*) byteBlock, 512);
          for (unsigned j = 0; j < 64; j++) byteBlock[j] = ~(byteBlock[j]);
          (void) context->writeBuffer->writeBytes(i * 512, (UInt8*) byteBlock, 512);
        }
        buffer->complete();
        context->writeBuffer->complete();
        /* hang over provider with our buffer
         */
        getProvider()->write(this, byteStart, context->readBuffer, completion);
        return; // succeeded
      }
    }
  }
  /* something goes wrong...
   * hang over provider without encryption?? or should panic?
   */
  (void) EKErrorLog("allocation failed");
  getProvider()->write(this, byteStart, buffer, completion);
}

IOReturn com_enkript_driver_FilterScheme::synchronizeCache(IOService* client)
{
	return getProvider()->synchronizeCache(this);
}

bool com_enkript_driver_FilterScheme::start(IOService* provider)
{
  EKDebugLog("");
  /* cast provider
   */
  IOMedia* providerMedia = OSDynamicCast(IOMedia, provider); // our provider should be a media created by IOApplePartitionScheme
  EKAssert(providerMedia);
  /* start super object
   */
  if (super::start(provider) == false) return EKErrorLog("super::start() failed");
  /* this object handle all encrypted media (typed EnKript_HFS)
   * we need to publish a new media that will represent the uncrypted content
   * the chosen type for this media is Apple_HFS (IOFDiskPartitionScheme)
   */
  IOMedia* media = new IOMedia;
  if (!media) return EKErrorLog("new IOMedia failed");;
  bool initRet = media->init(0 /* base */, providerMedia->getSize() /* size */,
                             providerMedia->getPreferredBlockSize() /* preferredBlockSize */,
                             providerMedia->isEjectable() /* attributes (IOMediaAttributeMask) */,
                             false /* isWhole */, providerMedia->isWritable() /* isWritable */,
                             "Apple_HFS" /* contentHint */);
  if (!initRet) // init failed
  {
    media->release();
    return EKErrorLog("media->init(...) failed");;
  }
  /* initialize partition name
   */
  OSNumber* instanceCount = OSNumber::withNumber(getMetaClass()->getInstanceCount(), 32);
  EKString* id = EKString::withNumber(instanceCount);
  OSString* name = OSString::withCString("EnKript_HFS ");
  EKString* uniqueName = EKString::withStrings(name, id);  
  if (!instanceCount || !id || !name || !uniqueName)
  {
    if (id) id->release();
    if (name) name->release();
    if (uniqueName) uniqueName->release();
    if (instanceCount) instanceCount->release();
    media->release();
    return EKErrorLog("allocations failed");
  }
  media->setName(uniqueName->getCStringNoCopy());
  media->setLocation(id->getCStringNoCopy());
  /* cleanup strings
   */
  id->release();
  name->release();
  uniqueName->release();
  instanceCount->release();  
  /* attach the new media      
   */
  uncryptedMedia = media;
  uncryptedMedia->attach(this);
  /* publish the new media
   */
  uncryptedMedia->registerService();
  
  return true;
}

void com_enkript_driver_FilterScheme::stop(IOService* provider)
{
  EKDebugLog("");
  /* stop super object
   */
  super::stop(provider);
}

IOMedia* com_enkript_driver_FilterScheme::getProvider(void) const
{
//IOMedia* ret = (IOMedia*) super::getProvider();
  IOMedia* ret =  (IOMedia*) IOService::getProvider();
  EKAssert(ret); // should never be NULL
  
  return ret;
}
