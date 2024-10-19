#include "types.h"
#include "dArray.h"

#include "cairo.h"
#include "pango/pango-font.h"
#include "pango/pango-glyph.h"
#include "pango/pango-layout.h"
#include "pango/pango-renderer.h"
#include <X11/X.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <unistd.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>
#include <cairo/cairo-xlib.h>
// I have manually copied pango into /usr/include/. The same may have to be done for glib. It seems that xlibs font rendering uses X's core font rendering (at least by default) which is not modern and doesn't do things like anti-aliasing (I think) and which is why I am looking to pango. Pango builds on Xft which is also apparently not that modern. Sounds like pango is the go to low level library for font rendering.
#define BACKSPACE                     0x0008
#define TAB                           0x0009
#define LINEFEED                      0x000a
#define CLEAR                         0x000b
#define RETURN                        0x000d
#define PAUSE                         0x0013
#define SCROLL_LOCK                   0x0014
#define SYS_REQ                       0x0015
#define ESCAPE                        0x001b
#define DELETE                        0x00ff

typedef struct Viewport Viewport;
typedef struct Sheet Sheet;
typedef struct Color Color;
typedef char Boolean;
typedef struct String String; 
typedef struct MapNode MapNode; 
typedef struct Map Map; 
typedef enum NORMAL_MODE_ACTION NORMAL_MODE_ACTION;
typedef enum NORMAL_MODE_STATE NORMAL_MODE_STATE;
typedef enum MOTION MOTION;
typedef struct Action Action;
typedef struct Program Program;
typedef struct DynamicArrayMapNode DynamicArrayMapNode;

PangoLayout *initialisePangoLayout(Program program);
int clamp(int number, int lower, int higher);
int calculateMaxRowsOnScreen(XWindowAttributes winAttribs, int cellHeightPixels);
int calculateMaxColumnsOnScreen(XWindowAttributes winAttribs, int cellHeightPixels);
int calculateTextHeightPixels(PangoLayout *layout);
int calculateTextWidthPixels(PangoLayout *layout);
int calculateCellHeightPixels(int textHeightPixels, Sheet sheet);
int calculateCellWidthPixels(int textHeightPixels, Sheet sheet);
char stringEquals(String a, String b);
char *stringToNativeTerminated(String str);
String stringInsertChar(String s, char c, int i);
String stringCat(String pre, String post);

Program program;

struct Color {
  double red;
  double blue;
  double green;
  double alpha;
};

enum NORMAL_MODE_STATE {
  NMS_MOTION = 1,
  NMS_COUNT,
  NMS_ACTION,
  /*COUNT_ACTION,*/
  /*COUNT_ACTION_MOTION,*/
  /*ACTION_MOTION,*/
};

enum NORMAL_MODE_ACTION {
  COLUMN_LEFT = 1,
  COLUMN_RIGHT,
  ROW_UP,
  ROW_DOWN,
  SELECTED_CELL_CENTRE,
  SELECTED_CELL_TOP,
  SELECTED_CELL_BOTTOM,
  SELECTED_CELL_BEGINNING,
  SELECTED_CELL_END,
  VIEWPORT_ROW_UP,
  VIEWPORT_ROW_DOWN,
  VIEWPORT_COLUMN_LEFT,
  VIEWPORT_COLUMN_RIGHT,
  VIEWPORT_COLUMN_LEFT_HALF_PAGE,
  VIEWPORT_COLUMN_RIGHT_HALF_PAGE,
  VIEWPORT_PAGE_UP,
  VIEWPORT_PAGE_DOWN,
  VIEWPORT_HALF_PAGE_UP,
  VIEWPORT_HALF_PAGE_DOWN,
  VIEWPORT_GO_TO,
  INSERT_BEGIN,
  INSERT_TEXT,
  APPEND_COLUMN,
  INSERT_COLUMN,
  APPEND_ROW,
  INSERT_ROW,
  DELETE_ROW,
};

enum MOTION {
  MTN_SHEET_TOP = 1,
  MTN_ROW,
  MTN_COLUMN,
  MTN_ROW_UP,
  MTN_ROW_DOWN,
  MTN_COLUMN_LEFT,
  MTN_COLUMN_RIGHT,
};

struct Action {
  NORMAL_MODE_ACTION action;
  int count;
  MOTION motion;
  String text;
};

struct Program {
  Display *display;
  Window window;
  GC gc;
  cairo_surface_t *surface;
  cairo_t *cr;
  PangoFontDescription *font;
  XColor background;
  XColor highlight;
  XColor foreground;
  Color text;

  NORMAL_MODE_STATE normalModeState;
  Action lastAction;
  Action currentAction;
};



//******************************************//
//                   Util                   //
//******************************************//

void assert_lt_int(int value, int expected, char *assertionMessage) {
  if (value >= expected) {
    printf("[ASSERT] %s\n", assertionMessage);
    exit(1);
  }
}

void assert_gt_int(int value, int expected, char *assertionMessage) {
  if (value <= expected) {
    printf("[ASSERT] %s\n", assertionMessage);
    exit(1);
  }
}

void assert_equal_int(int value, int expected, char *assertionMessage) {
  if (value != expected) {
    printf("[ASSERT] %s\n", assertionMessage);
    exit(1);
  }
}

void assert_zeroed(char *value, int length, char *assertionMessage) {
  for (int i = 0; i < length; i++) {
    if (value[i] != 0) {
      printf("[ASSERT] %s\n", assertionMessage);
      exit(1);
    }
  }
}

void assert(char value, char *assertionMessage) {
  if (! value) {
    printf("[ASSERT] %s\n", assertionMessage);
    exit(1);
    // TODO: NEVER ASSERT IF PRODUCTION
    // TODO: WRITE CUSTOM EXIT FUNCTION
    // TODO: HOW TO PRINT STACK TRACE
    // TODO: HOW TO CORE DUMP
  }
}

char keyToUpper(char key) {
  if (key > 96 && key < 123) {
    return key - 32;
  }
  return key; // TODO
}

String stringLeftPad(String s, char c, int count) {
  String new = {0};
  new.length = s.length + count;
  new.value = malloc(sizeof(char) * (new.length));
  for (int i = 0; i < count; i++) {
    new.value[i] = c;
  }
  for (int i = count; i < new.length; i++) {
    new.value[i] = s.value[i - count];
  }
  return new;
}

String stringLeftAlign(String s, char c, int size) {
  if (s.length >= size) {
    return s;
  }
  int padAmount = size - s.length;
  return stringLeftPad(s, c, padAmount);
}

String stringTruncate(String s, int length) {
  s.length = length;
  return s;
}

String stringFromLong(long number) {
  String string = {0};
  string.length = 1;
  int a = number;
  while (a > 9) {
    string.length++;
    a /= 10;
  }
  string.value = malloc(sizeof(char) * string.length);
  for (int i = string.length - 1; i >= 0; i--) {
    char character = number % 10;
    character += 48;
    string.value[i] = character;
    number /= 10;
  }
  return string;
}

String intToLetters(int number) {
  String string = {0};
  string.length = 1;
  int a = number;
  while (a > 25) {
    string.length++;
    a /= 26;
  }
  string.value = malloc(sizeof(char) * string.length);
  for (int i = string.length - 1; i >= 0; i--) {
    char character = number % 26;
    character += 65;
    string.value[i] = character;
    number /= 26;
  }
  return string;
}

int clampToValue(int number, int lower, int higher, int value) {
  if (number < lower) {
    return value;
  }
  if (number > higher) {
    return value;
  }
  return number;
}

int clamp(int number, int lower, int higher) {
  if (number < lower) {
    return lower;
  }
  if (number > higher) {
    return higher;
  }
  return number;
}

Boolean stringsEqual(char *str, int strLength, char *otherStr) {
  for (int i = 0; i < strLength; i++) {
    if (otherStr[i] == '\0')
      return FALSE;
    if (str[i] != otherStr[i])
      return FALSE;
  }
  return TRUE;
}

char keyPressedToChar(char *keyPressed) {
  if (stringsEqual("period", 6, keyPressed)) return '.';
  return keyPressed[0];
}

