/*
 *  EKStorageCompletion.cpp
 *  enkriptKext
 *
 *  Created by nico on 23/09/08.
 *
 */

#include "EKStorageCompletion.h"
#include "EKDebug.h"

#define super OSObject

//  Implementation
//
//OSDefineMetaClassAndStructors(EKStorageCompletion, OSObject)

bool  EKStorageCompletion::initWithStorageOriginalCompletion(IOStorageCompletion completion)
{
  // initialize super
  if (!super::init()) return EKErrorLog("OSObject::init() failed");
  // initialize members
  originalCompletion = completion;
  
  return true;
}

void  EKStorageCompletion::free(void)
{
  // cleanup members
  //  - nothing to do here...
  // cleanup super
  super::free();
}

void   EKStorageCompletion::completionRoutine(EKStorageCompletion* target, void* /* parameter*/, IOReturn status, UInt64 actualByteCount)
{
  // run specific completion routine
  target->specificCompletion();
  //if (!target->specificCompletion());// (void) EKErrorLog("specificCompletion() failed");  
  // run original completion routine
  if (target->originalCompletion.action)
    (*(target->originalCompletion.action))(target->originalCompletion.target, target->originalCompletion.parameter, status, actualByteCount);
  // clean up
  target->release();
}



