#include "map.h"
#include "map_utils.h"
#include "../common.h"
#include "../types.h"

MapIS mapISNew(int capacity) {
  MapIS map = {0};
  map.arrayMapNode = dArrayMapNodeISNew(capacity);
  return map;
}

int mapISHasKey(MapIS *map, int key) {
  char success;
  String result = mapISLookup(map, key, &success);
  return success;
}

void mapISInsert(MapIS *map, int key, String value) {
  MapNodeIS node = {0};
  node.key = key;
  node.value = value;
  dArrayMapNodeISInsert(&map->arrayMapNode, node, map->arrayMapNode.length);
}

char mapISRemove(MapIS *map, int key) {
  char success;
  int index = mapISLookupIndex(map, key, &success);
  if (! success) {
    return FALSE;
  }
  dArrayMapNodeISRemove(&map->arrayMapNode, index);
  return TRUE;
}



String mapISLookup(MapIS *map, int key, char *success) {
  for(int i = 0; i < map->arrayMapNode.length; i++) {
    if (mapISKeyEquals(key, map->arrayMapNode.data[i].key)) {
      *success = TRUE;
      return map->arrayMapNode.data[i].value;
    }
  }
  *success = FALSE;
  return mapISDefaultValue();
}

int mapISLookupIndex(MapIS *map, int key, char *success) {
  for(int i = 0; i < map->arrayMapNode.length; i++) {
    if (mapISKeyEquals(key, map->arrayMapNode.data[i].key)) {
      *success = TRUE;
      return i;
    }
  }
  *success = FALSE;
  return 0;
}

MapSI mapSINew(int capacity) {
  MapSI map = {0};
  map.arrayMapNode = dArrayMapNodeStringIntNew(capacity);
  return map;
}

int mapSIHasKey(MapSI *map, String key) {
  char success;
  int result = mapSILookup(map, key, &success);
  return success;
}

void mapSIInsert(MapSI *map, String key, int value) {
  MapNodeStringInt node = {0};
  node.key = key;
  node.value = value;
  dArrayMapNodeStringIntInsert(&map->arrayMapNode, node, map->arrayMapNode.length);
}

char mapSIRemove(MapSI *map, String key) {
  char success;
  int index = mapSILookupIndex(map, key, &success);
  if (! success) {
    return FALSE;
  }
  dArrayMapNodeStringIntRemove(&map->arrayMapNode, index);
  return TRUE;
}



int mapSILookup(MapSI *map, String key, char *success) {
  for(int i = 0; i < map->arrayMapNode.length; i++) {
    if (mapSIKeyEquals(key, map->arrayMapNode.data[i].key)) {
      *success = TRUE;
      return map->arrayMapNode.data[i].value;
    }
  }
  *success = FALSE;
  return mapSIDefaultValue();
}

int mapSILookupIndex(MapSI *map, String key, char *success) {
  for(int i = 0; i < map->arrayMapNode.length; i++) {
    if (mapSIKeyEquals(key, map->arrayMapNode.data[i].key)) {
      *success = TRUE;
      return i;
    }
  }
  *success = FALSE;
  return 0;
}

