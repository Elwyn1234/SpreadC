#ifndef _TYPES_H
#define _TYPES_H

typedef struct String String;
struct String {
  char *value;
  int length;
};

typedef struct MapNodeStringInt MapNodeStringInt;
struct MapNodeStringInt {
  String key;
  int value;
};

typedef struct MapNodeIS MapNodeIS;
struct MapNodeIS {
  int key;
  String value;
};

#endif

