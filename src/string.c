#include "string.h"
#include "types.h"
#include "common.h"

#include <stdlib.h>

String stringCat(String pre, String post) {
  String new = {0};
  new.length = pre.length + post.length;
  new.value = malloc(sizeof(char) * (pre.length + post.length));
  for (int i = 0; i < new.length; i++) {
    if (i < pre.length) {
      new.value[i] = pre.value[i];
    } else {
      int postIndex = i - pre.length;
      new.value[i] = post.value[postIndex];
    }
  }
  return new;
}

char *stringToNativeTerminated(String str) {
  char *native = malloc(sizeof(char) * (str.length + 1));
  for (int i = 0; i < str.length; i++) {
    native[i] = str.value[i];
  }
  native[str.length] = '\0';
  return native;
}

String stringFromNative(char *native, int length) {
  String str = {0};
  str.length = length;
  str.value = calloc(sizeof(char), length);
  for (int i = 0; i < length; i++) {
    str.value[i] = native[i];
  }
  return str;
}

String stringFromNativeTerminated(char *native) {
  int i = 0;
  while (TRUE) {
    if (native[i] == '\0') {
      break;
    }
  }
  int length = i;
  return stringFromNative(native, length);
}

String stringInsertChar(String s, char c, int i) {
  String newString = {0};
  newString.length = s.length + 1;
  newString.value = malloc(sizeof(char) * newString.length);
  for (int oldStringIndex, newStringIndex = 0; newStringIndex < newString.length; newStringIndex++ && oldStringIndex++) {
    if (i == newStringIndex) {
      newString.value[newStringIndex] = c;
      oldStringIndex--;
      continue;
    }
    newString.value[newStringIndex] = s.value[oldStringIndex];
  }
  return newString;
}

char stringEquals(String a, String b) {
  if (a.length != b.length) {
    return FALSE;
  }

  for (int i = 0; i < a.length; i++) {
    if (a.value[i] != b.value[i]) {
      return FALSE;
    }
  }

  return TRUE;
}

