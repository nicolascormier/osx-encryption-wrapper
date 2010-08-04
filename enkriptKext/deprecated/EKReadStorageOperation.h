/*
 *  EKReadStorageOperation.h
 *  enkriptKext
 *
 *  Created by nico on 23/09/08.
 *
 */

#ifndef __ENKRIPTKEXT_EKREADSTORAGEOPERATION_H
# define __ENKRIPTKEXT_EKREADSTORAGEOPERATION_H

#include "EKStorageOperation.h"


class EKReadStorageOperation : public EKStorageOperation
  {
    OSDeclareDefaultStructors(EKReadStorageOperation)
    
  protected:
    IOMemoryDescriptor* buffer;
    
  protected:
    virtual bool specificCompletion(void);
    
  public:
    static EKReadStorageOperation* withMemoryDescriptorAndOriginalCompletion(IOMemoryDescriptor* buffer, IOStorageCompletion completion);
    virtual bool initWithMemoryDescriptorAndOriginalCompletion(IOMemoryDescriptor* buffer, IOStorageCompletion completion);
    virtual void free(void);
    virtual bool read(void);
  };

#endif // __ENKRIPTKEXT_EKREADSTORAGEOPERATION_H
