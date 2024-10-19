#include "types.h"
#ifndef _DARRAY_H
#define _DARRAY_H

typedef struct DArrayString DArrayString;

DArrayString dArrayStringNew(int capacity);
void dArrayStringInsert(DArrayString *array, String element, int index);
void dArrayStringRemove(DArrayString *array, int index);

struct DArrayString {
  int length;
  int capacity;
  String *data;
};

typedef struct DArrayInt DArrayInt;

DArrayInt dArrayIntNew(int capacity);
void dArrayIntInsert(DArrayInt *array, int element, int index);
void dArrayIntRemove(DArrayInt *array, int index);

struct DArrayInt {
  int length;
  int capacity;
  int *data;
};

#endif

