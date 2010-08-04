/*
 *  EKString.h
 *  enkriptKext
 *
 *  Created by nico on 25/09/08.
 *
 */

#ifndef __ENKRIPTKEXT_EKSTRING_H
# define __ENKRIPTKEXT_EKSTRING_H 

# include <libkern/c++/OSString.h>

/* Forward decalarations
 */
class OSSNumber;


class EKString : public OSString
{
  OSDeclareDefaultStructors(EKString)
  
public:
  static EKString* withStrings(const OSString* aString, const OSString* anotherString);
  static EKString* withNumber(const OSNumber* number);
  //static EKString* withFormat(const char* format, ...);
  virtual bool initWithStrings(const OSString* aString, const OSString* anotherString);
  virtual bool initWithNumber(const OSNumber* number);
  //virtual bool initWithFormat(const char* format, ...);
};

#endif // __ENKRIPTKEXT_EKSTRING_H