char *tsFormat(struct timespec tsPre, struct timespec tsPost) {
  time_t sTime = tsPost.tv_sec - tsPre.tv_sec;
  time_t nsTime = sTime == 0 ? (tsPost.tv_nsec - tsPre.tv_nsec) : tsPost.tv_nsec;
  String sString = stringFromLong(sTime);
  String nsString = stringFromLong(nsTime);
  nsString = stringLeftAlign(nsString, '0', 9);
  String msString = stringTruncate(nsString, 3);

  String x = { "s.", 2 };
  String y = { "ms", 2 };
  String formatted = stringCat(sString, x);
  formatted = stringCat(formatted, msString);
  formatted = stringCat(formatted, y);
  
  char *formattedNative = stringToNativeTerminated(formatted);

  return formattedNative;
}



//////////////////////////////////////////////
// Map
//////////////////////////////////////////////

struct MapNode {
  String key;
  int value;
};

struct DynamicArrayMapNode {
  int length;
  int capacity;
  MapNode *data;
};

DynamicArrayMapNode dynamicArrayMapNodeNew(int capacity) {
  DynamicArrayMapNode array = {0};
  array.length = 0;
  array.capacity = capacity;
  array.data = malloc(sizeof(MapNode) * array.capacity);
  return array;
}

void dynamicArrayMapNodeInsert(DynamicArrayMapNode *array, MapNode element, int index) {
  int oldLength = array->length;
  int newLength = array->length + 1;
  if (newLength > array->capacity) {
    MapNode *arrayData = array->data;
    array->capacity *= 2;
    array->data = malloc(sizeof(MapNode) * array->capacity);
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

void dynamicArrayMapNodeRemove(DynamicArrayMapNode *array, int index) {
  int oldLength = array->length;
  int newLength = array->length - 1;
  if (newLength < array->capacity/4) {
    MapNode *arrayData = array->data;
    array->capacity /= 2;
    array->data = malloc(sizeof(MapNode) * array->capacity);
    for (int i = 0; i < oldLength; i++) {
      array->data[i] = arrayData[i];
    }
  }
  for (int i = index; i < newLength; i++) {
    array->data[i] = array->data[i+1];
  }
  array->length = newLength;
}



struct Map {
  DynamicArrayMapNode arrayMapNode;
};

Map mapNew(int capacity) {
  Map map = {0};
  map.arrayMapNode = dynamicArrayMapNodeNew(capacity);
  return map;
}

int mapLookup(Map *map, String key) {
  for(int i = 0; i < map->arrayMapNode.length; i++) {
    if (stringEquals(key, map->arrayMapNode.data[i].key)) {
      return map->arrayMapNode.data[i].value;
    }
  }
  return INT_MAX;
}

int mapHasKey(Map *map, String key) {
  int result = mapLookup(map, key);
  if (result == INT_MAX) {
    return FALSE;
  } else {
    return TRUE;
  }
}

void mapInsert(Map *map, String key, int value) {
  MapNode node = {0};
  node.key = key;
  node.value = value;
  dynamicArrayMapNodeInsert(&map->arrayMapNode, node, map->arrayMapNode.length);
}

int mapLookupIndex(Map *map, String key) {
  for(int i = 0; i < map->arrayMapNode.length; i++) {
    if (stringEquals(key, map->arrayMapNode.data[i].key)) {
      return i;
    }
  }
  return INT_MAX;
}

int mapRemove(Map *map, String key) {
  int index = mapLookupIndex(map, key);
  if (index == INT_MAX) {
    return 1;
  }
  dynamicArrayMapNodeRemove(&map->arrayMapNode, index);
  return 0;
}

//******************************************//
//                  String                  //
//******************************************//

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
  String shorter;
  String longer;
  if (a.length > b.length) {
    longer = a;
    shorter = b;
  } else {
    longer = b;
    shorter = a;
  }

  for (int i = 0; i < shorter.length; i++) {
    if (shorter.value[i] != longer.value[i]) {
      return FALSE;
    }
  }

  return TRUE;
}



//******************************************//
//               Sheet                      //
//******************************************//

struct Viewport {
  int anchorRow;
  int anchorColumn;
  /*int maxCellCountTall;*/
  /*int maxCellCountWide;*/
  /*Boolean hasFocus;*/
};

struct Sheet {
  DArrayString strings;
  int columnCount;
  int rowCount;

  // Split fields into SheetCursor struct
  int selectedCell;
  Boolean insertMode;

  // Split fields into SheetFormat struct
  DArrayInt cellWidths;
  DArrayInt cellHeights;
  int verticalPadding;
  int horizontalPadding;
};

const int TEMP_CELL_WIDTH = 15;
const int TEMP_CELL_HEIGHT = 3;
const float TEMP_TEXT_SCALE = 0.5;

Sheet newSheet(int rowCount, int columnCount) {
  Sheet sheet = {0};
  int cellCount = columnCount * rowCount;
  sheet.cellWidths = dArrayIntNew(columnCount + 100);
  sheet.cellHeights = dArrayIntNew(rowCount + 1000);
  sheet.strings = dArrayStringNew(cellCount + 1000);
  for (int i = 0; i < cellCount; i++) {
    String string = {0};
    dArrayStringInsert(&sheet.strings, string, i);
  }
  for (int i = 0; i < columnCount; i++) {
    dArrayIntInsert(&sheet.cellWidths, TEMP_CELL_WIDTH, i);
  }
  for (int i = 0; i < rowCount; i++) {
    dArrayIntInsert(&sheet.cellHeights, TEMP_CELL_HEIGHT, i);
  }
  sheet.verticalPadding = 4;
  sheet.horizontalPadding = 4;
  sheet.columnCount = columnCount;
  sheet.rowCount = rowCount;
  return sheet;
}

void sheetDeleteRow(Sheet *sheet, int cellIndex) { // TODO: test
  int row = cellIndex / sheet->columnCount;
  int startCellOfRow = row * sheet->columnCount;
  int startCellOfNextRow = startCellOfRow + sheet->columnCount;
  dArrayIntRemove(&sheet->cellHeights, row);
  for (int i = startCellOfNextRow - 1; i >= startCellOfRow; i--) {
    printf("[REMOVE ME] i: %d\n", i);
    dArrayStringRemove(&sheet->strings, i);
  }
  sheet->rowCount --;
}

// TODO: Make this function like sheetInsertRow and rename both this and sheetAppendColumn
void sheetAppendRow(Sheet *sheet, int cellIndex) { // TODO: test
  printf("[REMOVE ME] before\n");
  int selectedRow = cellIndex / sheet->columnCount;
  int startCellOfNewRow = selectedRow * sheet->columnCount;
  int startCellOfNextRow = startCellOfNewRow + sheet->columnCount;
  printf("[REMOVE ME] before\n");
  dArrayIntInsert(&sheet->cellHeights, TEMP_CELL_HEIGHT, selectedRow);
  printf("[REMOVE ME] after\n");
  for (int i = startCellOfNewRow; i < startCellOfNextRow; i++) {
    String string = {0};
    dArrayStringInsert(&sheet->strings, string, i);
  }
  sheet->rowCount++;
}

void sheetAppendColumn(Sheet *sheet, int cellIndex) { // TODO: debug
  int selectedColumn = cellIndex % sheet->columnCount;
  dArrayIntInsert(&sheet->cellWidths, TEMP_CELL_WIDTH, selectedColumn); // TODO: is this insertion correct, same for sheetAppendRow
  sheet->columnCount++;
  for (int row = 0; row < sheet->rowCount; row++) {
    String string = {0};
    dArrayStringInsert(&sheet->strings, string, row * sheet->columnCount + selectedColumn);
  }
}

void sheetCellBackSpace(Sheet sheet, int cellIndex, int stringIndex) {
  for (int i = stringIndex; i < sheet.strings.data[cellIndex].length; i++) {
    if (i + 1 >= sheet.strings.data[cellIndex].length)
      break;
    sheet.strings.data[cellIndex].value[i] = sheet.strings.data[cellIndex].value[i + 1];
  }
  sheet.strings.data[cellIndex].length--;
}

void sheetCellAppend(Sheet sheet, int cellIndex, char* valueToInsert, int valueToInsertLength) {
  String str = {0};
  str.value = malloc(sizeof(char) * valueToInsertLength + sizeof(char) * sheet.strings.data[cellIndex].length);
  for (int i = 0; i < sheet.strings.data[cellIndex].length; i++)
    str.value[i] = sheet.strings.data[cellIndex].value[i];
  for (int i = 0; i < valueToInsertLength; i++)
    str.value[sheet.strings.data[cellIndex].length + i] = valueToInsert[i];
  str.length = sheet.strings.data[cellIndex].length + valueToInsertLength;

  sheet.strings.data[cellIndex] = str;
}



////////////////////////////////////////////////////////////////////////
// NORMAL MODE ACTIONS
////////////////////////////////////////////////////////////////////////
void normalModeAction(Sheet *sheet, Viewport *viewport, Action action) {
  switch (action.action) {
    case COLUMN_LEFT: {
      if (sheet->selectedCell % sheet->columnCount > 0)
        sheet->selectedCell -= 1;
      break;
    }
    case ROW_DOWN: {
      if (sheet->selectedCell / sheet->columnCount >= sheet->rowCount - 1)
        break;

      PangoLayout *layout = initialisePangoLayout(program);
      int textHeightPixels = calculateTextHeightPixels(layout);
      int cellHeightPixels = calculateCellHeightPixels(textHeightPixels, *sheet);
      XWindowAttributes winAttribs = {0};
      XGetWindowAttributes(program.display, program.window, &winAttribs);
      int maxRowsOnScreen = calculateMaxRowsOnScreen(winAttribs, cellHeightPixels);

      sheet->selectedCell += sheet->columnCount;
      int selectedRow = sheet->selectedCell / sheet->columnCount;
      // selectedToAnchor is the range from the selectedRow to the anchorRow. We add one to this because the range is inclusive
      int selectedToAnchor = selectedRow - viewport->anchorRow + 1;
      if (selectedToAnchor > maxRowsOnScreen) {
        viewport->anchorRow ++;
      }
      break;
    }
    case ROW_UP: {
      if (sheet->selectedCell / sheet->columnCount == 0)
        break;
      sheet->selectedCell -= sheet->columnCount;
      int selectedRow = sheet->selectedCell / sheet->columnCount;
      if (viewport->anchorRow > selectedRow) {
        viewport->anchorRow --;
      }
      break;
    }
    case COLUMN_RIGHT: {
      if (sheet->selectedCell % sheet->columnCount < sheet->columnCount - 1)
        sheet->selectedCell += 1;
      break;
    }
    case SELECTED_CELL_CENTRE: {
      PangoLayout *layout = initialisePangoLayout(program);
      int textHeightPixels = calculateTextHeightPixels(layout);
      int cellHeightPixels = calculateCellHeightPixels(textHeightPixels, *sheet);
      XWindowAttributes winAttribs = {0};
      XGetWindowAttributes(program.display, program.window, &winAttribs);
      int maxRowsOnScreen = calculateMaxRowsOnScreen(winAttribs, cellHeightPixels);

      /*int topmostRow = clamp(viewport->anchorRow, 0, sheet->rowCount);*/
      int topmostRow = viewport->anchorRow;
      sheet->selectedCell = topmostRow * sheet->columnCount + maxRowsOnScreen / 2 * sheet->columnCount + sheet->selectedCell % sheet->columnCount;
      break;
    }
    case SELECTED_CELL_TOP: {
      /*int topmostRow = clamp(viewport->anchorRow, 0, sheet->rowCount);*/
      int topmostRow = viewport->anchorRow;
      int columnShift = sheet->selectedCell % sheet->columnCount;
      sheet->selectedCell = topmostRow * sheet->columnCount + columnShift;
      break;
    }
    case SELECTED_CELL_BOTTOM: {
      PangoLayout *layout = initialisePangoLayout(program);
      int textHeightPixels = calculateTextHeightPixels(layout);
      int cellHeightPixels = calculateCellHeightPixels(textHeightPixels, *sheet);
      XWindowAttributes winAttribs = {0};
      XGetWindowAttributes(program.display, program.window, &winAttribs);
      int maxRowsOnScreen = calculateMaxRowsOnScreen(winAttribs, cellHeightPixels);

      /*int topmostRow = clamp(viewport->anchorRow, 0, sheet->rowCount);*/
      int topmostRow = viewport->anchorRow;
      int rowShift = maxRowsOnScreen * sheet->columnCount - sheet->columnCount;
      int columnShift = sheet->selectedCell % sheet->columnCount;
      sheet->selectedCell = topmostRow * sheet->columnCount + rowShift + columnShift;
      break;
    }
    case SELECTED_CELL_BEGINNING: {
      sheet->selectedCell -= sheet->selectedCell % sheet->columnCount;
      break;
    }
    case SELECTED_CELL_END: {
      sheet->selectedCell += sheet->columnCount - 1 - sheet->selectedCell % sheet->columnCount;
      break;
    }
    case VIEWPORT_ROW_UP: {
      viewport->anchorRow --;
      break;
    }
    case VIEWPORT_ROW_DOWN: {
      viewport->anchorRow ++;
      break;
    }
    case VIEWPORT_COLUMN_RIGHT: {
      viewport->anchorColumn ++;
      break;
    }
    case VIEWPORT_COLUMN_LEFT: {
      viewport->anchorColumn --;
      break;
    }
    case VIEWPORT_COLUMN_LEFT_HALF_PAGE: {
      PangoLayout *layout = initialisePangoLayout(program);
      int textWidthPixels = calculateTextWidthPixels(layout);
      int cellWidthPixels = calculateCellWidthPixels(textWidthPixels, *sheet);
      XWindowAttributes winAttribs = {0};
      XGetWindowAttributes(program.display, program.window, &winAttribs);
      int maxColumnsOnScreen = calculateMaxColumnsOnScreen(winAttribs, cellWidthPixels);

      viewport->anchorColumn -= maxColumnsOnScreen / 2;

      int selectedColumn = viewport->anchorColumn + maxColumnsOnScreen - 1;
      selectedColumn = clamp(selectedColumn, 0, sheet->columnCount);
      int selectedRow = sheet->selectedCell / sheet->columnCount;
      sheet->selectedCell = selectedRow * sheet->columnCount + selectedColumn;

      break;
    }
    case VIEWPORT_COLUMN_RIGHT_HALF_PAGE: {
      PangoLayout *layout = initialisePangoLayout(program);
      int textWidthPixels = calculateTextWidthPixels(layout);
      int cellWidthPixels = calculateCellWidthPixels(textWidthPixels, *sheet);
      XWindowAttributes winAttribs = {0};
      XGetWindowAttributes(program.display, program.window, &winAttribs);
      int maxColumnsOnScreen = calculateMaxColumnsOnScreen(winAttribs, cellWidthPixels);

      viewport->anchorColumn += maxColumnsOnScreen / 2;

      int selectedColumn = viewport->anchorColumn;
      selectedColumn = clamp(selectedColumn, 0, sheet->columnCount);
      int selectedRow = sheet->selectedCell / sheet->columnCount;
      sheet->selectedCell = selectedRow * sheet->columnCount + selectedColumn;

      break;
    }
    case VIEWPORT_PAGE_UP: {
      PangoLayout *layout = initialisePangoLayout(program);
      int textHeightPixels = calculateTextHeightPixels(layout);
      int cellHeightPixels = calculateCellHeightPixels(textHeightPixels, *sheet);
      XWindowAttributes winAttribs = {0};
      XGetWindowAttributes(program.display, program.window, &winAttribs);
      int maxRowsOnScreen = calculateMaxRowsOnScreen(winAttribs, cellHeightPixels);
      viewport->anchorRow -= maxRowsOnScreen;

      int rowShift = maxRowsOnScreen * sheet->columnCount - sheet->columnCount;
      int columnShift = sheet->selectedCell % sheet->columnCount;
      /*int topmostRow = clamp(viewport->anchorRow, 0, sheet->rowCount);*/
      int topmostRow = viewport->anchorRow;
      sheet->selectedCell = topmostRow * sheet->columnCount + rowShift + columnShift;
      break;
    }
    case VIEWPORT_PAGE_DOWN: {
      PangoLayout *layout = initialisePangoLayout(program);
      int textHeightPixels = calculateTextHeightPixels(layout);
      int cellHeightPixels = calculateCellHeightPixels(textHeightPixels, *sheet);
      XWindowAttributes winAttribs = {0};
      XGetWindowAttributes(program.display, program.window, &winAttribs);
      int maxRowsOnScreen = calculateMaxRowsOnScreen(winAttribs, cellHeightPixels);
      viewport->anchorRow += maxRowsOnScreen;

      /*int topmostRow = clamp(viewport->anchorRow, 0, sheet->rowCount);*/
      int topmostRow = viewport->anchorRow;
      sheet->selectedCell = topmostRow * sheet->columnCount + sheet->selectedCell % sheet->columnCount;
      break;
    }
    case VIEWPORT_HALF_PAGE_UP: {
      PangoLayout *layout = initialisePangoLayout(program);
      int textHeightPixels = calculateTextHeightPixels(layout);
      int cellHeightPixels = calculateCellHeightPixels(textHeightPixels, *sheet);
      XWindowAttributes winAttribs = {0};
      XGetWindowAttributes(program.display, program.window, &winAttribs);
      int maxRowsOnScreen = calculateMaxRowsOnScreen(winAttribs, cellHeightPixels);

      viewport->anchorRow -= maxRowsOnScreen / 2;
      if (maxRowsOnScreen % 2 == 1) {
        viewport->anchorRow --;
      }

      int selectedRow = sheet->selectedCell / sheet->columnCount;
      // selectedToAnchor is the range from the selectedRow to the anchorRow. We add one to this because the range is inclusive
      int selectedToAnchor = selectedRow - viewport->anchorRow + 1;
      if (selectedToAnchor > maxRowsOnScreen) {
        int rowShift = maxRowsOnScreen * sheet->columnCount - sheet->columnCount;
        int columnShift = sheet->selectedCell % sheet->columnCount;
        /*int topmostRow = clamp(viewport->anchorRow, 0, sheet->rowCount);*/
        int topmostRow = viewport->anchorRow;
        sheet->selectedCell = topmostRow * sheet->columnCount + rowShift + columnShift;
      }

      break;
    }
    case VIEWPORT_HALF_PAGE_DOWN: {
      PangoLayout *layout = initialisePangoLayout(program);
      int textHeightPixels = calculateTextHeightPixels(layout);
      int cellHeightPixels = calculateCellHeightPixels(textHeightPixels, *sheet);
      XWindowAttributes winAttribs = {0};
      XGetWindowAttributes(program.display, program.window, &winAttribs);
      int maxRowsOnScreen = calculateMaxRowsOnScreen(winAttribs, cellHeightPixels);

      viewport->anchorRow += maxRowsOnScreen / 2;
      if (maxRowsOnScreen % 2 == 1) {
        viewport->anchorRow ++;
      }

      int selectedRow = sheet->selectedCell / sheet->columnCount;
      if (viewport->anchorRow > selectedRow) {
        /*int topmostRow = clamp(viewport->anchorRow, 0, sheet->rowCount);*/
        int topmostRow = viewport->anchorRow;
        sheet->selectedCell = topmostRow * sheet->columnCount + sheet->selectedCell % sheet->columnCount;
      }
      break;
    }
    case VIEWPORT_GO_TO: {
      PangoLayout *layout = initialisePangoLayout(program);
      int textHeightPixels = calculateTextHeightPixels(layout);
      int cellHeightPixels = calculateCellHeightPixels(textHeightPixels, *sheet);
      XWindowAttributes winAttribs = {0};
      XGetWindowAttributes(program.display, program.window, &winAttribs);
      int maxRowsOnScreen = calculateMaxRowsOnScreen(winAttribs, cellHeightPixels);

      switch (action.motion) {
        case MTN_SHEET_TOP: {
          sheet->selectedCell = sheet->selectedCell % sheet->columnCount;
          viewport->anchorRow = clampToValue(viewport->anchorRow, -maxRowsOnScreen, 0, 0);
          break;
        }
        default: {
          break;
          assert(FALSE, "[aL9dsKLE] action.motion should never hit this");
        }
      }
      if (action.motion == 0) {
        assert_gt_int(action.count, 0, "[ckm+lVnh] action.count is not gt 0");
        int oneBasedRowIndex = action.count - 1;
        int columnShift = sheet->selectedCell % sheet->columnCount;
        sheet->selectedCell = oneBasedRowIndex * sheet->columnCount + columnShift;
        viewport->anchorRow = clampToValue(viewport->anchorRow, oneBasedRowIndex - maxRowsOnScreen + 1, oneBasedRowIndex, oneBasedRowIndex - maxRowsOnScreen / 2);
        viewport->anchorRow = clamp(viewport->anchorRow, 0, INT_MAX);
      }

      break;
    }
    case INSERT_BEGIN: {
      sheet->insertMode = TRUE;
      // INSERT MODE AT THE START OF THE STRING. Maybe ii does this, i puts you in "normal text mode" as opposed to "normal sheet mode"
      break;
    }
    case INSERT_TEXT:
      sheetCellAppend(*sheet, sheet->selectedCell, action.text.value, action.text.length);
      break;
    /* case 'I': { */
    /*   sheet->insertMode = TRUE; */
    /*   // INSERT MODE AT THE END OF THE STRING */
    /*   break; */
    /* } */
    case APPEND_COLUMN: {
      sheetAppendColumn(sheet, sheet->selectedCell + 1);
      int selectedRow = sheet->selectedCell / (sheet->columnCount - 1);
      sheet->selectedCell += selectedRow + 1; // sheetAppendColumn will add insert a cell for each row
      break;
    }
    case INSERT_COLUMN: {
      // INSERT NEW COLUMN
      sheetAppendColumn(sheet, sheet->selectedCell);
      int selectedRow = sheet->selectedCell / (sheet->columnCount - 1);
      sheet->selectedCell += selectedRow; // sheetAppendColumn will add insert a cell for each row
      break;
    }
    case APPEND_ROW: {
      sheetAppendRow(sheet, sheet->selectedCell + sheet->columnCount);
      sheet->selectedCell += sheet->columnCount;
      break;
    }
    case INSERT_ROW: {
      sheetAppendRow(sheet, sheet->selectedCell);
      break;
    }
    case DELETE_ROW: {
      if (sheet->rowCount > 0)
        sheetDeleteRow(sheet, sheet->selectedCell);
      break;
    }
    /* case '$': { // TODO */
    /*   sheetAppendRow(&sheet, sheet->selectedCell); */
    /*   break; */
    /* } */
    /* case 'e': { // TODO */
    /*   sheetAppendRow(&sheet, sheet->selectedCell); */
    /*   break; */
    /* } */
    /* case 'Input.CTRL_E': { // TODO */
    /*   sheetAppendRow(&sheet, sheet->selectedCell); */
    /*   break; */
    /* } */
    /* case 'y???': { // TODO */
    /*   sheetAppendRow(&sheet, sheet->selectedCell); */
    /*   break; */
    /* } */
    /* case 'Input.CTRL_Y': { // TODO */
    /*   sheetAppendRow(&sheet, sheet->selectedCell); */
    /*   break; */
    /* } */
  }
}

void commitAction(Sheet *sheet, Viewport *viewport, Action action) {
  normalModeAction(sheet, viewport, action);
  Action emptyAction = {0};
  program.currentAction = emptyAction;
}



PangoLayout *initialisePangoLayout(Program program) {
  PangoLayout *layout = pango_cairo_create_layout(program.cr);
  pango_layout_set_font_description(layout, program.font);
  pango_layout_set_text(layout, "a", -1);
  return layout;
}

int calculateTextHeightPixels(PangoLayout *layout) {
  PangoRectangle logicalRectPixels;
  pango_layout_get_pixel_extents(layout, NULL, &logicalRectPixels);
  int textHeightPixels = TEMP_TEXT_SCALE * logicalRectPixels.height;
  return textHeightPixels;
}

int calculateTextWidthPixels(PangoLayout *layout) {
  PangoRectangle logicalRectPixels;
  pango_layout_get_pixel_extents(layout, NULL, &logicalRectPixels);
  int textWidthPixels = TEMP_TEXT_SCALE * logicalRectPixels.width;
  return textWidthPixels;
}

int calculateCellWidthPixels(int textWidthPixels, Sheet sheet) {
  int cellWidthPixels = TEMP_CELL_WIDTH * textWidthPixels + 2 * sheet.horizontalPadding;
  return cellWidthPixels;
} 
int calculateCellHeightPixels(int textHeightPixels, Sheet sheet) {
  int cellHeightPixels = TEMP_CELL_HEIGHT * textHeightPixels + 2 * sheet.verticalPadding;
  return cellHeightPixels;
} 

int calculateMaxRowsOnScreen(XWindowAttributes winAttribs, int cellHeightPixels) {
  int maxRowsOnScreen = (winAttribs.height) / cellHeightPixels;

  return maxRowsOnScreen;
}

int calculateMaxColumnsOnScreen(XWindowAttributes winAttribs, int cellWidthPixels) {
  int maxColumnsOnScreen = (winAttribs.width) / cellWidthPixels;

  return maxColumnsOnScreen;
}

/////////////////////////////////////////////////////////////////////
// Render
/////////////////////////////////////////////////////////////////////
void render(Program program, Sheet sheet, Viewport *viewport) {
  XWindowAttributes winAttribs = {0};
  XGetWindowAttributes(program.display, program.window, &winAttribs);
  PangoLayout *layout = initialisePangoLayout(program);

  int textHeightPixels = calculateTextHeightPixels(layout);
  int textWidthPixels = calculateTextWidthPixels(layout);
  int cellWidthPixels = calculateCellWidthPixels(textWidthPixels, sheet);
  int cellHeightPixels = calculateCellHeightPixels(textHeightPixels, sheet);
  int sheetWidth = sheet.columnCount * cellWidthPixels;
  int sheetHeight = sheet.rowCount * cellHeightPixels;

  int rowNumberColumnWidth = 0;
  int columnNumberRowHeight = textHeightPixels + 2 * sheet.verticalPadding;
  {
    int maxRowsOnScreen = calculateMaxRowsOnScreen(winAttribs, cellHeightPixels);
    int rowsOnScreen = maxRowsOnScreen > sheet.rowCount ? sheet.rowCount : maxRowsOnScreen;
    rowNumberColumnWidth = stringFromLong(rowsOnScreen).length * textWidthPixels + 2 * sheet.horizontalPadding;
  }
  int xoffset = clamp((winAttribs.width - sheetWidth) / 2 - sheet.horizontalPadding, rowNumberColumnWidth, INT_MAX);
  int yoffset = clamp((winAttribs.height - sheetHeight) / 2 - sheet.verticalPadding, columnNumberRowHeight, INT_MAX);
  yoffset -= 1 * viewport->anchorRow * cellHeightPixels;
  xoffset -= 1 * viewport->anchorColumn * cellWidthPixels;

  printf("[DEBUG] sheet.selectedCell: %d\n", sheet.selectedCell);
  printf("[DEBUG] selectedRow: %d\n", sheet.selectedCell / sheet.columnCount);
  printf("[DEBUG] viewport->anchorRow: %d\n", viewport->anchorRow);



  XClearWindow(program.display, program.window);

  XSetForeground(program.display, program.gc, program.background.pixel);
  XFillRectangle(program.display, program.window, program.gc, 0, 0, winAttribs.width, winAttribs.height);



  /////////////////////////////////////////////////////////////////////
  // Highlight the selected Cell
  /////////////////////////////////////////////////////////////////////
  { // Use braces here to make it clear that the variables defined are only used here and not lower in the function
    int row = sheet.selectedCell / sheet.columnCount;
    int column = sheet.selectedCell % sheet.columnCount;
    int x = TEMP_CELL_WIDTH * textWidthPixels * column + xoffset + 2 * column * sheet.horizontalPadding;
    int y = TEMP_CELL_HEIGHT * textHeightPixels * row + yoffset + 2 * row * sheet.verticalPadding;
    XSetForeground(program.display, program.gc, program.highlight.pixel);
    XFillRectangle(program.display, program.window, program.gc, x, y, cellWidthPixels, cellHeightPixels);
  }

  PangoRectangle logicalRectPangoUnits;
  pango_layout_get_extents(layout, NULL, &logicalRectPangoUnits);
  // int textScaledHeightPangoUnits = TEMP_TEXT_SCALE * logicalRectPangoUnits.height;
  // int textScaledWidthPangoUnits = TEMP_TEXT_SCALE * logicalRectPangoUnits.width;



  /////////////////////////////////////////////////////////////////////
  // Render text for each cell
  /////////////////////////////////////////////////////////////////////
  for (int i = 0; i < sheet.columnCount * sheet.rowCount; i++) {
    cairo_save(program.cr);

    if (sheet.strings.data[i].length == 0) continue;
    pango_layout_set_text(layout, sheet.strings.data[i].value, sheet.strings.data[i].length);
    pango_layout_set_width(layout, TEMP_CELL_WIDTH * logicalRectPangoUnits.width + 2); // TODO: why +2. Is this because padding is 4 and maybe borders are 2?
    pango_layout_set_height(layout, TEMP_CELL_HEIGHT * logicalRectPangoUnits.height + 2);
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
    pango_cairo_update_layout(program.cr, layout);

    int column = i % sheet.columnCount;
    int row = i / sheet.columnCount;
    int x = xoffset + column * TEMP_CELL_WIDTH * textWidthPixels + 2 * column * sheet.horizontalPadding + sheet.horizontalPadding;
    int y = yoffset + row * TEMP_CELL_HEIGHT * textHeightPixels + 2 * row * sheet.verticalPadding + sheet.verticalPadding;
    cairo_translate(program.cr, x, y);
    cairo_scale(program.cr, TEMP_TEXT_SCALE, TEMP_TEXT_SCALE);
    cairo_set_source_rgba(program.cr, program.text.red, program.text.green, program.text.blue, program.text.alpha);

    pango_cairo_show_layout(program.cr, layout);

    cairo_restore(program.cr);
  }
  


  /////////////////////////////////////////////////////////////////////
  // Render text for row numbering & column lettering
  /////////////////////////////////////////////////////////////////////
  for (int column = 0; column < sheet.columnCount; column++) {
    cairo_save(program.cr);

    String string = intToLetters(column);
    pango_layout_set_text(layout, string.value, string.length);
    pango_layout_set_width(layout, TEMP_CELL_WIDTH * logicalRectPangoUnits.width + 2);
    pango_layout_set_height(layout, TEMP_CELL_HEIGHT * logicalRectPangoUnits.height + 2);
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
    pango_cairo_update_layout(program.cr, layout);

    int x = xoffset + column * TEMP_CELL_WIDTH * textWidthPixels + 2 * column * sheet.horizontalPadding + sheet.horizontalPadding;
    int y = yoffset + sheet.verticalPadding - textHeightPixels - 2 * sheet.verticalPadding;
    cairo_translate(program.cr, x, y);
    cairo_scale(program.cr, TEMP_TEXT_SCALE, TEMP_TEXT_SCALE);
    cairo_set_source_rgba(program.cr, program.text.red, program.text.green, program.text.blue, program.text.alpha);

    pango_cairo_show_layout(program.cr, layout);

    cairo_restore(program.cr);
  }
  for (int row = 0; row < sheet.rowCount; row++) {
    cairo_save(program.cr);

    String string = stringFromLong(row + 1);
    pango_layout_set_text(layout, string.value, string.length);
    pango_layout_set_width(layout, TEMP_CELL_WIDTH * logicalRectPangoUnits.width + 2);
    pango_layout_set_height(layout, TEMP_CELL_HEIGHT * logicalRectPangoUnits.height + 2);
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
    pango_cairo_update_layout(program.cr, layout);

    int x = xoffset + sheet.horizontalPadding - rowNumberColumnWidth;
    int y = yoffset + row * TEMP_CELL_HEIGHT * textHeightPixels + 2 * row * sheet.verticalPadding + sheet.verticalPadding;
    cairo_translate(program.cr, x, y);
    cairo_scale(program.cr, TEMP_TEXT_SCALE, TEMP_TEXT_SCALE);
    cairo_set_source_rgba(program.cr, program.text.red, program.text.green, program.text.blue, program.text.alpha);

    pango_cairo_show_layout(program.cr, layout);

    cairo_restore(program.cr);
  }

  /*
  pango_font_description_free(desc);
  g_object_unref(layout);
  cairo_destroy(program.cr);
  */



  /////////////////////////////////////////////////////////////////////
  // Draw the Rows and Columns
  /////////////////////////////////////////////////////////////////////
  XSetForeground(program.display, program.gc, program.foreground.pixel);
  XDrawLine(program.display, program.window, program.gc, winAttribs.x, winAttribs.y + yoffset, winAttribs.x + winAttribs.width, winAttribs.y + yoffset);
  for (int i = 0; i < sheet.rowCount; i++) {
    int y = winAttribs.y + yoffset + cellHeightPixels;
    int x1 = winAttribs.x;
    int x2 = winAttribs.x + winAttribs.width;
    XDrawLine(program.display, program.window, program.gc, x1, y, x2, y);
    yoffset += cellHeightPixels;
  }

  XDrawLine(program.display, program.window, program.gc, winAttribs.x + xoffset, winAttribs.y, winAttribs.x + xoffset, winAttribs.y + winAttribs.height);
  for (int i = 0; i < sheet.columnCount; i++) {
    int x = winAttribs.x + xoffset + cellWidthPixels;
    int y1 = winAttribs.y;
    int y2 = winAttribs.y + winAttribs.height;
    XDrawLine(program.display, program.window, program.gc, x, y1, x , y2);
    xoffset += cellWidthPixels;
  }

  // XFlush(program.display);
}


////////////////////////////////////////////////////////////////////////
// KeyPress functions
////////////////////////////////////////////////////////////////////////
char validateUserInput() {
// bad user input - abort and reset
if (program.currentAction.action != 0) {
  Action action = {0};
  program.currentAction = action;
  return FALSE;
}
assert_equal_int(program.currentAction.motion, 0, "[erfDJ05n] program.currentAction.motion is not 0");
assert_zeroed((void*) &program.currentAction.text, sizeof(program.currentAction.text), "[P5f9O2Mu] program.currentAction.text is not zeroed");

return TRUE;
}

void handleKeyPress(XEvent event, Sheet *sheet, Viewport *viewport, Map mapCount, Map mapAction, Map mapMotion) {
  KeySym keySymPressed = XLookupKeysym(&event.xkey, 0);
  char *translatedCharArray = calloc(100, sizeof(char));
  int translatedCharLength = XLookupString(&event.xkey, translatedCharArray, 100, NULL, NULL);
  String translatedString = stringFromNative(translatedCharArray, translatedCharLength);
  char translatedChar = translatedCharArray[0];

  printf("[DEBUG] translatedChar: %s\n", translatedCharArray);
  printf("[DEBUG] translatedChar (hex): %x\n", translatedChar);
  printf("[DEBUG] translatedCharLength: %d\n", translatedCharLength);

  if (event.xkey.state & LockMask) {
    printf("[DEBUG] Lock is on\n");
  }
  if (event.xkey.state & ControlMask) {
    printf("[DEBUG] Control is held\n");
  }
  if (event.xkey.state & ShiftMask) {
    printf("[DEBUG] Shift is held\n");
  }

  if (translatedCharLength == 0) {
    return;
  }



  ////////////////////////////////////////////////////////////////
  // INSERT MODE
  ////////////////////////////////////////////////////////////////
  if (sheet->insertMode == TRUE) {
    if (translatedChar == ESCAPE) {
      sheet->insertMode = FALSE;
      return;
    }
    else if (translatedChar == BACKSPACE) {
      sheetCellBackSpace(*sheet, sheet->selectedCell, sheet->strings.data[sheet->selectedCell].length - 1);
    }
    else if (translatedChar < 0x20 || translatedChar == 0x7F) {
      return;
      // Not sure if this is a good blanket solution but doing
      // this to prevent ctrl chars being rendered to the
      // screen by typing e.g. CTRL-K or DELETE
      // is this an exhaustive list of ctrl chars?
    }
    else {
      sheetCellAppend(*sheet, sheet->selectedCell, &translatedChar, translatedCharLength);
      program.lastAction.text = stringInsertChar(program.lastAction.text, translatedChar, program.lastAction.text.length);
    }
  }



  ////////////////////////////////////////////////////////////////
  // NORMAL MODE
  ////////////////////////////////////////////////////////////////
  else {
    Action action = {0};
    Action emptyAction = {0};
    program.normalModeState = 0;

    NORMAL_MODE_STATE nms = 0;
    if (program.currentAction.action != 0) {
      nms = NMS_MOTION;
    }
    else if (mapHasKey(&mapCount, translatedString)) {
      nms = NMS_COUNT;
    } 
    else {
      nms = NMS_ACTION;
      if (event.xkey.state & ControlMask) {
        /*translatedString = map*/
      }
    }

    switch (nms) {
      case NMS_COUNT: {
        int count = mapLookup(&mapCount, translatedString);
        if (count == INT_MAX) {
          program.currentAction = emptyAction;
        }
        else {
          program.currentAction.count = count;
        }
        break;
      }
      case NMS_ACTION: {
  printf("[REMOVE ME] before\n");
        NORMAL_MODE_ACTION action = mapLookup(&mapAction, translatedString);
  printf("[REMOVE ME] before\n");
        if (action == INT_MAX) {
          program.currentAction.action = action;
          program.currentAction = emptyAction;
        }
        else {
          program.currentAction.action = action;
          commitAction(sheet, viewport, program.currentAction);
        }
        break;
      }
      case NMS_MOTION: {
        MOTION motion = mapLookup(&mapMotion, translatedString);
        if (motion == INT_MAX) {
          program.currentAction = emptyAction;
        }
        else {
          program.currentAction.motion = motion;
          commitAction(sheet, viewport, program.currentAction);
        }
        break;
      }
    }



    /*if (event.xkey.state & ControlMask) {*/
    /*  switch (keySymPressed) {*/
    /*    case XK_d: {*/
    /*      action.action = VIEWPORT_HALF_PAGE_DOWN;*/
    /*      commitAction(sheet, viewport, action);*/
    /*      break;*/
    /*    }*/
    /*  }*/
    /*}*/
    /*else {*/
    /*  switch (translatedChar) {*/
    /*    case 'd': {*/
    /*      switch (program.normalModeState) {*/
    /*        case COUNT_ACTION: {*/
    /*          program.currentAction.action = DELETE_ROW;*/
    /*          break;*/
    /*        }*/
    /*        case ACTION: {*/
    /*          program.currentAction.action = DELETE_ROW;*/
    /*          break;*/
    /*        }*/
    /*        case COUNT_ACTION_MOTION: {*/
    /*          if (program.currentAction.action == DELETE_ROW) {*/
    /*            program.currentAction.motion = MTN_ROW;*/
    /*            for (int i = 0; i < program.currentAction.count; i++) {*/
    /*              commitAction(sheet, viewport, action);*/
    /*            }*/
    /*            program.lastAction = action;*/
    /*          } else {*/
    /*            Action action = {0};*/
    /*            program.currentAction = action;*/
    /*          }*/
    /*          break;*/
    /*        }*/
    /*        case ACTION_MOTION: {*/
    /*          if (program.currentAction.action == DELETE_ROW) {*/
    /*            action.motion = MTN_ROW;*/
    /*            commitAction(sheet, viewport, action);*/
    /*            program.lastAction = action;*/
    /*          } else {*/
    /*            Action action = {0};*/
    /*            program.currentAction = action;*/
    /*          }*/
    /*          break;*/
    /*        }*/
    /*        default: {*/
    /*          assert(FALSE, "FUCKED");*/
    /*          break;*/
    /*        }*/
    /*      }*/
    /*      break;*/
    /*    }*/
    /*  }*/
    /*}*/
  }
  render(program, *sheet, viewport);
}

//******************************************//
//               Main                       //
//******************************************//

int main() {
  /*Map map = mapNew(1);*/
  /*for (int i = 0; i < 10000; i++) {*/
  /*  String post = stringFromLong(i);*/
  /*  String pre = { "key", 3 };*/
  /*  String key = stringCat(pre, post);*/
  /*  mapInsert(&map, key, i);*/
  /*}*/
  /*String keyToRemove1 = { "key1122", 7 };*/
  /*String keyToRemove2 = { "key1124", 7 };*/
  /*int mapRemoveResult1 = mapRemove(&map, keyToRemove1);*/
  /*int mapRemoveResult2 = mapRemove(&map, keyToRemove2);*/
  /*int mapRemoveResult3 = mapRemove(&map, keyToRemove2);*/
  /*printf("mapRemoveResult1: %d\n", mapRemoveResult1);*/
  /*printf("mapRemoveResult2: %d\n", mapRemoveResult2);*/
  /*printf("mapRemoveResult3: %d\n", mapRemoveResult3);*/
  /*String e1 = { "key1123", 7 };*/
  /*int value1 = mapLookup(&map, keyToRemove1);*/
  /*int value2 = mapLookup(&map, e1);*/
  /*int value3 = mapLookup(&map, keyToRemove2);*/
  /*printf("value1: %d\n", value1);*/
  /*printf("value2: %d\n", value2);*/
  /*printf("value3: %d\n", value3);*/

  /*String e1 = { "key1", 4 };*/
  /*String e2 = { "key2", 4 };*/
  /*String e3 = { "key3", 4 };*/
  /**/
  /*mapInsert(&map, e1, 1);*/
  /*mapInsert(&map, e2, 2);*/

  printf("[DEBUG] Application Initialising\n");
  struct timespec tsPreInit;
  clock_gettime(CLOCK_BOOTTIME, &tsPreInit);



  Display* display = XOpenDisplay(NULL);
  int screen_number = XDefaultScreen(display);
  Window root = XRootWindow(display, screen_number);

  /*
  unsigned long valueMask = CWBackPixel;
  XSetWindowAttributes windowAttributes = {0};
  windowAttributes.background_pixel = UINT32_MAX;
  windowAttributes.event_mask = KeyPressMask | ButtonPressMask;
  Window window = XCreateWindow(display, root, 0, 0, 100, 100, 10, XDefaultDepth(display, screen_number), CopyFromParent, CopyFromParent, valueMask, &windowAttributes);
  */
  Window window = XCreateSimpleWindow(display, XDefaultRootWindow(display), 0, 0, 100, 100, 0, 0, UINT32_MAX);
  XSelectInput(display, window, KeyPressMask | KeyReleaseMask | ButtonPressMask | ExposureMask);

  /*

        "name": "Night Owl",

        "cursorColor": "#80a4c2",
        "selectionBackground": "#1d3b53",

        "background": "#011627",
        "foreground": "#D6DEEB",
        
        "black": "#011627",
        "blue": "#82AAFF",
        "cyan": "#21C7A8",            
        "green": "#22DA6E",
        "purple": "#C792EA",
        "red": "#EF5350",
        "white": "#FFFFFF",
        "yellow": "#c5e478",
        "brightBlack": "#575656",
        "brightBlue": "#82AAFF",
        "brightCyan": "#7FDBCA",
        "brightGreen": "#22DA6E",
        "brightPurple": "#C792EA",
        "brightRed": "#EF5350",
        "brightWhite": "#FFFFFF",
        "brightYellow": "#FFEB95"

   */

  Colormap colormap = XDefaultColormap(display, screen_number);
  XColor foreground = {0};
  foreground.red = 0x1d1d;
  foreground.green = 0x3b3b;
  foreground.blue = 0x5353;
  Status status = XAllocColor(display, colormap, &foreground);
  XColor background = {0};
  background.green = 0x1515;
  background.blue = 0x2626;
  status = XAllocColor(display, colormap, &background);
  XColor highlight = {0};
  highlight.green = 0x2515;
  highlight.blue = 0x3626;
  status = XAllocColor(display, colormap, &highlight);
  Color text = {0};
  text.red = ((double)0xd6d6 / (double)0xffff);
  text.green = ((double)0xdede / (double)0xffff);
  text.blue = ((double)0xebeb / (double)0xffff);
  text.alpha = 1.0;
  if (status == 0) 
    printf("error\n");

  GC gc = XCreateGC(display, window, 0, 0);
  XSetForeground(display, gc, foreground.pixel);
  XSetLineAttributes(display, gc, 1, LineSolid, CapNotLast, JoinMiter);

  XClassHint classHint = {
    "elwyn",
    "elwyn"
  };
  XSetClassHint(display, window, &classHint);
//   XSetWMProperties(display, window, NULL, NULL, NULL, 0, NULL, NULL, &classHint);
  XMapWindow(display, window);

  printf("[TRACE] X Initialised\n");



  Screen *screen = XScreenOfDisplay(display, screen_number);
  int width = XWidthOfScreen(screen);
  int height = XHeightOfScreen(screen);
  cairo_surface_t *surface = cairo_xlib_surface_create(display, window, XDefaultVisual(display, screen_number), width, height);
  cairo_t *cr = cairo_create(surface);

  PangoFontDescription *desc = pango_font_description_from_string("Liberation Mono 20");

  printf("[TRACE] Cairo Context Initialised\n");



  /*
  Sheet conf = {0};
  int cellWidths[7] = {60,100,60,60,200,500,100};
  for (int i = 0; i < 7; i++)
    conf.cellWidths[i] = cellWidths[i];
  int cellHeights[7] = {60,20,20,20,20,100,20};
  for (int i = 0; i < 7; i++)
    conf.cellHeights[i] = cellHeights[i];
  char *cells[49];
  for (int i = 0; i < 49; i++) {
    char t[2] = {(char)i, '\0'};
    cells[i] = t;
  }
  */

  Sheet sheet = newSheet(13, 5);
  sheetCellAppend(sheet, 0, "helyo", 5);
  sheet.selectedCell = 0;

  printf("[TRACE] Spreadsheet Initialised\n");



  Viewport viewport = {0};
  /*viewport.maxCellCountTall = 3;*/
  /*viewport.maxCellCountWide = 3;*/
  /*viewport.hasFocus = TRUE;*/

  printf("[TRACE] Viewport Initialised\n");



  program.display = display;
  program.window = window;
  program.gc = gc;
  program.surface = surface;
  program.cr = cr;
  program.font = desc;
  program.background = background;
  program.highlight = highlight;
  program.foreground = foreground;
  program.text = text;

  printf("[TRACE] Program Data Initialised\n");



  String inputCharacter = {0};
  inputCharacter.length = 1;

  Map mapMotion = mapNew(100);

  inputCharacter.value = "g";
  mapInsert(&mapMotion, inputCharacter, MTN_SHEET_TOP);
  inputCharacter.value = "d";
  mapInsert(&mapMotion, inputCharacter, MTN_ROW);
  inputCharacter.value = "y";
  mapInsert(&mapMotion, inputCharacter, MTN_ROW);
  inputCharacter.value = "c";
  mapInsert(&mapMotion, inputCharacter, MTN_ROW);
  /*mapInsert(&mapMotion, inputCharacter, MTN_COLUMN);*/
  /*inputCharacter.value = "k";*/
  mapInsert(&mapMotion, inputCharacter, MTN_ROW_UP);
  inputCharacter.value = "j";
  mapInsert(&mapMotion, inputCharacter, MTN_ROW_DOWN);
  inputCharacter.value = "h";
  mapInsert(&mapMotion, inputCharacter, MTN_COLUMN_LEFT);
  inputCharacter.value = "l";
  mapInsert(&mapMotion, inputCharacter, MTN_COLUMN_RIGHT);



  Map mapAction = mapNew(100);

  inputCharacter.value = "h";
  mapInsert(&mapAction, inputCharacter, COLUMN_LEFT);
  inputCharacter.value = "l";
  mapInsert(&mapAction, inputCharacter, COLUMN_RIGHT);
  inputCharacter.value = "k";
  mapInsert(&mapAction, inputCharacter, ROW_UP);
  inputCharacter.value = "j";
  mapInsert(&mapAction, inputCharacter, ROW_DOWN);
  inputCharacter.value = "M";
  mapInsert(&mapAction, inputCharacter, SELECTED_CELL_CENTRE);
  inputCharacter.value = "H";
  mapInsert(&mapAction, inputCharacter, SELECTED_CELL_TOP);
  inputCharacter.value = "L";
  mapInsert(&mapAction, inputCharacter, SELECTED_CELL_BOTTOM);
  inputCharacter.value = "0";
  mapInsert(&mapAction, inputCharacter, SELECTED_CELL_BEGINNING);
  inputCharacter.value = "$";
  mapInsert(&mapAction, inputCharacter, SELECTED_CELL_END);
  /*inputCharacter.value = "0";*/
  /*mapInsert(&mapAction, inputCharacter, VIEWPORT_ROW_UP);*/
  /*inputCharacter.value = "0";*/
  /*mapInsert(&mapAction, inputCharacter, VIEWPORT_ROW_DOWN);*/
  /*inputCharacter.value = "0";*/
  /*mapInsert(&mapAction, inputCharacter, VIEWPORT_COLUMN_LEFT);*/
  /*inputCharacter.value = "0";*/
  /*mapInsert(&mapAction, inputCharacter, VIEWPORT_COLUMN_RIGHT);*/
  /*inputCharacter.value = "0";*/
  /*mapInsert(&mapAction, inputCharacter, VIEWPORT_COLUMN_LEFT_HALF_PAGE);*/
  /*inputCharacter.value = "0";*/
  /*mapInsert(&mapAction, inputCharacter, VIEWPORT_COLUMN_RIGHT_HALF_PAGE);*/
  /*inputCharacter.value = "0";*/
  /*mapInsert(&mapAction, inputCharacter, VIEWPORT_PAGE_UP);*/
  /*inputCharacter.value = "0";*/
  /*mapInsert(&mapAction, inputCharacter, VIEWPORT_PAGE_DOWN);*/
  /*inputCharacter.value = "0";*/
  /*mapInsert(&mapAction, inputCharacter, VIEWPORT_HALF_PAGE_UP);*/
  /*inputCharacter.value = "0";*/
  /*mapInsert(&mapAction, inputCharacter, VIEWPORT_HALF_PAGE_DOWN);*/
  /*inputCharacter.value = "0";*/
  /*mapInsert(&mapAction, inputCharacter, VIEWPORT_GO_TO);*/
  inputCharacter.value = "i";
  mapInsert(&mapAction, inputCharacter, INSERT_BEGIN);
  inputCharacter.value = "a";
  mapInsert(&mapAction, inputCharacter, APPEND_COLUMN);
  inputCharacter.value = "A";
  mapInsert(&mapAction, inputCharacter, INSERT_COLUMN);
  inputCharacter.value = "o";
  mapInsert(&mapAction, inputCharacter, APPEND_ROW);
  inputCharacter.value = "O";
  mapInsert(&mapAction, inputCharacter, INSERT_ROW);
  inputCharacter.value = "d";
  mapInsert(&mapAction, inputCharacter, DELETE_ROW);




  Map mapCount = mapNew(9);

  inputCharacter.value = "1";
  mapInsert(&mapCount, inputCharacter, 1);
  inputCharacter.value = "2";
  mapInsert(&mapCount, inputCharacter, 2);
  inputCharacter.value = "3";
  mapInsert(&mapCount, inputCharacter, 3);
  inputCharacter.value = "4";
  mapInsert(&mapCount, inputCharacter, 4);
  inputCharacter.value = "5";
  mapInsert(&mapCount, inputCharacter, 5);
  inputCharacter.value = "6";
  mapInsert(&mapCount, inputCharacter, 6);
  inputCharacter.value = "7";
  mapInsert(&mapCount, inputCharacter, 7);
  inputCharacter.value = "8";
  mapInsert(&mapCount, inputCharacter, 8);
  inputCharacter.value = "9";
  mapInsert(&mapCount, inputCharacter, 9);

  printf("[TRACE] Input Maps Initialised\n");



  struct timespec tsPostInit;
  clock_gettime(CLOCK_BOOTTIME, &tsPostInit);
  char *initDurationFormatted = tsFormat(tsPreInit, tsPostInit);
  printf("[DEBUG] Application Initialisation Complete\n");
  printf("[DEBUG] Application Initialisation took %s\n", initDurationFormatted);
  printf("[DEBUG] Entering Event Loop\n");



  XEvent event = {0};
  while (TRUE) {
    XNextEvent(display, &event);

    switch (event.type) {



      //////////////////////////////////////////////////////////////////
      // Expose
      //////////////////////////////////////////////////////////////////
      case Expose: {
        printf("\n");
        printf("[DEBUG] ========= BEGIN Expose =========\n");

        render(program, sheet, &viewport);

        printf("[DEBUG] ========= END Expose =========\n");
        break;

        /*
        XTextItem textItem = {0};
        textItem.chars = "hi";
        textItem.nchars = 2;
        XFontStruct *fontStruct = XLoadQueryFont(display, "r14");
        if (!fontStruct) {
          printf("error");
          return 1;
        }
        XSetFont(display, gc, fontStruct->fid);
        XDrawString(display, window, gc, 800, 500, "Text", 4);



        PangoGlyphString *glyphString = pango_glyph_string_new();
        char *text = "hi";
        for (int i = 0; text[i] != 0; i++) {
          PangoGlyphInfo gi = {0};
          gi.glyph = text[i];
          glyphString->glyphs[i] = gi;
        }
        PangoRenderer pangoRenderer = {0};
        pango_renderer_draw_glyphs(pangoRenderer, PangoFont *font, glyphString, 10, 10);
        */

      }



      //////////////////////////////////////////////////////////////////
      // KeyPress
      //////////////////////////////////////////////////////////////////
      case KeyPress: {
        printf("\n");
        printf("[DEBUG] ========= BEGIN KeyPress =========\n");

        handleKeyPress(event, &sheet, &viewport, mapCount, mapAction, mapMotion);

        printf("[DEBUG] ========= END KeyPress =========\n");
      }


      //////////////////////////////////////////////////////////////////
      // KeyRelease
      //////////////////////////////////////////////////////////////////
      case KeyRelease: {
        KeySym keysym = XLookupKeysym(&event.xkey, 0);
        char *keyReleased = XKeysymToString(keysym);
        if (!keyReleased)
          break;
        break;
      }



      //////////////////////////////////////////////////////////////////
      // ButtonPress
      //////////////////////////////////////////////////////////////////
      case ButtonPress: {
        printf("\n");
        printf("[DEBUG] ========= BEGIN ButtonPress =========\n");

        if (event.xbutton.button == Button1) {
          printf("Button1 press\n");
        }

        printf("[DEBUG] ========= END ButtonPress =========\n");
        break;
      }
    }
  }


  return 0;
}


// TODO: 
// - sticky header column
// - sticky header row
// - read file
// - save file
// - improve memory manaegent (sheet.strings)
// - fix: seg fault on too many o actions
// - fix: crash on too many a actions
// - fix: M is relative to the screen, not the sheet
// - need a nice way to simplify the complexity of implementing compositions (<count><action><motion>)
//   - improve <count>g implementation
// - fix: VIEWPORT_ROW_DOWN does not clamp selected cell
// - fix: VIEWPORT_ROW_UP does not clamp selected cell
// - fix: VIEWPORT_COLUMN_LEFT does not clamp selected cell
// - fix: VIEWPORT_COLUMN_RIGHT does not clamp selected cell
// - g<motion>
// - G
// - d<motion>
// - dd
// - clamp the viewport before rendering
// - ignore function keys
// - make the renderer render things relative to a point so that a cells text, extents, etc are all 'stuck' together
// - make i insert before text and a insert after text
// - Support keyToUpper for non alphabet characters such as $
// - Support scrolling
// - '.' doesnt work for certain input operations, i think my string handling is broken
//
// Done:
// - <count>g
// - gg
// - Add VIEWPORT_COLUMN_RIGHT_HALF_PAGE('E')
// - Add VIEWPORT_COLUMN_LEFT_HALF_PAGE('B')
// - Add VIEWPORT_COLUMN_RIGHT('e')
// - Add VIEWPORT_COLUMN_LEFT('b')
// - $
// - 0
// - l - bug fix - when at the furthest right column, do not change the selectedCell
// - H
// - L
// - M
// - VIEWPORT_HALF_PAGE_UP ('CTRL-u')
// - VIEWPORT_HALF_PAGE_DOWN ('CTRL-d')
// - clamp viewport for ROW_UP
// - clamp viewport for ROW_DOWN
// - Add VIEWPORT_PAGE_UP ('CTRL-b')
// - Add VIEWPORT_PAGE_DOWN ('CTRL-f')
// - Add VIEWPORT_ROW_DOWN('CTRL-e') - moves the viewport down
// - Add VIEWPORT_ROW_UP('CTRL-y') - moves the viewport up  
// - Use the following field to determine shift state when determining capital letters: event.xkey.state. In fact, seems that XLookupString might account for shift/caps lock state and perhaps this should be used for converting key events to a string/character. Currently, caps lock does not result in capital letters.
// - centre view
// - handle space insertions properly
// - add a gap between cells
// - only display text in cell extent
// - Escape goes to normal mode
// - BackSpace deletes text
// - add 'o' or 'O' to insert new row and 'a' or 'A' to insert new column
// - support uppercase character insertion and uppercase key actions
// - Add line numbers and column characters
// - Make '.' repeat the last command
//
//
// Requirements:
// - Edit cells
// - add rows and columns
//
// NOTES - TEXT INPUT:
// - Supported
//   - Latin-1
//   - the renderer support UTF-8 although the input manager does not (only supports Latin-1)
// - Unsupported
//   - Is Euro sign support (i.e. shift-alt-4 or whatever it is)
//
// NOTES - ACTIONS:
// - an interaction with the spreadsheet is a composition of <count><action><motion>
//   - In cases where the action can either act on rows or columns, the user can specify this using the <count> component.
//     - a <count> of 18r would act on 19 rows (the current row plus 18 rows above or below)
//     - 18c would similarly act on the columns
//     - full example 18rdj - i.e. delete 19 rows from the selected row, downwards
//       - 18r is the count
//       - d is the action
//       - j is the motion
