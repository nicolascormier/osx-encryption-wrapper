/*
 *  EKDebug.h
 *  enkriptKext
 *
 *  Created by nico on 19/09/08.
 *
 */

#ifndef __ENKRIPTKEXT_EKDEBUG_H
# define __ENKRIPTKEXT_EKDEBUG_H 

# include <IOKit/IOLib.h>
# include <IOKit/assert.h>


namespace
{
# define EKErrorLogMaxBufSize  256
//# define EKErrorLogVerbose
  static inline bool  _EKErrorLog(int line, const char* file, const char* function, const char* format, ...)
  {
    char buf[EKErrorLogMaxBufSize];
    va_list	ap;
    va_start(ap, format);
    (void) vsnprintf(buf, EKErrorLogMaxBufSize, format, ap);
    va_end(ap);
#ifdef EKErrorLogVerbose
    IOLog("%s[%s:%d] - %s\n", function, file, line, buf);
#else // !EKErrorLogVerbose
    IOLog("%s - %s\n", function, buf);
#endif //EKErrorLogVerbose
    
    return false;
  }
}

# define EKAssert assert

# ifndef DEBUG
#  define EKDebugLog(format, a...) (void)_EKErrorLog(__LINE__, __FILE__, __PRETTY_FUNCTION__, format, ##a)
#  define EKErrorLog(format, a...) _EKErrorLog(__LINE__, __FILE__, __PRETTY_FUNCTION__, format, ##a)
# else // !DEBUG
#  define EKDebugLog(...) 
#  define EKErrorLog(...) false
# endif // DEBUG

#endif // __ENKRIPTKEXT_EKDEBUG_H
