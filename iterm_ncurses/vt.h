#ifndef VT_H
#define VT_H

#include <stdlib.h>
#include <string.h>
#include <ncurses.h>

#define VT_RUNNING 1
#define VT_PAUSED 2
#define VT_FOCUS 4
#define VT_VISIBLE 8

//#define DEBUG

struct virtualTerminal {
	VTCore *core;
	VTScreenView *screenView;
	TerminalIO *terminalIO;
	WINDOW *window, *wBorder;
	int fd; // should probably set this to -1 when closed
	// use active, or something for Running, Pause states
	short state; // VT_PAUSED VT_RUNNING VT_VISIBLE

	int id; // virtual terminal number
	short int border, padding;
	short int width, height;
	short int x, y;
};

struct virtualTerminal *virtualTerminals[10];
short int vt_colors[8][8];

// callbacks for iterm
void vtDrawText(VTScreenView*, int, int, char*, int, int);
void vtMoveCursor(VTScreenView*, int, int);
void vtClearRect(VTScreenView*, int, int, int, int);
void vtScroll(VTScreenView*, int, int, int);
void vtRendition(VTScreenView*, int, int, int, int, int, int, char);
void vtExit(VTScreenView*);

// Virtual Terminal creation/destroying
void vtCreate(int, int, int, int, int);
void vtDestroy(int);

// interaction with terminals
// update X Y
void vtRun(int);
struct virtualTerminal *vtGet(int);
int vtReady(int);
void vtSend(int, char*);
void vtHighlight(int);
void vtDrawBorders();

#endif
/*
struct virtualTerminal {
	int fd; // should probably set this to -1 when closed
	// use active, or something for Running, Pause states
	short state; // VT_PAUSED VT_RUNNING

	int id; // virtual terminal number
	int x, y;
	int width, height;
	WINDOW *window, *wBorder;
	int border, padding;
};

struct tesiObject *tesiObjects[10];

// callbacks for iterm
void vtPrintCharacter(void*, char, int, int);
void vtMoveCursor(void*, int, int);
void vtInsertLine(void*, int);
void vtEraseLine(void*, int);
void vtScrollRegion(void*, int, int);
void vtScrollUp(void*);
void vtAttributes(void*, short, short, short, short, short, short, short);
void vtExit(void*);
#endif
*/
