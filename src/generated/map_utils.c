#include "../string.h"
#include "../types.h"

char mapSIKeyEquals(String a, String b) {
  return stringEquals(a, b);
}

String mapISDefaultValue() {
  String string = {0};
  return string;
}

char mapISKeyEquals(int a, int b) {
  return a == b;
}

int mapSIDefaultValue() {
  return 0;
}

