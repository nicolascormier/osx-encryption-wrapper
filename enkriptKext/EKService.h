/*
 *  EKService.h
 *  enkriptKext
 *
 *  Created by nico on 26/09/08.
 *
 */

#ifndef __ENKRIPTKEXT_EKSERVICE_H
# define __ENKRIPTKEXT_EKSERVICE_H 

# include <IOKit/IOService.h>


class com_enkript_driver_Service : public IOService
{
  OSDeclareDefaultStructors(com_enkript_driver_Service)
  
public:
  /* Override IOService
   */
  virtual bool init(OSDictionary* properties = 0);
  virtual void free(void);
  virtual bool start(IOService* provider);
  virtual void stop(IOService* provider);
  /* Methods called by clients
   */
  virtual IOReturn hello(task_t task);
};

#endif // __ENKRIPTKEXT_EKSERVICE_H