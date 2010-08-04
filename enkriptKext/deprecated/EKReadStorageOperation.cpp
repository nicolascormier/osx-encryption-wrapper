/*
 *  EKReadStorageOperation.cpp
 *  enkriptKext
 *
 *  Created by nico on 23/09/08.
 *
 */

#include "EKReadStorageOperation.h"
#include "EKDebug.h"

#define super EKStorageOperation

//   Implementation
//
OSDefineMetaClassAndStructors(EKReadStorageOperation, EKStorageOperation)

EKReadStorageOperation*  EKReadStorageOperation::withMemoryDescriptorAndOriginalCompletion(IOMemoryDescriptor* buffer, IOStorageCompletion completion)
{
  EKReadStorageOperation* self = new EKReadStorageOperation;
  if (!self)
  {
    (void) EKErrorLog("allocation failed");
    return NULL;
  }
  if (!self->initWithMemoryDescriptorAndOriginalCompletion(buffer, completion))
  {
    self->free();
    (void) EKErrorLog("initWithMemoryDescriptor() failed");
    return NULL;
  }
  
  return self;
}

bool  EKReadStorageOperation::initWithMemoryDescriptorAndOriginalCompletion(IOMemoryDescriptor* buffer, IOStorageCompletion completion)
{
  // check parameters
  if (!buffer) return EKErrorLog("invalid parameter, buffer should not be null");
  // initialize super
  if (!super::initWithStorageOriginalCompletion(completion)) return EKErrorLog("EKSimpleCryptoDiskContext::init() failed");
  // initialize members
  this->buffer = buffer;
  this->buffer->retain();
  
  return true;
}

void  EKReadStorageOperation::free(void)
{
  // clean up members
  buffer->release();
  buffer = NULL;
  // clean up super
  super::free();
}

bool  EKReadStorageOperation::specificCompletion(void)
{
  // do decrytion
  
  return false;
}

bool  EKReadStorageOperation::read(void)
{
  // delayed after completion
  
  return false;
}
