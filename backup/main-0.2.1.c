/*
version 0.2.1

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
	scrollok((WINDOW*)view->object, 1);
	wscrl((WINDOW*)view->object, src_row - dest_row);
	//mvwprintw((WINDOW*)view->object, 0,0, "dest: %d src: %d num_line: %d\n", dest_row, src_row, num_line);
	scrollok((WINDOW*)view->object, 0);
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
		on |= COLOR_PAIR(foreground);
	else
		off |= COLOR_PAIR(foreground);
	if(background)
		on |= COLOR_PAIR(background);
	else
		off |= COLOR_PAIR(background);
	*/
	wattron((WINDOW*)view->object, on);
	wattroff((WINDOW*)view->object, off);
	//mvwaddstr((WINDOW*)view->object, 0, 0, "attributes");
}

int main() {
	// iTerm related variables
	VTCore *vtA, *vtB, *vtCurrent;
	TerminalIO *tioA, *tioB;
	VTScreenView *viewA, *viewB;
	char *vtArguments[2];

	// nCurses related variables
	WINDOW *main, *left, *right, *current;
	int vtRows, vtCols;	

	// other
	int done;
	unsigned int input;
	char ch[2];
	int fdA, fdB, fdCurrent;
	fd_set fileDescriptors;
	struct timeval timeout;

	// Initialize Terminal
	done = 0;
	main = initscr();
	start_color();
	init_pair(1, COLOR_BLACK, COLOR_WHITE);
	bkgd(' ' | COLOR_PAIR(1));
	attron(COLOR_PAIR(1));
	keypad(main, true);
	noecho();
	refresh();

	getmaxyx(main, vtRows, vtCols);
	//vtRows = (vtRows / 2) - (vtRows % 2);
	vtCols = (vtCols / 2) - (vtCols % 2);

	left = newwin(vtRows,vtCols, 0, 0);
	right = newwin(vtRows, vtCols, 0, vtCols);
	current = left;

	// iTerm Setup
	vtArguments[0] = (char*) getenv("SHELL");
	if(vtArguments[0] == NULL)
		vtArguments[0] = "/bin/sh";
	vtArguments[1] = NULL;

	// new iTerm TerminalIO object
	tioA = TtyTerminalIO_new(vtCols, vtRows, vtArguments[0], vtArguments); // width, height, program, argv
	tioB = TtyTerminalIO_new(vtCols, vtRows, vtArguments[0], vtArguments); // width, height, program, argv
	if(tioA == NULL) {
		perror("TtyTerminalIO_new error");
		return 1;
	}

	// new Virtual Terminal Screen View object
	viewA = malloc (sizeof (VTScreenView));
	viewB = malloc (sizeof (VTScreenView));
	if (viewA == NULL) {
		perror ("Error allocating VTScreenView struct");
		return 1;
	}

	VTScreenView_init(viewA);
	viewA->object = left;
	viewA->clear_rect = vtClearRect;
	viewA->draw_text = vtDrawText;
	viewA->set_rendition = vtRendition;
	viewA->scroll_view = vtScroll;
	viewA->update_cursor_position = vtMoveCursor;

	VTScreenView_init(viewB);
	viewB->object = right;
	viewB->clear_rect = vtClearRect;
	viewB->draw_text = vtDrawText;
	viewB->set_rendition = vtRendition;
	viewB->scroll_view = vtScroll;
	viewB->update_cursor_position = vtMoveCursor;

	// Start the iTerm core
	//vt = VTCore_new(tio, SLtt_Screen_Cols, SLtt_Screen_Rows, 10); // terminalIO, cols, rows, history
	vtA = VTCore_new(tioA, vtCols, vtRows, 10); // terminalIO, cols, rows, history
	vtB = VTCore_new(tioB, vtCols, vtRows, 10); // terminalIO, cols, rows, history
	vtCurrent = vtA;
	if(vtA == NULL) {
		printf("Error initializing VTCore\n");
		return 1;
	}

	// Tell the core which view to use ... can we have multiple?
	VTCore_set_screen_view(vtA, viewA);
	VTCore_set_screen_view(vtB, viewB);

	timeout.tv_sec = 0;
	timeout.tv_usec = 100000;
	fdA = TtyTerminalIO_get_associated_fd(tioA);
	fdB = TtyTerminalIO_get_associated_fd(tioB);
	fdCurrent = fdA;

	while(!done) {
		FD_ZERO(&fileDescriptors);
		FD_SET(0, &fileDescriptors);
		FD_SET( fdA, &fileDescriptors);
		FD_SET( fdB, &fileDescriptors);
		timeout.tv_sec = 0;
		timeout.tv_usec = 100000;
		select(fdB + 1, &fileDescriptors, NULL, NULL, &timeout); 

		if(FD_ISSET(0, &fileDescriptors)) {
			input = getch(); //SLang_getkey();

			switch(input) {
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

				case 'Q':
					done = 1;
					break;

				default:
					ch[0] = (char) input;
					ch[1] = 0;

					write(fdCurrent, ch, 1);
					break;
			}
		}

		if(FD_ISSET(fdA, &fileDescriptors)) {
			VTCore_dispatch(vtA);
			//SLsmg_refresh();
			wrefresh(left);
		}

		if(FD_ISSET(fdB, &fileDescriptors)) {
			VTCore_dispatch(vtB);
			//SLsmg_refresh();
			wrefresh(right);
		}
	}
		
	free(viewA);
	free(viewB);
	VTCore_destroy(vtA);
	VTCore_destroy(vtB);
	TtyTerminalIO_destroy(tioA);
	TtyTerminalIO_destroy(tioB);

	// Shut down s-lang IO
	delwin(right);
	delwin(left);
	endwin();

	return 0;
}
