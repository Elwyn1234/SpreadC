#ifndef _TYPES_H
#define _TYPES_H

typedef struct String String;
struct String {
  char *value;
  int length;
};

typedef struct MapNode MapNode;
struct MapNode {
  String key;
  int value;
};

#endif

