#include "vt.h"

#define DEBUG

void vtPrintCharacter(void *pointer, char c, int x, int y) {
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

	vt->x = col;
	vt->y = row;
	wmove(vt->window, row, col);
}

void vtInsertLine(void *pointer, int y) {
	struct virtualTerminal *vt = (struct virtualTerminal*) pointer;
#ifdef DEBUG
	fprintf(stderr, "cbInsertLine\n");
#endif
	wmove(vt->window, y, 0);
	winsertln(vt->window);
}

void vtEraseLine(void *pointer, int y) {
	struct virtualTerminal *vt = (struct virtualTerminal*) pointer;
	int i;
	for(i = 0; i < vt->width; i++) {
		wmove(vt->window, y, i);
		waddch(vt->window, ' ');
	}
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

void vtCreate(int width, int height, int x, int y, int number) {
	struct tesiObject *to;
	struct virtualTerminal *vt;

	to = newTesiObject("/bin/cat", width, height);
	to->callback_printCharacter = &vtPrintCharacter;
	//to->callback_printString = &vtPrintString;
	to->callback_moveCursor = &vtMoveCursor;
	to->callback_insertLine = &vtInsertLine;
	to->callback_eraseLine = &vtEraseLine;
	to->callback_scrollRegion = &vtScrollRegion;
	to->callback_scrollUp = &vtScrollUp;
	to->callback_attributes = &vtAttributes;

	vt = malloc(sizeof(struct virtualTerminal));
	if(vt == NULL) {
		fprintf(stderr, "Error allocating memory for virtual terminal\n");
		return;
	}
	vt->id = number + 1;
	vt->state = VT_RUNNING;
	vt->border = 1;
	vt->padding = 0;

	vt->window = NULL;
	vt->wBorder = NULL;

	if(vt->border) {
#ifdef DEBUG
		fprintf(stderr, "Creating nCurses border window (rows,cols,y,x) %d %d %d %d\n", height, width, y, x);
#endif
		vt->wBorder = newwin(height, width, y, x);
		if(vt->wBorder == NULL) {
			fprintf(stderr, "Error creating nCurses border window\n");

		} else {
			//box(vt->wBorder, ACS_VLINE, ACS_HLINE);
			mvwprintw(vt->wBorder, 0, 0, "%i  ", vt->id);
			//width -= 2;
			//height -= 2;
			height--;
			//wnoutrefresh(vt->wBorder);
		}
	}

#ifdef DEBUG
	fprintf(stderr, "Creating nCurses window (rows,cols,y,x) %d %d %d %d\n", height, width, y, x);
#endif

	if(vt->border)
		vt->window = derwin(vt->wBorder, height, width, 1, 0);
	else
		vt->window = newwin(height, width, y, x);

	if(vt->window == NULL) {
		fprintf(stderr, "Error creating nCurses window\n");
		free(vt);
		return;
	}
	scrollok(vt->window, false);

	if(vt->border)
		wnoutrefresh(vt->wBorder);
	wnoutrefresh(vt->window);

	vt->width = width;
	vt->height = height;

	// use first line for terminal's number
	//mvwprintf(vt->wBorder);

	to->pointer = vt;
	virtualTerminals[number] = to;
}

/*
 * Takes a pointer to a virtualTerminal structure
 * Frees all elements, and then the structure itself
 * */
void vtDestroy(int index) {
	struct tesiObject *to;
	struct virtualTerminal *vt;

	to = virtualTerminals[index];
	// send kill signals to child processes
	vt = to->pointer;
	// not sure if the library code makes sure not to double-free
	if(vt)
		free(vt); // free our encapsulation structure
	deleteTesiObject(to);
	virtualTerminals[index] = NULL;
}


struct tesiObject *vtGet(int index) {
	return virtualTerminals[index];
}

void vtMove(int index, int x, int y, int width, int height) {
	struct tesiObject *to = virtualTerminals[index];
	struct virtualTerminal *vt;
	char message[64];

	if(to != NULL) {
		vt = (struct virtualTerminal*) to->pointer;
		vt->x = x;
		vt->y = y;
		height--; // because of title window
		to->width = width;
		to->height = height;
		vt->width = width;
		vt->height = height;

		// resize ncurses windows
		// send resize signal to app (will bash pass signal on to child?)
		// is there any way to check if bash has a child process?
		vtScrollRegion(vt, 0, height);
		snprintf(message, 63, "export LINES=%d \n", height);
		vtSend(index, message);
		snprintf(message, 63, "export COLUMNS=%d \n", width);
		vtSend(index, message);
		kill(to->pid, SIGWINCH);
		// re-number terminal in wBorder
	}
}

void vtSend(int index, char *input) {
	struct tesiObject *to = virtualTerminals[index];
	if(to != NULL) // set input to vt
		write(to->fd_input, input, strlen(input));
}

void vtHighlight(int index) {
	struct tesiObject *to;
	struct virtualTerminal *vt;
	int i, j;

	if(to != NULL) {
		for(i = 0; i < 10; i++) {
			to = vtGet(i);
			if(!to)
				continue;
			vt = (struct virtualTerminal*) to->pointer;
			if(i == index) { // for now just highlight terminal's title bar
				wattron(vt->wBorder, A_BOLD);
				wattron(vt->wBorder, A_REVERSE);
				for(j = 0; j < vt->width; j++) {
					wmove(vt->wBorder, 0, j);
					waddch(vt->wBorder, ' ');
				}
				mvwprintw(vt->wBorder, 0, 0, "%d", vt->id);
				wnoutrefresh(vt->wBorder);

			} else {
				wattroff(vt->wBorder, A_BOLD);
				wattroff(vt->wBorder, A_REVERSE);
				for(j = 0; j < vt->width; j++) {
					wmove(vt->wBorder, 0, j);
					waddch(vt->wBorder, ' ');
				}
				mvwprintw(vt->wBorder, 0, 0, "%d", vt->id);
				wnoutrefresh(vt->wBorder);
			}
		}
	}
}

void vtDrawBorders() {
}
