#include "vt.h"

#define DEBUG

void vtPrintCharacter(void *pointer, char c) {
	struct virtualTerminal *vt = (struct virtualTerminal*) pointer;
#ifdef DEBUG
	fprintf(stderr, "Printing: %c\n", c);
#endif
	waddch(vt->window, c);
}

void vtMoveCursor(void *pointer, int col, int row) {
	struct virtualTerminal *vt = (struct virtualTerminal*) pointer;
#ifdef DEBUG
	fprintf(stderr, "Move character %d %d\n", col,row);
#endif

	wmove(vt->window, row, col);
}

void vtInsertLine(void *pointer) {
	struct virtualTerminal *vt = (struct virtualTerminal*) pointer;
#ifdef DEBUG
	fprintf(stderr, "cbInsertLine\n");
#endif
	winsertln(vt->window);
}

void vtEraseLine(void *pointer) {
	struct virtualTerminal *vt = (struct virtualTerminal*) pointer;
	wclrtoeol(vt->window);
}

void vtScrollRegion(void *pointer, int start, int end) {
	struct virtualTerminal *vt = (struct virtualTerminal*) pointer;
#ifdef DEBUG
	fprintf(stderr, "cbScrollRegion\n");
#endif
	wsetscrreg(vt->window, start, end);
}

void vtScrollUp(void *pointer) {
	struct virtualTerminal *vt = (struct virtualTerminal*) pointer;
#ifdef DEBUG
	fprintf(stderr, "cbScrollUp\n");
#endif
	scrollok(vt->window, true);
	wscrl(vt->window, 1);
	scrollok(vt->window, false);
}

void vtAttributes(void *pointer, short bold, short blink, short inverse, short underline, short foreground, short background, short charset) {
	struct virtualTerminal *vt = (struct virtualTerminal*) pointer;
	int on, off;
	on = off = 0;
	if(bold)
		on |= A_BOLD;
	else
		off |= A_BOLD;
	if(blink)
		on |= A_BLINK;
	else
		off |= A_BLINK;
	if(inverse)
		on |= A_REVERSE;
	else
		off |= A_REVERSE;
	if(underline)
		on |= A_UNDERLINE;
	else
		off |= A_UNDERLINE;
	/*
	if(foreground)
		on |= foreground;
	else
		off |= foreground;
	if(background)
		on |= background;
	else
		off |= background;
	*/
	wattron(vt->window, on);
	wattroff(vt->window, off);
}

void vtExit(void *pointer) {
	struct virtualTerminal *vt = (struct virtualTerminal*) pointer;
	vt->fd = -1; // this will no longer be added to the select list
	//fprintf(stderr,"VT Exit\n");
}


struct virtualTerminal *vtCreate(int cols, int rows, int x, int y, int number) {
	char *vtArguments[2];

	struct virtualTerminal *vt;

	vt = malloc(sizeof(struct virtualTerminal));
	if(vt == NULL) {
		fprintf(stderr, "Error allocating memory for virtual terminal\n");
		return NULL;
	}
	vt->id = number;
	vt->state = VT_RUNNING;
	vt->border = 1;
	vt->padding = 0;

	vt->window = NULL;
	vt->wBorder = NULL;

	if(vt->border) {
#ifdef DEBUG
		fprintf(stderr, "Creating nCurses border window (rows,cols,y,x) %d %d %d %d\n", rows, cols, y, x);
#endif
		vt->wBorder = newwin(rows, cols, y, x);
		if(vt->wBorder == NULL) {
			fprintf(stderr, "Error creating nCurses border window\n");

		} else {
			box(vt->wBorder, ACS_VLINE, ACS_HLINE);
			mvwprintw(vt->wBorder, 0, 0, "%i ", vt->id);
			cols -= 2;
			rows -= 2;
			/*
			y += 1;
			x += 1;
			*/
			wnoutrefresh(vt->wBorder);
		}
	}

#ifdef DEBUG
	fprintf(stderr, "Creating nCurses window (rows,cols,y,x) %d %d %d %d\n", rows, cols, y, x);
#endif

	if(vt->border)
		vt->window = derwin(vt->wBorder, rows, cols, 1, 1);
	else
		vt->window = newwin(rows, cols, y, x);

	if(vt->window == NULL) {
		fprintf(stderr, "Error creating nCurses window\n");
		free(vt);
		return NULL;
	}

	//scrollok(vt->window, true);
	return vt;
}

/*
 * Takes a pointer to a virtualTerminal structure
 * Frees all elements, and then the structure itself
 * */
void vtDestroy(void *pointer) {
	struct virtualTerminal *vt = (struct virtualTerminal*) pointer;

	// not sure if the library code makes sure not to double-free
	if(vt->window)
		delwin(vt->window);
	if(vt->wBorder)
		delwin(vt->wBorder);
	if(vt)
		free(vt); // free our encapsulation structure
}
