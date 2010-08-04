/*
 *  EKStorageCompletion.h
 *  enkriptKext
 *
 *  Created by nico on 23/09/08.
 *
 */

#ifndef __ENKRIPTKEXT_EKSTORAGECOMPLETION_H
# define __ENKRIPTKEXT_EKSTORAGECOMPLETION_H

# include <IOKit/storage/IOStorage.h>


// IOStorage wrapper for our purpose
//
class EKStorageCompletion : public OSObject
{
    OSDeclareDefaultStructors(EKStorageCompletion)
    
  protected:
    IOStorageCompletion originalCompletion;
    
  protected:
    virtual bool  specificCompletion(void) = 0;
    
  public:
    virtual bool  initWithStorageOriginalCompletion(IOStorageCompletion completion);
    virtual void  free(void);
    static void   completionRoutine(EKStorageCompletion* target, void* /* parameter*/, IOReturn status, UInt64 actualByteCount);
};

#endif // __ENKRIPTKEXT_EKSTORAGECOMPLETION_H

