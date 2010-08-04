/*
 *  EKFilterScheme.h
 *  enkriptKext
 *
 *  Created by nico on 19/09/08.
 *
 */

#ifndef __ENKRIPTKEXT_EKFILTERSCHEME_H
# define __ENKRIPTKEXT_EKFILTERSCHEME_H 

# include <IOKit/storage/IOMedia.h>
# include <IOKit/storage/IOStorage.h>


class com_enkript_driver_FilterScheme : public IOStorage
{
  OSDeclareDefaultStructors(com_enkript_driver_FilterScheme)
	
protected:
  IOMedia* uncryptedMedia;
  
protected:	
  static void readWriteCompletionRoutine(void* target, void* parameter, IOReturn status, UInt64 actualByteCount);
	/* Override IOStorage
   */
  virtual bool handleOpen(IOService* client, IOOptionBits options, void* access);
  virtual bool handleIsOpen(const IOService* client) const;	
  virtual void handleClose(IOService* client, IOOptionBits options);
  
public:
	/* Override IORegistryEntry
   */
  virtual bool init(OSDictionary* properties = 0);
	/* Override IOStorage
   */
  virtual void read(IOService* client, UInt64 byteStart, IOMemoryDescriptor* buffer, IOStorageCompletion completion);
  virtual void write(IOService* client, UInt64 byteStart, IOMemoryDescriptor* buffer, IOStorageCompletion completion);	
  virtual IOReturn synchronizeCache(IOService* client);
	/* Override IOService
   */
  virtual bool start(IOService* provider);
  virtual void stop(IOService* provider);
  virtual void free(void);
  virtual IOMedia* getProvider(void) const;
	
};

#endif // __ENKRIPTKEXT_EKFILTERSCHEME_H