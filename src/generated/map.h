#include "../types.h"
#include "dArray.h"
#ifndef _MAP_H
#define _MAP_H

typedef struct MapIS MapIS;
struct MapIS {
  DArrayMapNodeIS arrayMapNode;
};

// MapIS mapNew(int capacity);
// int mapLookup(MapIS *map, int key);
// int mapHasKey(MapIS *map, int key);
// void mapInsert(MapIS *map, int key, String value);
// int mapLookupIndex(MapIS *map, int key);
// int mapRemove(MapIS *map, int key);

MapIS mapISNew(int capacity);
int mapISHasKey(MapIS *map, int key);
void mapISInsert(MapIS *map, int key, String value);
char mapISRemove(MapIS *map, int key);
String mapISLookup(MapIS *map, int key, char *success);
int mapISLookupIndex(MapIS *map, int key, char *success);

typedef struct MapSI MapSI;
struct MapSI {
  DArrayMapNodeStringInt arrayMapNode;
};

// MapSI mapNew(int capacity);
// int mapLookup(MapSI *map, String key);
// int mapHasKey(MapSI *map, String key);
// void mapInsert(MapSI *map, String key, int value);
// int mapLookupIndex(MapSI *map, String key);
// int mapRemove(MapSI *map, String key);

MapSI mapSINew(int capacity);
int mapSIHasKey(MapSI *map, String key);
void mapSIInsert(MapSI *map, String key, int value);
char mapSIRemove(MapSI *map, String key);
int mapSILookup(MapSI *map, String key, char *success);
int mapSILookupIndex(MapSI *map, String key, char *success);

#endif

