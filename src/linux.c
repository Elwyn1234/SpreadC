#include "cairo.h"
#include "pango/pango-font.h"
#include "pango/pango-glyph.h"
#include "pango/pango-layout.h"
#include "pango/pango-renderer.h"
#include <X11/X.h>
#include <stdint.h>
#include <stdlib.h>
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
typedef enum NORMAL_MODE_ACTION NORMAL_MODE_ACTION;
typedef struct Action Action;
typedef struct Program Program;
typedef struct DynamicStringArray DynamicStringArray;
typedef struct DynamicIntArray DynamicIntArray;

PangoLayout *initialisePangoLayout(Program program);
int calculateMaxRowsOnScreen(XWindowAttributes winAttribs, int cellHeightPixels);
int calculateTextHeightPixels(PangoLayout *layout);
int calculateCellHeightPixels(int textHeightPixels, Sheet sheet);

Program program;

struct Color {
  double red;
  double blue;
  double green;
  double alpha;
};

struct String {
  char *value;
  int length;
};

enum NORMAL_MODE_ACTION {
  COLUMN_LEFT,
  COLUMN_RIGHT,
  ROW_UP,
  ROW_DOWN,
  SELECTED_CELL_CENTRE,
  SELECTED_CELL_BOTTOM,
  VIEWPORT_ROW_UP,
  VIEWPORT_ROW_DOWN,
  VIEWPORT_PAGE_UP,
  VIEWPORT_PAGE_DOWN,
  VIEWPORT_HALF_PAGE_UP,
  VIEWPORT_HALF_PAGE_DOWN,
  INSERT_BEGIN,
  INSERT_TEXT,
  APPEND_COLUMN,
  INSERT_COLUMN,
  APPEND_ROW,
  INSERT_ROW,
};

struct Action {
  NORMAL_MODE_ACTION action;
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

  Action lastAction;
};



//******************************************//
//                   Util                   //
//******************************************//

char keyToUpper(char key) {
  if (key > 96 && key < 123) {
    return key - 32;
  }
  return key; // TODO
}

