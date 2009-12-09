#include "vt.h"

#define DEBUG 1

// CALLBACKS
void vtMoveCursor(void *pointer, int x, int y) {
#ifdef DEBUG
	fprintf(stderr, "cbMoveCursor: %d %d\n", x, y);
#endif
	wmove(pointer, y, x);
}

void vtPrintCharacter(void *pointer, char ch, int x, int y) {
#ifdef DEBUG
	fprintf(stderr, "cbPrintCharacter: %c (%d)\n", ch, (int)ch);
#endif
	waddch(pointer, ch);
}

void vtEraseCharacter(void *pointer, int x, int y) {
#ifdef DEBUG
	fprintf(stderr, "cbEraseCharacter\n");
#endif
	wdelch(pointer);
}

void vtScrollUp(void *pointer) {
#ifdef DEBUG
	fprintf(stderr, "cbScrollUp\n");
#endif
	scrollok(pointer, true);
	if(wscrl(pointer, 1) == ERR)
		fprintf(stderr, "ncurses said there was a scrolling error\n");
	scrollok(pointer, false);
}
void vtScrollDown(void *pointer) {
#ifdef DEBUG
	fprintf(stderr, "cbScrollDown\n");
#endif
	scrollok(pointer, true);
	if(wscrl(pointer, -1) == ERR)
		fprintf(stderr, "ncurses said there was a scrolling error\n");
	scrollok(pointer, false);
}
void vtScrollRegion(void *pointer, int start, int end) {
#ifdef DEBUG
	fprintf(stderr, "cbScrollRegion\n");
#endif
	wsetscrreg(pointer, start, end);
}

void vtInsertLine(void *pointer, int y) {
#ifdef DEBUG
	fprintf(stderr, "cbInsertLine\n");
#endif
	winsertln(pointer);
}

void vtEraseLine(void *pointer, int y) {
#ifdef DEBUG
	fprintf(stderr, "cbEraseLine\n");
#endif
	wclrtoeol(pointer);
}

void vtBell(void *pointer) {
#ifdef DEBUG
	fprintf(stderr, "cbBell\n");
#endif
}

void vtAttributes(void *pointer, short bold, short blink, short inverse, short underline, short foreground, short background, short charset) {
#ifdef DEBUG
	fprintf(stderr, "cbAttributes. bold %i blink %i inverse %i underline %i foreground %i background %i\n", bold, blink, inverse, underline, foreground, background);
#endif
	if(bold)
		wattron(pointer, A_BOLD);
	else
		wattroff(pointer, A_BOLD);
	if(inverse)
		wattron(pointer, A_STANDOUT);
	else
		wattroff(pointer, A_STANDOUT);

	// keep overriding the pair
	init_pair(7, foreground, background);
	wattron(pointer, COLOR_PAIR(7));
}

void vtExit(void *pointer) {
	struct virtualTerminal *vt = (struct virtualTerminal*) pointer;
	vt->fdActivity = vt->fdInput = -1; // this will no longer be added to the select list
	//fprintf(stderr,"VT Exit\n");
}

void vtCreate(int width, int height, int x, int y, int number) {
	struct tesiObject *to;
	struct virtualTerminal *vt;
	int border = 1;

	// height may be less if we're going to create a border window
	
	if(border) {
		to = newTesiObject("/bin/bash", width, height - 1);
	} else {
		to = newTesiObject("/bin/bash", width, height);
	}
	to->callback_printCharacter = &vtPrintCharacter;
	to->callback_moveCursor = &vtMoveCursor;
	to->callback_insertLine = &vtInsertLine;
	to->callback_eraseLine = &vtEraseLine;
	to->callback_eraseCharacter = &vtEraseCharacter;
	to->callback_scrollRegion = &vtScrollRegion;
	to->callback_scrollUp = &vtScrollUp;
	to->callback_scrollDown = &vtScrollDown;
	to->callback_attributes = &vtAttributes;
	to->callback_bell = &vtBell;

	vt = malloc(sizeof(struct virtualTerminal));
	if(vt == NULL) {
		fprintf(stderr, "Error allocating memory for virtual terminal\n");
		return;
	}
	vt->id = number + 1;
	vt->fdActivity = to->fd_activity;
	vt->fdInput = to->fd_input;
	vt->state = VT_RUNNING;
	vt->border = border;
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
	//scrollok(vt->window, false);

	if(vt->border)
		wnoutrefresh(vt->wBorder);
	wnoutrefresh(vt->window);

	vt->width = width;
	vt->height = height;

	// use first line for terminal's number
	//mvwprintf(vt->wBorder);

	to->pointer = vt->window;
	vt->pointer = to;
	virtualTerminals[number] = vt;
}

/*
 * Takes a pointer to a virtualTerminal structure
 * Frees all elements, and then the structure itself
 * */
void vtDestroy(int index) {
	struct tesiObject *to;
	struct virtualTerminal *vt;

	vt = virtualTerminals[index];

	// close out ncurses windows
	delwin(vt->window);
	delwin(vt->wBorder);
	// send kill signals to child processes
	to = vt->pointer;
	// not sure if the library code makes sure not to double-free
	if(vt)
		free(vt); // free our encapsulation structure
	deleteTesiObject(to);
	virtualTerminals[index] = NULL;
}



void vtMove(int index, int x, int y, int width, int height) {
	struct tesiObject *to;
	struct virtualTerminal *vt = virtualTerminals[index];
	char message[64];

	to = vt->pointer;

	if(to != NULL) {
		vt->x = to->x = x;
		vt->y = to->y = y;
		height--; // because of title window
		vt->width = to->width = width;
		vt->height = to->height = height;

		// resize ncurses windows
		// send resize signal to app (will bash pass signal on to child?)
		// is there any way to check if bash has a child process?
		vtScrollRegion(vt->window, 0, height);
		snprintf(message, 63, "export LINES=%d \n", height);
		vtSend(index, message);
		snprintf(message, 63, "export COLUMNS=%d \n", width);
		vtSend(index, message);
		kill(to->pid, SIGWINCH);
		// re-number terminal in wBorder
	}
}

struct virtualTerminal *vtGet(int index) {
	if(index >= 0 && index < 10) {
		return virtualTerminals[index];
	} else {
		return NULL;
	}
}

void vtSend(int index, char *input) {
	struct virtualTerminal *vt = virtualTerminals[index];
	if(vt != NULL) // set input to vt
		write(vt->fdInput, input, strlen(input));
}

void vtHighlight(int index) {
	struct tesiObject *to;
	struct virtualTerminal *vt;
	int i, j;

	if(to != NULL) {
		for(i = 0; i < 10; i++) {
			vt = vtGet(i);
			if(!vt)
				continue;
			//vt = (struct virtualTerminal*) to->pointer;
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
