/*
version 0.2.2

WORKING

draw text
clear rectangle
attributes other than color

*/

#include <sys/select.h>
#include <iterm/core.h>
#include <iterm/unix/ttyio.h>

#include <ncurses.h>

void vtDrawText(VTScreenView *view, int col, int row, char *mbstring, int length, int width) {
	mbstring[length] = 0;
	fprintf(stderr, "DrawText. col: %d row: %d string: %s\n", col, row, mbstring);
	mvwaddnstr((WINDOW*)view->object, row, col, mbstring, length);
	wmove((WINDOW*)view->object, row, col + length - 1);
}

void vtMoveCursor(VTScreenView *view, int col, int row) {
	wmove((WINDOW*)view->object, row, col);
}

void vtClearRect(VTScreenView *view, int s_col, int s_row, int e_col, int e_row) {
	int i,j;
	for(i = s_row; i <= e_row; i++) {
		for(j = s_col; j < e_col; j++)
			mvwaddch((WINDOW*)view->object, i, j, ' '); 
	}
}

void vtScroll(VTScreenView *view, int dest_row, int src_row, int num_line) {
	//wscrl((WINDOW*)view->object, src_row - dest_row);
	mvwprintw((WINDOW*)view->object, 0,0, "dest: %d src: %d num_line: %d\n", dest_row, src_row, num_line);
}

void vtRendition(VTScreenView *view, int bold, int blink, int inverse, int underline, int foreground, int background, char charset) {
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
	wattron((WINDOW*)view->object, on);
	wattroff((WINDOW*)view->object, off);
	//mvwaddstr((WINDOW*)view->object, 0, 0, "attributes");
}

void vtExit(VTScreenView *view) {
	perror("VT Exit");
}


VTCore *vtCreate(int cols, int rows, int x, int y) {
	char *vtArguments[2];
	VTCore *vt;
	TerminalIO *tio;
	VTScreenView *view;
	WINDOW *window;

	// iTerm Setup
	vtArguments[0] = (char*) getenv("SHELL");
	//if(vtArguments[0] == NULL)
		vtArguments[0] = "/bin/bash";
	vtArguments[1] = NULL;

	// new iTerm TerminalIO object
	tio = TtyTerminalIO_new(cols, rows, vtArguments[0], vtArguments); // width, height, program, argv
	if(tio == NULL) {
		perror("TtyTerminalIO_new error");
		exit(1);
	}

	// new Virtual Terminal Screen View object
	view = malloc (sizeof (VTScreenView));
	if (view == NULL) {
		perror("Error allocating VTScreenView struct");
		exit(1);
	}

	window = newwin(rows, cols, y, x);

	VTScreenView_init(view);
	view->object = window;
	view->clear_rect = vtClearRect;
	view->draw_text = vtDrawText;
	view->set_rendition = vtRendition;
	view->scroll_view = vtScroll;
	view->update_cursor_position = vtMoveCursor;

	// Start the iTerm core
	vt = VTCore_new(tio, cols, rows, 0); // terminalIO, cols, rows, history
	if(vt == NULL) {
		perror("VTCore_new error");
		exit(1);
	}

	// Tell the core with view to use ... can we have multiple?
	VTCore_set_screen_view(vt, view);
	VTCore_set_exit_callback(vt, vtExit);
	return vt;
}

void vtDestroy(VTCore *vt) {
	VTScreenView *view;
	TerminalIO *tio;	

	view = (VTScreenView*) vt->screen;
	tio = vt->host_io;

	delwin(view->object);
	free(view);
	TtyTerminalIO_destroy(tio);
	VTCore_destroy(vt);
} 

int main() {
	// iTerm related variables
	VTCore *virtuals[4];
	VTScreenView *tempScreenView;
	/*
	VTCore *vtA, *vtB, *vtCurrent;
	TerminalIO *tioA, *tioB;
	VTScreenView *viewA, *viewB;
	*/

	// nCurses related variables
	WINDOW *main, *left, *right, *current;
	int vtRows, vtCols;	

	// other
	unsigned int input;
	char ch[2];
	int i, j, done, fdMax, fdCurrent;
	fd_set fileDescriptors;
	struct timespec timeout;

	// Initialize Terminal
	main = initscr();
	start_color();
	keypad(main, true);
	noecho();
	refresh();

	// Initialize Other Variables
	done = fdMax = 0;
	//FD_ZERO(&fileDescriptors);
	timeout.tv_sec = 0;
	timeout.tv_nsec = 1000000000; // 10E-9

	getmaxyx(main, vtRows, vtCols);
	vtRows = (vtRows / 2); // - (vtRows % 4);
	vtCols = (vtCols / 2); // - (vtCols % 4);

	//scrollok(left, TRUE);
	//scrollok(right, TRUE);
	current = left;

	// add standard input to File Descriptor set
	//FD_SET(0, &fileDescriptors);

	for(i = 0; i < 4; i++) {
		virtuals[i] = (VTCore*) vtCreate(vtCols, vtRows, (i % 2) * vtCols, (i > 1 ? vtRows : 0) );
		j = TtyTerminalIO_get_associated_fd(virtuals[i]->host_io);
		if(j > fdMax)
			fdMax = j;

		//FD_SET(j, &fileDescriptors);
	}
	fdCurrent = TtyTerminalIO_get_associated_fd(virtuals[0]->host_io);

	while(!done) {
		FD_ZERO(&fileDescriptors);
		FD_SET(0, &fileDescriptors);
		for(i = 0; i < 4; i++) {
			j = TtyTerminalIO_get_associated_fd(virtuals[i]->host_io);

			FD_SET(j, &fileDescriptors);
		}

		pselect(fdMax + 1, &fileDescriptors, NULL, NULL, &timeout, NULL); 

		if(FD_ISSET(0, &fileDescriptors)) {
			input = getch(); //SLang_getkey();

			switch(input) {
/*
				case KEY_NPAGE:
					current = right;
					vtCurrent = vtB;
					fdCurrent = fdB;
					break;
				case KEY_PPAGE:
					current = left;
					vtCurrent = vtA;
					fdCurrent = fdA;
					break;
*/

				case 'Q':
					done = 1;
					break;

				default:
					ch[0] = (char) input;
					ch[1] = 0;

					//write(fdCurrent, ch, 1);
					break;
			}
		}

		for(i = 0; i < 4; i++) {
			j = TtyTerminalIO_get_associated_fd(virtuals[i]->host_io);
			if(FD_ISSET(j, &fileDescriptors)) {
				VTCore_dispatch(virtuals[i]);
				//SLsmg_refresh();
				tempScreenView = (VTScreenView*) virtuals[i]->screen;
				wrefresh((WINDOW*) tempScreenView->object);
			}
		}
	}
		

	// Shut down s-lang IO
	for(i = 0; i < 4; i++) {
		vtDestroy(virtuals[i]);
		virtuals[i] = NULL;
	}
	endwin();

	return 0;
}
