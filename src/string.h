#include "types.h"
#ifndef _STRING_H
#define _STRING_H

String stringCat(String pre, String post);
char *stringToNativeTerminated(String str);
String stringFromNative(char *native, int length);
String stringFromNativeTerminated(char *native);
String stringInsertChar(String s, char c, int i);
char stringEquals(String a, String b);

#endif

