#ifndef VT_H
#define VT_H

#include "../../tesi/tesi.h"

#define VT_RUNNING 1
#define VT_PAUSED 2
#define VT_FOCUS 4

struct virtualTerminal {
	int fd; // should probably set this to -1 when closed
	// use active, or something for Running, Pause states
	short state; // VT_PAUSED VT_RUNNING

	int id; // virtual terminal number
	int x, y;
	int *width, *height;
	int border;
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

// Virtual Terminal creation/destroying
void vtCreate(int, int, int, int, int);
void vtDestroy(int);

// SLang interaction with terminals
// update X Y
void vtRun(int);
struct tesiObject *vtGet(int);
int vtReady(int);
void sendToVT(int, char*);

#endif
