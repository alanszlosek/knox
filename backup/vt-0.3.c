#include "vt.h"

void vtDrawText(VTScreenView *view, int col, int row, char *mbstring, int length, int width) {
	struct virtualTerminal *vt = (struct virtualTerminal*) view->object;
	mbstring[length] = 0; // add null character just in case
	//fprintf(stderr, "DrawText. view: %d col: %d row: %d string: %s\n", (int) (view->object), col, row, mbstring);
	
	mvwaddnstr(vt->window, row, col, mbstring, length);
	//wmove((WINDOW*)view->object, row, col + length - 1);
}

void vtMoveCursor(VTScreenView *view, int col, int row) {
	struct virtualTerminal *vt = (struct virtualTerminal*) view->object;
	wmove(vt->window, row, col);
}

void vtClearRect(VTScreenView *view, int s_col, int s_row, int e_col, int e_row) {
	struct virtualTerminal *vt = (struct virtualTerminal*) view->object;
	int i,j;
	for(i = s_row; i <= e_row; i++) {
		for(j = s_col; j < e_col; j++)
			mvwaddch(vt->window, i, j, ' '); 
	}
}

void vtScroll(VTScreenView *view, int dest_row, int src_row, int num_line) {
	struct virtualTerminal *vt = (struct virtualTerminal*) view->object;
	//wscrl((WINDOW*)view->object, src_row - dest_row);
	mvwprintw((WINDOW*)vt->window, 0,0, "dest: %d src: %d num_line: %d\n", dest_row, src_row, num_line);
}

void vtRendition(VTScreenView *view, int bold, int blink, int inverse, int underline, int foreground, int background, char charset) {
	struct virtualTerminal *vt = (struct virtualTerminal*) view->object;
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
	//mvwaddstr((WINDOW*)view->object, 0, 0, "attributes");
}

void vtExit(VTScreenView *view) {
	struct virtualTerminal *vt = (struct virtualTerminal*) view->object;
	vt->fd = -1; // this will no longer be added to the select list
	//fprintf(stderr,"VT Exit\n");
}


struct virtualTerminal *vtCreate(int cols, int rows, int x, int y, int number) {
	char *vtArguments[2];

	struct virtualTerminal *vt;

	/*
	VTCore *vt;
	TerminalIO *tio;
	VTScreenView *view;
	WINDOW *window;
	*/

	vt = malloc(sizeof(struct virtualTerminal));
	vt->id = number;
	vt->state = VT_RUNNING;
	vt->border = 1;
	vt->padding = 0;
	
	if(vt->border) {
		vt->wBorder = newwin(rows, cols, y, x);
		box(vt->wBorder, ACS_VLINE, ACS_HLINE);
		mvwprintw(vt->wBorder, 0, 0, "%i ", vt->id);
		cols -= 2;
		rows -= 2;
		y += 1;
		x += 1;
		wnoutrefresh(vt->wBorder);
	}

	//fprintf(stderr, "Creating. cols: %d rows: %d x: %d y: %d\n", cols, rows, x, y);

	// iTerm Setup
	vtArguments[0] = (char*) getenv("SHELL");
	//if(vtArguments[0] == NULL)
		vtArguments[0] = "/bin/bash";
	vtArguments[1] = NULL;

	// new iTerm TerminalIO object
	vt->terminalIO = TtyTerminalIO_new(cols, rows, vtArguments[0], vtArguments); // width, height, program, argv
	if(vt->terminalIO == NULL) {
		perror("TtyTerminalIO_new error");
		exit(1);
	}
	vt->fd = TtyTerminalIO_get_associated_fd(vt->terminalIO); // store file descriptor for easy access

	/*
	if(vt->border)
		vt->window = derwin(vt->wBorder, rows, cols, y, x);
	else
	*/
		vt->window = newwin(rows, cols, y, x);

	if(vt->window == NULL) {
		perror("Error creating nCurses window");
		exit(1);
	}

	/*
	terminals[number] = window;
	mvwaddstr(window, 0, 0, "Hello");
	mvwaddstr(window, 21, 62, "#");
	wrefresh(window);
	*/

	// new Virtual Terminal Screen View object
	vt->screenView = malloc (sizeof (VTScreenView));
	if (vt->screenView == NULL) {
		perror("Error allocating VTScreenView struct");
		exit(1);
	}

	VTScreenView_init(vt->screenView);
	vt->screenView->object = vt; // make screenView have a pointer to its parent object, the virtualTerminal struct. It'll make many operations easier to perform, like dealing with a closed File Descriptor when the Virtual Terminal's process closes
	vt->screenView->clear_rect = vtClearRect;
	vt->screenView->draw_text = vtDrawText;
	vt->screenView->set_rendition = vtRendition;
	vt->screenView->scroll_view = vtScroll;
	vt->screenView->update_cursor_position = vtMoveCursor;

	// Start the iTerm core
	vt->core = VTCore_new(vt->terminalIO, cols, rows, 0); // terminalIO, cols, rows, history
	if(vt->core == NULL) {
		perror("VTCore_new error");
		exit(1);
	}

	// Tell the core with view to use ... can we have multiple?
	VTCore_set_screen_view(vt->core, vt->screenView);
	VTCore_set_exit_callback(vt->core, vtExit);
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
		delwin((WINDOW*)vt->window);
	if(vt->screenView)
		free(vt->screenView);
	if(vt->terminalIO)
		TtyTerminalIO_destroy(vt->terminalIO);
	if(vt->core)
		VTCore_destroy(vt->core);
	if(vt)
		free(vt); // free our encapsulation structure 
} 
