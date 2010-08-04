/*
 *  EKStorageOperation.h
 *  enkriptKext
 *
 *  Created by nico on 23/09/08.
 *
 */


#ifndef __ENKRIPTKEXT_EKSTORAGEOPERATION_H
# define __ENKRIPTKEXT_EKSTORAGEOPERATION_H

#include "EKStorageCompletion.h"


class EKStorageOperation : public EKStorageCompletion
{
  OSDeclareDefaultStructors(EKStorageOperation)
  
public:
  virtual bool  initWithStorageOriginalCompletion(IOStorageCompletion completion);
  virtual void  free(void);
  virtual bool  
  
};

#endif // __ENKRIPTKEXT_EKSTORAGEOPERATION_H