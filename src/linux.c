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

typedef char Boolean;
typedef struct Color {
  double red;
  double blue;
  double green;
  double alpha;
} Color;

typedef struct String {
  char *value;
  int length;
} String;

typedef struct Program {
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

  Boolean shiftDown;
  char lastCharKeyPressed;
  String lastTextInserted;
} Program;



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

typedef struct DynamicStringArray {
  int length;
  int capacity;
  String *data;
} DynamicStringArray;

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

typedef struct DynamicIntArray {
  int length;
  int capacity;
  int *data;
} DynamicIntArray;

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

typedef struct Sheet {
  DynamicIntArray cellWidths;
  DynamicIntArray cellHeights;
  DynamicStringArray strings;
  int columnCount;
  int rowCount;
  int selectedCell;
  Boolean insertMode;
  int verticalPadding;
  int horizontalPadding;
} Sheet;

const int TEMP_CELL_WIDTH = 15;
const int TEMP_CELL_HEIGHT = 3;

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

char handleNormalModeInput(Sheet *sheet, char charKeyPressed, Boolean useRecordedCommand, char lastCharKeyPressed, String text) {
  switch (charKeyPressed) {
    case 'h': {
      sheet->selectedCell -= 1;
      break;
    }
    case 'j': {
      sheet->selectedCell += sheet->columnCount;
      break;
    }
    case 'k': {
      sheet->selectedCell -= sheet->columnCount;
      break;
    }
    case 'l': {
      sheet->selectedCell += 1;
      break;
    }
    case 'i': {
      if (useRecordedCommand && text.length) {
        sheetCellAppend(*sheet, sheet->selectedCell, text.value, text.length);
      }
      else {
        sheet->insertMode = TRUE;
      }
      lastCharKeyPressed = 'i';
      // INSERT MODE AT THE START OF THE STRING. Maybe ii does this, i puts you in "normal text mode" as opposed to "normal sheet mode"
      break;
    }
    /* case 'I': { */
    /*   sheet->insertMode = TRUE; */
    /*   // INSERT MODE AT THE END OF THE STRING */
    /*   break; */
    /* } */
    case 'a': {
      sheetAppendColumn(sheet, sheet->selectedCell + 1);
      int selectedRow = sheet->selectedCell / (sheet->columnCount - 1);
      sheet->selectedCell += selectedRow + 1; // sheetAppendColumn will add insert a cell for each row
      lastCharKeyPressed = 'a';
      break;
    }
    case 'A': {
      // INSERT NEW COLUMN
      sheetAppendColumn(sheet, sheet->selectedCell);
      int selectedRow = sheet->selectedCell / (sheet->columnCount - 1);
      sheet->selectedCell += selectedRow; // sheetAppendColumn will add insert a cell for each row
      lastCharKeyPressed = 'A';
      break;
    }
    case 'o': {
      sheetAppendRow(sheet, sheet->selectedCell + sheet->columnCount);
      sheet->selectedCell += sheet->columnCount;
      lastCharKeyPressed = 'o';
      break;
    }
    case 'O': {
      sheetAppendRow(sheet, sheet->selectedCell);
      lastCharKeyPressed = 'O';
      break;
    }
    case '.': { // TODO
      handleNormalModeInput(sheet, lastCharKeyPressed, TRUE, 0, text);
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
  printf("lastCharKeyPressed: %c\n", lastCharKeyPressed);
  return lastCharKeyPressed;
}



//******************************************//
//               Render                     //
//******************************************//

void render(Program program, Sheet sheet) {
  XWindowAttributes winAttribs = {0};
  XGetWindowAttributes(program.display, program.window, &winAttribs);

  PangoLayout *layout = pango_cairo_create_layout(program.cr);
  pango_layout_set_font_description(layout, program.font);
  pango_layout_set_text(layout, "a", -1);
  float textScale = 0.5;
  PangoRectangle logicalRectPixels;
  pango_layout_get_pixel_extents(layout, NULL, &logicalRectPixels);
  int textScaledHeightPixels = textScale * logicalRectPixels.height;
  int textScaledWidthPixels = textScale * logicalRectPixels.width;

  int cellWidth = TEMP_CELL_WIDTH * textScaledWidthPixels + 2 * sheet.horizontalPadding;
  int cellHeight = TEMP_CELL_HEIGHT * textScaledHeightPixels + 2 * sheet.verticalPadding;
  int sheetWidth = sheet.columnCount * cellWidth;
  int sheetHeight = sheet.rowCount * cellHeight;

  int rowNumberColumnWidth = 0;
  int columnNumberRowHeight = textScaledHeightPixels + 2 * sheet.verticalPadding;
  {
    int maxRowsOnScreen = (winAttribs.height - cellHeight) / cellHeight;
    int rowsOnScreen = maxRowsOnScreen > sheet.rowCount ? sheet.rowCount : maxRowsOnScreen;
    rowNumberColumnWidth = intToString(rowsOnScreen).length * textScaledWidthPixels + 2 * sheet.horizontalPadding;
  }
  int xoffset = clamp((winAttribs.width - sheetWidth) / 2 - sheet.horizontalPadding, rowNumberColumnWidth, INT_MAX);
  int yoffset = clamp((winAttribs.height - sheetHeight) / 2 - sheet.verticalPadding, columnNumberRowHeight, INT_MAX);



  XClearWindow(program.display, program.window);

  XSetForeground(program.display, program.gc, program.background.pixel);
  XFillRectangle(program.display, program.window, program.gc, 0, 0, winAttribs.width, winAttribs.height);

  // Highlight the selected Cell
  { // Use braces here to make it clear that the variables defined are only used here and not lower in the function
    int row = sheet.selectedCell / sheet.columnCount;
    int column = sheet.selectedCell % sheet.columnCount;
    int x = TEMP_CELL_WIDTH * textScaledWidthPixels * column + xoffset + 2 * column * sheet.horizontalPadding;
    int y = TEMP_CELL_HEIGHT * textScaledHeightPixels * row + yoffset + 2 * row * sheet.verticalPadding;
    XSetForeground(program.display, program.gc, program.highlight.pixel);
    XFillRectangle(program.display, program.window, program.gc, x, y, cellWidth, cellHeight);
  }

  PangoRectangle logicalRectPangoUnits;
  pango_layout_get_extents(layout, NULL, &logicalRectPangoUnits);
  // int textScaledHeightPangoUnits = textScale * logicalRectPangoUnits.height;
  // int textScaledWidthPangoUnits = textScale * logicalRectPangoUnits.width;

  // Render text for each cell
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
    int x = xoffset + column * TEMP_CELL_WIDTH * textScaledWidthPixels + 2 * column * sheet.horizontalPadding + sheet.horizontalPadding;
    int y = yoffset + row * TEMP_CELL_HEIGHT * textScaledHeightPixels + 2 * row * sheet.verticalPadding + sheet.verticalPadding;
    cairo_translate(program.cr, x, y);
    cairo_scale(program.cr, textScale, textScale);
    cairo_set_source_rgba(program.cr, program.text.red, program.text.green, program.text.blue, program.text.alpha);

    pango_cairo_show_layout(program.cr, layout);

    cairo_restore(program.cr);
  }
  
  // Render text for row numbering & column lettering
  for (int column = 0; column < sheet.columnCount; column++) {
    cairo_save(program.cr);

    String string = intToLetters(column);
    pango_layout_set_text(layout, string.value, string.length);
    pango_layout_set_width(layout, TEMP_CELL_WIDTH * logicalRectPangoUnits.width + 2);
    pango_layout_set_height(layout, TEMP_CELL_HEIGHT * logicalRectPangoUnits.height + 2);
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
    pango_cairo_update_layout(program.cr, layout);

    int x = xoffset + column * TEMP_CELL_WIDTH * textScaledWidthPixels + 2 * column * sheet.horizontalPadding + sheet.horizontalPadding;
    int y = yoffset + sheet.verticalPadding - textScaledHeightPixels - 2 * sheet.verticalPadding;
    cairo_translate(program.cr, x, y);
    cairo_scale(program.cr, textScale, textScale);
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
    int y = yoffset + row * TEMP_CELL_HEIGHT * textScaledHeightPixels + 2 * row * sheet.verticalPadding + sheet.verticalPadding;
    cairo_translate(program.cr, x, y);
    cairo_scale(program.cr, textScale, textScale);
    cairo_set_source_rgba(program.cr, program.text.red, program.text.green, program.text.blue, program.text.alpha);

    pango_cairo_show_layout(program.cr, layout);

    cairo_restore(program.cr);
  }

  /*
  pango_font_description_free(desc);
  g_object_unref(layout);
  cairo_destroy(program.cr);
  */



  // Draw the Rows and Columns
  XSetForeground(program.display, program.gc, program.foreground.pixel);
  XDrawLine(program.display, program.window, program.gc, winAttribs.x, winAttribs.y + yoffset, winAttribs.x + winAttribs.width, winAttribs.y + yoffset);
  for (int i = 0; i < sheet.rowCount; i++) {
    int y = winAttribs.y + yoffset + cellHeight;
    int x1 = winAttribs.x;
    int x2 = winAttribs.x + winAttribs.width;
    XDrawLine(program.display, program.window, program.gc, x1, y, x2, y);
    yoffset += cellHeight;
  }

  XDrawLine(program.display, program.window, program.gc, winAttribs.x + xoffset, winAttribs.y, winAttribs.x + xoffset, winAttribs.y + winAttribs.height);
  for (int i = 0; i < sheet.columnCount; i++) {
    int x = winAttribs.x + xoffset + cellWidth;
    int y1 = winAttribs.y;
    int y2 = winAttribs.y + winAttribs.height;
    XDrawLine(program.display, program.window, program.gc, x, y1, x , y2);
    xoffset += cellWidth;
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

  Sheet sheet = newSheet(3, 3);
  sheetCellAppend(sheet, 0, "helyo", 5);
  sheetCellAppend(sheet, 10, "helyo", 5);
  sheetCellAppend(sheet, 20, "helyo", 5);
  sheetCellAppend(sheet, 21, "helyo", 5);
  sheet.selectedCell = 11;

  Screen *screen = XScreenOfDisplay(display, screen_number);
  int width = XWidthOfScreen(screen);
  int height = XHeightOfScreen(screen);
  cairo_surface_t *surface = cairo_xlib_surface_create(display, window, XDefaultVisual(display, screen_number), width, height);
  cairo_t *cr = cairo_create(surface);

  PangoFontDescription *desc = pango_font_description_from_string("Liberation Mono 20");

  Program program = {0};
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
      case Expose: {
        render(program, sheet);
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
      case KeyPress: {
        KeySym keysym = XLookupKeysym(&event.xkey, 0);
        char *keyPressed = XKeysymToString(keysym);
        if (!keyPressed)
          break;

        if (stringsEqual("Shift_L", 7, keyPressed)) {
          program.shiftDown = TRUE;
        }

        if (sheet.insertMode == TRUE && !IsModifierKey(keysym) && !IsFunctionKey(keysym)) {
          if (stringsEqual("Escape", 6, keyPressed)) {
            sheet.insertMode = FALSE;
            break;
          }
          else if (stringsEqual("space", 5, keyPressed)) {
            sheetCellAppend(sheet, sheet.selectedCell, " ", 1);
          }
          else if (stringsEqual("Return", 6, keyPressed)) {
            sheetCellAppend(sheet, sheet.selectedCell, "\n", 1);
          }
          else if (stringsEqual("BackSpace", 9, keyPressed)) {
            sheetCellBackSpace(sheet, sheet.selectedCell, sheet.strings.data[sheet.selectedCell].length - 1);
          }
          else {
            char valueToInsert = *keyPressed;
            if (program.shiftDown) {
              valueToInsert = keyToUpper(*keyPressed);
            }
            int valueToInsertLength = 1;
            sheetCellAppend(sheet, sheet.selectedCell, &valueToInsert, valueToInsertLength);
            program.lastTextInserted = stringInsertChar(program.lastTextInserted, valueToInsert, program.lastTextInserted.length);
          }
        }
        else {
          char charKeyPressed = keyPressedToChar(keyPressed);
          if (program.shiftDown) {
            charKeyPressed = keyToUpper(charKeyPressed);
          }
          String placeholder = {0};
          program.lastCharKeyPressed = handleNormalModeInput(&sheet, charKeyPressed, FALSE, program.lastCharKeyPressed, program.lastTextInserted);
        }
        render(program, sheet);
        break;
      }
      case KeyRelease: {
        KeySym keysym = XLookupKeysym(&event.xkey, 0);
        char *keyReleased = XKeysymToString(keysym);
        if (!keyReleased)
          break;
        if (stringsEqual("Shift_L", 7, keyReleased)) {
          program.shiftDown = FALSE;
        }
        break;
      }
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
// ignore function keys
// make the renderer render things relative to a point so that a cells text, extents, etc are all 'stuck' together
// make i insert before text and a insert after text
// Support keyToUpper for non alphabet characters such as $
// Support scrolling
// '.' doesnt work for certain input operations, i think my string handling is broken
//
// Done:
// centre view
// handle space insertions properly
// add a gap between cells
// only display text in cell extent
// Escape goes to normal mode
// BackSpace deletes text
// add 'o' or 'O' to insert new row and 'a' or 'A' to insert new column
// support uppercase character insertion and uppercase key actions
// Add line numbers and column characters
// Make '.' repeat the last command
//
//
// Requirements:
// Edit cells
// add rows and columns
//
