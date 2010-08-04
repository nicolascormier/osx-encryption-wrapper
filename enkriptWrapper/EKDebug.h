/*
 *  EKDebug.h
 *  enkriptWrapper
 *
 *  Created by nico on 1/10/08.
 *
 */

#import <Cocoa/Cocoa.h>


static inline BOOL  _EKErrorLog(int line, const char* file, const char* function, NSString* format, ...)
{
  va_list	ap;
  va_start(ap, format);
#ifdef EKErrorLogVerbose
  NSLog(@"%s[%s:%d] - %@\n", function, file, line, [[NSString alloc] initWithFormat:format arguments:ap]);
#else // !EKErrorLogVerbose
  NSLog(@"%s - %@\n", function, [[NSString alloc] initWithFormat:format arguments:ap]);
#endif //EKErrorLogVerbose
  va_end(ap);
  
  return NO;
}

# define EKAssert NSAssert

# ifndef DEBUG
#  define EKDebugLog(format, a...) (void)_EKErrorLog(__LINE__, __FILE__, __PRETTY_FUNCTION__, format, ##a)
#  define EKErrorLog(format, a...) _EKErrorLog(__LINE__, __FILE__, __PRETTY_FUNCTION__, format, ##a)
# else // !DEBUG
#  define EKDebugLog(...) 
#  define EKErrorLog(...) false
# endif // DEBUG
