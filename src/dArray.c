#include "dArray.h"

#include <stdlib.h>

DArrayString dArrayStringNew(int capacity) {
  DArrayString array = {0};
  array.length = 0;
  array.capacity = capacity;
  array.data = malloc(sizeof(String) * array.capacity);
  return array;
}

void dArrayStringInsert(DArrayString *array, String element, int index) {
  int oldLength = array->length;
  int newLength = array->length + 1;

  if (newLength > array->capacity) {
    String *arrayData = array->data;
    array->capacity *= 2;
    array->data = malloc(sizeof(String) * array->capacity);
    for (int i = 0; i < oldLength; i++) {
      array->data[i] = arrayData[i];
    }
    free(arrayData);
  }

  for (int i = newLength - 1; i > index; i--) {
    array->data[i] = array->data[i-1];
  }
  array->data[index] = element;
  array->length = newLength;
}

void dArrayStringRemove(DArrayString *array, int index) {
  int oldLength = array->length;
  int newLength = array->length - 1;

  if (newLength < array->capacity/4) {
    String *arrayData = array->data;
    array->capacity /= 2;
    array->data = malloc(sizeof(String) * array->capacity);
    for (int i = 0; i < oldLength; i++) {
      array->data[i] = arrayData[i];
    }
    free(arrayData);
  }

  for (int i = index; i < newLength; i++) {
    array->data[i] = array->data[i+1];
  }
  array->length = newLength;
}

DArrayInt dArrayIntNew(int capacity) {
  DArrayInt array = {0};
  array.length = 0;
  array.capacity = capacity;
  array.data = malloc(sizeof(int) * array.capacity);
  return array;
}

void dArrayIntInsert(DArrayInt *array, int element, int index) {
  int oldLength = array->length;
  int newLength = array->length + 1;

  if (newLength > array->capacity) {
    int *arrayData = array->data;
    array->capacity *= 2;
    array->data = malloc(sizeof(int) * array->capacity);
    for (int i = 0; i < oldLength; i++) {
      array->data[i] = arrayData[i];
    }
    free(arrayData);
  }

  for (int i = newLength - 1; i > index; i--) {
    array->data[i] = array->data[i-1];
  }
  array->data[index] = element;
  array->length = newLength;
}

void dArrayIntRemove(DArrayInt *array, int index) {
  int oldLength = array->length;
  int newLength = array->length - 1;

  if (newLength < array->capacity/4) {
    int *arrayData = array->data;
    array->capacity /= 2;
    array->data = malloc(sizeof(int) * array->capacity);
    for (int i = 0; i < oldLength; i++) {
      array->data[i] = arrayData[i];
    }
    free(arrayData);
  }

  for (int i = index; i < newLength; i++) {
    array->data[i] = array->data[i+1];
  }
  array->length = newLength;
}