String intToString(int number) {
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

//******************************************//
//                  String                  //
//******************************************//

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

//******************************************//
//               String Array               //
//******************************************//

struct DynamicStringArray {
  int length;
  int capacity;
  String *data;
};

DynamicStringArray dynamicStringArrayNew(int capacity) {
  DynamicStringArray array = {0};
  array.length = 0;
  array.capacity = capacity;
  array.data = malloc(sizeof(String) * array.capacity);
  return array;
}
void dynamicStringArrayInsert(DynamicStringArray *array, String element, int index) {
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
  for (int i = newLength; i > index; i--) {
    array->data[i] = array->data[i-1];
  }
  array->data[index] = element;
  array->length = newLength;
}
void dynamicStringArrayRemove(DynamicStringArray *array, int element, int index) {
  int oldLength = array->length;
  int newLength = array->length - 1;
  if (newLength < array->capacity/4) {
    String *arrayData = array->data;
    array->capacity /= 2;
    array->data = malloc(sizeof(String) * array->capacity);
    for (int i = 0; i < oldLength; i++) {
      array->data[i] = arrayData[i];
    }
    for (int i = index; i < newLength; i++) {
      array->data[i] = array->data[i+1];
    }
  }
  array->length = newLength;
}



//******************************************//
//               Int Array               //
//******************************************//

struct DynamicIntArray {
  int length;
  int capacity;
  int *data;
};

DynamicIntArray dynamicIntArrayNew(int capacity) {
  DynamicIntArray array = {0};
  array.length = 0;
  array.capacity = capacity;
  array.data = malloc(sizeof(int) * array.capacity);
  return array;
}
void dynamicIntArrayInsert(DynamicIntArray *array, int element, int index) {
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
  for (int i = newLength; i > index; i--) {
    array->data[i] = array->data[i-1];
  }
  array->data[index] = element;
  array->length = newLength;
}
void dynamicIntArrayRemove(DynamicIntArray *array, int element, int index) {
  int oldLength = array->length;
  int newLength = array->length - 1;
  if (newLength < array->capacity/4) {
    int *arrayData = array->data;
    array->capacity /= 2;
    array->data = malloc(sizeof(int) * array->capacity);
    for (int i = 0; i < oldLength; i++) {
      array->data[i] = arrayData[i];
    }
    for (int i = index; i < newLength; i++) {
      array->data[i] = array->data[i+1];
    }
  }
  array->length = newLength;
}



//******************************************//
//               Sheet                      //
//******************************************//

struct Viewport {
  int anchorCell;
  /*int maxCellCountTall;*/
  /*int maxCellCountWide;*/
  /*Boolean hasFocus;*/
};

struct Sheet {
  DynamicStringArray strings;
  int columnCount;
  int rowCount;

  // Split fields into SheetCursor struct
  int selectedCell;
  Boolean insertMode;

  // Split fields into SheetFormat struct
  DynamicIntArray cellWidths;
  DynamicIntArray cellHeights;
  int verticalPadding;
  int horizontalPadding;
};

const int TEMP_CELL_WIDTH = 15;
const int TEMP_CELL_HEIGHT = 3;
const float TEMP_TEXT_SCALE = 0.5;

Sheet newSheet(int rowCount, int columnCount) {
  Sheet sheet = {0};
  int cellCount = columnCount * rowCount;
  sheet.cellWidths = dynamicIntArrayNew(columnCount + 100);
  sheet.cellHeights = dynamicIntArrayNew(rowCount + 1000);
  sheet.strings = dynamicStringArrayNew(cellCount + 1000);
  for (int i = 0; i < cellCount; i++) {
    String string = {0};
    dynamicStringArrayInsert(&sheet.strings, string, i);
  }
  for (int i = 0; i < columnCount; i++) {
    dynamicIntArrayInsert(&sheet.cellWidths, TEMP_CELL_WIDTH, i);
  }
  for (int i = 0; i < rowCount; i++) {
    dynamicIntArrayInsert(&sheet.cellHeights, TEMP_CELL_HEIGHT, i);
  }
  sheet.verticalPadding = 4;
  sheet.horizontalPadding = 4;
  sheet.columnCount = columnCount;
  sheet.rowCount = rowCount;
  return sheet;
}

// TODO: Make this function like sheetInsertRow and rename both this and sheetAppendColumn
void sheetAppendRow(Sheet *sheet, int cellIndex) { // TODO: test
  int selectedRow = cellIndex / sheet->columnCount;
  int startCellOfNewRow = selectedRow * sheet->columnCount;
  int startCellOfNextRow = startCellOfNewRow + sheet->columnCount;
  dynamicIntArrayInsert(&sheet->cellHeights, TEMP_CELL_HEIGHT, selectedRow);
  for (int i = startCellOfNewRow; i < startCellOfNextRow; i++) {
    String string = {0};
    dynamicStringArrayInsert(&sheet->strings, string, i);
  }
  sheet->rowCount++;
}

void sheetAppendColumn(Sheet *sheet, int cellIndex) { // TODO: debug
  int selectedColumn = cellIndex % sheet->columnCount;
  dynamicIntArrayInsert(&sheet->cellWidths, TEMP_CELL_WIDTH, selectedColumn); // TODO: is this insertion correct, same for sheetAppendRow
  sheet->columnCount++;
  for (int row = 0; row < sheet->rowCount; row++) {
    String string = {0};
    dynamicStringArrayInsert(&sheet->strings, string, row * sheet->columnCount + selectedColumn);
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
      sheet->selectedCell -= 1;
      break;
    }
    case ROW_DOWN: {
      PangoLayout *layout = initialisePangoLayout(program);
      int textHeightPixels = calculateTextHeightPixels(layout);
      int cellHeightPixels = calculateCellHeightPixels(textHeightPixels, *sheet);
      XWindowAttributes winAttribs = {0};
      XGetWindowAttributes(program.display, program.window, &winAttribs);
      int maxRowsOnScreen = calculateMaxRowsOnScreen(winAttribs, cellHeightPixels);

      sheet->selectedCell += sheet->columnCount;
      int anchorRow = viewport->anchorCell / sheet->columnCount;
      int selectedRow = sheet->selectedCell / sheet->columnCount;
      // selectedToAnchor is the range from the selectedRow to the anchorRow. We add one to this because the range is inclusive
      int selectedToAnchor = selectedRow - anchorRow + 1;
      if (selectedToAnchor > maxRowsOnScreen) {
        viewport->anchorCell += sheet->columnCount;
      }
      break;
    }
    case ROW_UP: {
      sheet->selectedCell -= sheet->columnCount;
      int anchorRow = viewport->anchorCell / sheet->columnCount;
      int selectedRow = sheet->selectedCell / sheet->columnCount;
      if (anchorRow > selectedRow) {
        viewport->anchorCell = sheet->selectedCell;
      }
      break;
    }
    case COLUMN_RIGHT: {
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

      sheet->selectedCell = viewport->anchorCell + maxRowsOnScreen / 2 * sheet->columnCount + sheet->selectedCell % sheet->columnCount;
      break;
    }
    case SELECTED_CELL_BOTTOM: {
      PangoLayout *layout = initialisePangoLayout(program);
      int textHeightPixels = calculateTextHeightPixels(layout);
      int cellHeightPixels = calculateCellHeightPixels(textHeightPixels, *sheet);
      XWindowAttributes winAttribs = {0};
      XGetWindowAttributes(program.display, program.window, &winAttribs);
      int maxRowsOnScreen = calculateMaxRowsOnScreen(winAttribs, cellHeightPixels);

      int rowShift = maxRowsOnScreen * sheet->columnCount - sheet->columnCount;
      int columnShift = sheet->selectedCell % sheet->columnCount;
      sheet->selectedCell = viewport->anchorCell + rowShift + columnShift;
      break;
    }
    case VIEWPORT_ROW_UP: {
      viewport->anchorCell -= sheet->columnCount;
      break;
    }
    case VIEWPORT_ROW_DOWN: {
      viewport->anchorCell += sheet->columnCount;
      break;
    }
    case VIEWPORT_PAGE_UP: {
      PangoLayout *layout = initialisePangoLayout(program);
      int textHeightPixels = calculateTextHeightPixels(layout);
      int cellHeightPixels = calculateCellHeightPixels(textHeightPixels, *sheet);
      XWindowAttributes winAttribs = {0};
      XGetWindowAttributes(program.display, program.window, &winAttribs);
      int maxRowsOnScreen = calculateMaxRowsOnScreen(winAttribs, cellHeightPixels);
      viewport->anchorCell -= maxRowsOnScreen * sheet->columnCount;

      int rowShift = maxRowsOnScreen * sheet->columnCount - sheet->columnCount;
      int columnShift = sheet->selectedCell % sheet->columnCount;
      sheet->selectedCell = viewport->anchorCell + rowShift + columnShift;
      break;
    }
    case VIEWPORT_PAGE_DOWN: {
      PangoLayout *layout = initialisePangoLayout(program);
      int textHeightPixels = calculateTextHeightPixels(layout);
      int cellHeightPixels = calculateCellHeightPixels(textHeightPixels, *sheet);
      XWindowAttributes winAttribs = {0};
      XGetWindowAttributes(program.display, program.window, &winAttribs);
      int maxRowsOnScreen = calculateMaxRowsOnScreen(winAttribs, cellHeightPixels);
      viewport->anchorCell += maxRowsOnScreen * sheet->columnCount;
      sheet->selectedCell = viewport->anchorCell + sheet->selectedCell % sheet->columnCount;
      break;
    }
    case VIEWPORT_HALF_PAGE_UP: {
      PangoLayout *layout = initialisePangoLayout(program);
      int textHeightPixels = calculateTextHeightPixels(layout);
      int cellHeightPixels = calculateCellHeightPixels(textHeightPixels, *sheet);
      XWindowAttributes winAttribs = {0};
      XGetWindowAttributes(program.display, program.window, &winAttribs);
      int maxRowsOnScreen = calculateMaxRowsOnScreen(winAttribs, cellHeightPixels);

      viewport->anchorCell -= maxRowsOnScreen / 2 * sheet->columnCount;
      if (maxRowsOnScreen % 2 == 1) {
        viewport->anchorCell -= sheet->columnCount;
      }

      int anchorRow = viewport->anchorCell / sheet->columnCount;
      int selectedRow = sheet->selectedCell / sheet->columnCount;
      // selectedToAnchor is the range from the selectedRow to the anchorRow. We add one to this because the range is inclusive
      int selectedToAnchor = selectedRow - anchorRow + 1;
      if (selectedToAnchor > maxRowsOnScreen) {
        int rowShift = maxRowsOnScreen * sheet->columnCount - sheet->columnCount;
        int columnShift = sheet->selectedCell % sheet->columnCount;
        sheet->selectedCell = viewport->anchorCell + rowShift + columnShift;
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

      viewport->anchorCell += maxRowsOnScreen / 2 * sheet->columnCount;
      if (maxRowsOnScreen % 2 == 1) {
        viewport->anchorCell += sheet->columnCount;
      }

      int anchorRow = viewport->anchorCell / sheet->columnCount;
      int selectedRow = sheet->selectedCell / sheet->columnCount;
      if (anchorRow > selectedRow) {
        sheet->selectedCell = viewport->anchorCell + sheet->selectedCell % sheet->columnCount;
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
  /*int maxRowsOnScreen = (winAttribs.height - cellHeightPixels) / cellHeightPixels;*/
  int maxRowsOnScreen = (winAttribs.height) / cellHeightPixels;

  return maxRowsOnScreen;
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
    rowNumberColumnWidth = intToString(rowsOnScreen).length * textWidthPixels + 2 * sheet.horizontalPadding;
  }
  int xoffset = 0;
  int yoffset = 0;
  printf("[DEBUG] viewport->anchorCell: %d\n", viewport->anchorCell);
  printf("[DEBUG] sheet.selectedCell: %d\n", sheet.selectedCell);
  printf("[DEBUG] anchorRow: %d\n", viewport->anchorCell / sheet.columnCount);
  printf("[DEBUG] selectedRow: %d\n", sheet.selectedCell / sheet.columnCount);
  yoffset -= 1 * viewport->anchorCell / sheet.columnCount * cellHeightPixels;
  /*int xoffset = clamp((winAttribs.width - sheetWidth) / 2 - sheet.horizontalPadding, rowNumberColumnWidth, INT_MAX);*/
  /*int yoffset = clamp((winAttribs.height - sheetHeight) / 2 - sheet.verticalPadding, columnNumberRowHeight, INT_MAX);*/
  /*yoffset -= viewport->anchorCell / sheet.columnCount * cellHeightPixels;*/



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

    String string = intToString(row + 1);
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



//******************************************//
//               Main                       //
//******************************************//

int main() {
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

  Sheet sheet = newSheet(13, 1);
  sheetCellAppend(sheet, 0, "helyo", 5);
  sheet.selectedCell = 0;

  Viewport viewport = {0};
  viewport.anchorCell = 0;
  /*viewport.maxCellCountTall = 3;*/
  /*viewport.maxCellCountWide = 3;*/
  /*viewport.hasFocus = TRUE;*/

  Screen *screen = XScreenOfDisplay(display, screen_number);
  int width = XWidthOfScreen(screen);
  int height = XHeightOfScreen(screen);
  cairo_surface_t *surface = cairo_xlib_surface_create(display, window, XDefaultVisual(display, screen_number), width, height);
  cairo_t *cr = cairo_create(surface);

  PangoFontDescription *desc = pango_font_description_from_string("Liberation Mono 20");

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



  XEvent event = {0};
  while (TRUE) {
    XNextEvent(display, &event);

    switch (event.type) {



      //////////////////////////////////////////////////////////////////
      // Expose
      //////////////////////////////////////////////////////////////////
      case Expose: {
        render(program, sheet, &viewport);
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
        KeySym keySymPressed = XLookupKeysym(&event.xkey, 0);
        char *translatedCharArray = calloc(100, sizeof(char));
        int translatedCharLength = XLookupString(&event.xkey, translatedCharArray, 100, NULL, NULL);
        char translatedChar = translatedCharArray[0];
        if (translatedCharLength == 0) {
          break;
        }

        printf("\n");
        printf("[DEBUG] translatedChar: %s\n", translatedCharArray);
        printf("[DEBUG] translatedChar (hex): %x\n", translatedChar);
        printf("[DEBUG] translatedCharLength: %d\n", translatedCharLength);
        printf("[DEBUG] event.xkey.state (hex): %x\n", event.xkey.state);
        printf("[DEBUG] event.xkey.state (hex): %x\n", event.xkey.state & ShiftMask);

        if (event.xkey.state & LockMask) {
          printf("[DEBUG] LockMask registered that Lock is on\n");
        }
        if (event.xkey.state & ControlMask) {
          printf("[DEBUG] ControlMask registered that Control is held\n");
        }
        if (event.xkey.state & ShiftMask) {
          printf("[DEBUG] ShiftMask registered that Shift is held\n");
        }



        ////////////////////////////////////////////////////////////////
        // INSERT MODE
        ////////////////////////////////////////////////////////////////
        if (sheet.insertMode == TRUE) {
          if (translatedChar == ESCAPE) {
            sheet.insertMode = FALSE;
            break;
          }
          else if (translatedChar == BACKSPACE) {
            sheetCellBackSpace(sheet, sheet.selectedCell, sheet.strings.data[sheet.selectedCell].length - 1);
          }
          else if (translatedChar < 0x20 || translatedChar == 0x7F) {
            break;
            // Not sure if this is a good blanket solution but doing
            // this to prevent ctrl chars being rendered to the
            // screen by typing e.g. CTRL-K or DELETE
            // is this an exhaustive list of ctrl chars?
          }
          else {
            sheetCellAppend(sheet, sheet.selectedCell, &translatedChar, translatedCharLength);
            program.lastAction.text = stringInsertChar(program.lastAction.text, translatedChar, program.lastAction.text.length);
          }
        }



        ////////////////////////////////////////////////////////////////
        // NORMAL MODE
        ////////////////////////////////////////////////////////////////
        else {
          {
            Action action = {0};
            if (event.xkey.state & ControlMask) {
              switch (keySymPressed) {
                case XK_e: {
                  action.action = VIEWPORT_ROW_DOWN;
                  normalModeAction(&sheet, &viewport, action);
                  break;
                }
                case XK_y: {
                  action.action = VIEWPORT_ROW_UP;
                  normalModeAction(&sheet, &viewport, action);
                  break;
                }
                case XK_b: {
                  action.action = VIEWPORT_PAGE_UP;
                  normalModeAction(&sheet, &viewport, action);
                  break;
                }
                case XK_f: {
                  action.action = VIEWPORT_PAGE_DOWN;
                  normalModeAction(&sheet, &viewport, action);
                  break;
                }
                case XK_u: {
                  action.action = VIEWPORT_HALF_PAGE_UP;
                  normalModeAction(&sheet, &viewport, action);
                  break;
                }
                case XK_d: {
                  action.action = VIEWPORT_HALF_PAGE_DOWN;
                  normalModeAction(&sheet, &viewport, action);
                  break;
                }
              }
            }
            else {
              switch (translatedChar) {
                case 'h': {
                  action.action = COLUMN_LEFT;
                  normalModeAction(&sheet, &viewport, action);
                  break;
                }
                case 'j': {
                  action.action = ROW_DOWN;
                  normalModeAction(&sheet, &viewport, action);
                  break;
                }
                case 'k': {
                  action.action = ROW_UP;
                  normalModeAction(&sheet, &viewport, action);
                  break;
                }
                case 'l': {
                  action.action = COLUMN_RIGHT;
                  normalModeAction(&sheet, &viewport, action);
                  break;
                }
                case 'M': {
                  action.action = SELECTED_CELL_CENTRE;
                  normalModeAction(&sheet, &viewport, action);
                  break;
                }
                case 'L': {
                  action.action = SELECTED_CELL_BOTTOM;
                  normalModeAction(&sheet, &viewport, action);
                  break;
                }
                case 'y': {
                  /*action.action = FIND_CELL;*/
                  /*normalModeAction(&sheet, &viewport, action);*/
                  /*break;*/
                }
                case 'e': {
                  /*action.action = FIND_CELL;*/
                  /*normalModeAction(&sheet, &viewport, action);*/
                  /*break;*/
                }
                case 'f': {
                  /*action.action = FIND_CELL;*/
                  /*normalModeAction(&sheet, &viewport, action);*/
                  /*break;*/
                }
                case 'i': {
                  action.action = INSERT_BEGIN;
                  normalModeAction(&sheet, &viewport, action);
                  program.lastAction = action;
                  program.lastAction.action = INSERT_TEXT;
                  break;
                }
                /* case 'I': { */
                /*   sheet.insertMode = TRUE; */
                /*   // INSERT MODE AT THE END OF THE STRING */
                /*   break; */
                /* } */
                case 'a': {
                  action.action = APPEND_COLUMN;
                  normalModeAction(&sheet, &viewport, action);
                  program.lastAction = action;
                  break;
                }
                case 'A': {
                  action.action = INSERT_COLUMN;
                  normalModeAction(&sheet, &viewport, action);
                  program.lastAction = action;
                  break;
                }
                case 'o': {
                  action.action = APPEND_ROW;
                  normalModeAction(&sheet, &viewport, action);
                  program.lastAction = action;
                  break;
                }
                case 'O': {
                  action.action = INSERT_ROW;
                  normalModeAction(&sheet, &viewport, action);
                  program.lastAction = action;
                  break;
                }
                case '.': { // TODO
                  normalModeAction(&sheet, &viewport, program.lastAction);
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
          }
        }
        render(program, sheet, &viewport);
        break;
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
        if (event.xbutton.button == Button1) {
          printf("Button1 press\n");
        }
        break;
      }
    }
  }


  return 0;
}


// TODO: 
// - l - bug fix - when at the furthest right column, do not change the selectedCell
// - H
// - g<motion>
// - gg
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
