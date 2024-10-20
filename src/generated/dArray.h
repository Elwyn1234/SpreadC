#include "../types.h"
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

typedef struct DArrayMapNodeStringInt DArrayMapNodeStringInt;

DArrayMapNodeStringInt dArrayMapNodeStringIntNew(int capacity);
void dArrayMapNodeStringIntInsert(DArrayMapNodeStringInt *array, MapNodeStringInt element, int index);
void dArrayMapNodeStringIntRemove(DArrayMapNodeStringInt *array, int index);

struct DArrayMapNodeStringInt {
  int length;
  int capacity;
  MapNodeStringInt *data;
};

typedef struct DArrayMapNodeIS DArrayMapNodeIS;

DArrayMapNodeIS dArrayMapNodeISNew(int capacity);
void dArrayMapNodeISInsert(DArrayMapNodeIS *array, MapNodeIS element, int index);
void dArrayMapNodeISRemove(DArrayMapNodeIS *array, int index);

struct DArrayMapNodeIS {
  int length;
  int capacity;
  MapNodeIS *data;
};

#endif

