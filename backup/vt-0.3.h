#ifndef VT_H
#define VT_H

#include <iterm/core.h>
#include <iterm/unix/ttyio.h>
#include <ncurses.h>

#define VT_RUNNING 1
#define VT_PAUSED 2
#define VT_FOCUS 4

struct virtualTerminal {
	VTCore *core;
	VTScreenView *screenView;
	TerminalIO *terminalIO;
	WINDOW *window, *wBorder;
	int fd; // should probably set this to -1 when closed
	// use active, or something for Running, Pause states
	short state; // VT_PAUSED VT_RUNNING

	int id; // virtual terminal number
	short int border, padding;
};

// callbacks for iterm
void vtDrawText(VTScreenView*, int, int, char*, int, int);
void vtMoveCursor(VTScreenView*, int, int);
void vtClearRect(VTScreenView*, int, int, int, int);
void vtScroll(VTScreenView*, int, int, int);
void vtRendition(VTScreenView*, int, int, int, int, int, int, char);
void vtExit(VTScreenView*);

// Virtual Terminal creation/destroying
struct virtualTerminal *vtCreate(int, int, int, int, int);
void vtDestroy(void*);

#endif
