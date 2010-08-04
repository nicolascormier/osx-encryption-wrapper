/*
 *  EKString.cpp
 *  enkriptKext
 *
 *  Created by nico on 25/09/08.
 *
 */

#include <string.h>
#include <libkern/c++/OSNumber.h>

#include "EKString.h"
#include "EKDebug.h"

#define super OSString
OSDefineMetaClassAndStructors(EKString, OSString)


bool EKString::initWithStrings(const OSString* aString, const OSString* anotherString)
{
  if (!aString || !anotherString) return EKErrorLog("invalid arguments");
  /* we skip OSString init call...
   */
  if (!OSObject::init()) return EKErrorLog("OSObject::init() failed");
  /* allocate OSString internal data
   */
  length = aString->getLength() + anotherString->getLength() + 1 /* \0 */;
  string = new char[length];
  if (!string) return EKErrorLog("allocation failed");
  /* initialize string
   */
  bcopy(aString->getCStringNoCopy(), string, aString->getLength());
  bcopy(anotherString->getCStringNoCopy(), string + aString->getLength(), anotherString->getLength() + 1 /* \0 */);
  
  return true;
}

bool EKString::initWithNumber(const OSNumber* number)
{
  if (!number) return EKErrorLog("invalid argument");
  /* we skip OSString init call...
   */
  if (!OSObject::init()) return EKErrorLog("OSObject::init() failed");
  /* allocate OSString internal data
   */
  length = 32; // Dummy size
  string = new char[length];  if (!string) return EKErrorLog("allocation failed");
  /* initialize string
   */
  bool snprintfRet = snprintf(string, length, "%lld", number->unsigned64BitValue()) >= 0;
  if (!snprintfRet) return EKErrorLog("initialize failed");
  
  return true;
}

EKString* EKString::withStrings(const OSString* aString, const OSString* anotherString)
{
  EKString* self = new EKString;
  
  if (self && !self->initWithStrings(aString, anotherString))
  {
    self->release();
    return NULL;
  }
  
  return self;
}

EKString* EKString::withNumber(const OSNumber* number)
{
  EKString* self = new EKString;
  
  if (self && !self->initWithNumber(number))
  {
    self->release();
    return NULL;
  }
  
  return self; 
}
