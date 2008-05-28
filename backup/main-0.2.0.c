#include <sys/select.h>
#include <iterm/core.h>
#include <iterm/unix/ttyio.h>

#include <ncurses.h>

#define VTROWS 40
#define VTCOLS 40

WINDOW *left, *right, *current;

void vtDrawText(VTScreenView *view, int col, int row, char *mbstring, int length, int width) {
	/*
	SLsmg_gotorc(row, col);
	SLsmg_write_nchars(mbstring, length);
	SLsmg_gotorc(row, col + length - 1);
	*/

	mvwaddnstr((WINDOW*)view->object, row, col, mbstring, length);
}

void vtMoveCursor(VTScreenView *view, int col, int row) {
	//SLsmg_gotorc(row, col);
}

void vtClearRect(VTScreenView *view, int s_col, int s_row, int e_col, int e_row) {
	/*
	int i,j;
	for(i = s_row; i <= e_row; i++) {
		SLsmg_gotorc(i, s_col);
		for(j = s_col; j < e_col; j++)
			SLsmg_write_char(' ');
		
		//SLsmg_erase_eol();
	}
	*/
}

void vtScroll(VTScreenView *view, int dest_row, int src_row, int num_line) {
	/*
	int i,j;
	SLsmg_gotorc(0,0);
	SLsmg_printf("dest: %d src: %d num_line: %d\n", dest_row, src_row, num_line);
	*/
}

int main() {
	VTCore *vtA, *vtB, *vtCurrent;
	TerminalIO *tioA, *tioB;
	VTScreenView *viewA, *viewB;

	WINDOW *main;

	char *argv[] = {"/bin/bash", 0};
	int done;
	unsigned int input;
	char ch[2];
	int fdA, fdB, fdCurrent;
	fd_set fileDescriptors;
	struct timeval timeout;

	// Initialize Terminal
	/*
	SLtt_get_terminfo ();
	SLang_init_tty (-1, 0, 0);
	SLsmg_init_smg ();
	SLsmg_refresh();
	*/
	done = 0;
	main = initscr();
	keypad(main, true);
	noecho();
	refresh();

	left = newwin(VTROWS,VTCOLS, 0, 0);
	right = newwin(VTROWS, VTCOLS, 0, VTCOLS + 10);
	current = left;

	// iTerm Setup
	// new iTerm TerminalIO object
	tioA = TtyTerminalIO_new(VTCOLS, VTROWS, "/bin/bash", argv); // width, height, program, argv
	tioB = TtyTerminalIO_new(VTCOLS, VTROWS, "/bin/bash", argv); // width, height, program, argv
	if(tioA == NULL) {
		printf("Error initializing TerminalIO\n");
		return 1;
	}

	// new Virtual Terminal Screen View object
	viewA = malloc (sizeof (VTScreenView));
	viewB = malloc (sizeof (VTScreenView));
	if (viewA == NULL) {
		perror ("VTScreenView_new");
		return 1;
	}

	VTScreenView_init(viewA);
	viewA->object = left;
	viewA->draw_text = vtDrawText;
	viewA->update_cursor_position = vtMoveCursor;
	viewA->clear_rect = vtClearRect;
	viewA->scroll_view = vtScroll;

	VTScreenView_init(viewB);
	viewB->object = right;
	viewB->draw_text = vtDrawText;
	viewB->update_cursor_position = vtMoveCursor;
	viewB->clear_rect = vtClearRect;
	viewB->scroll_view = vtScroll;

	// Start the iTerm core
	//vt = VTCore_new(tio, SLtt_Screen_Cols, SLtt_Screen_Rows, 10); // terminalIO, cols, rows, history
	vtA = VTCore_new(tioA, VTCOLS, VTROWS, 10); // terminalIO, cols, rows, history
	vtB = VTCore_new(tioB, VTCOLS, VTROWS, 10); // terminalIO, cols, rows, history
	vtCurrent = vtA;
	if(vtA == NULL) {
		printf("Error initializing VTCore\n");
		return 1;
	}

	// Tell the core with view to use ... can we have multiple?
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

					write(fdCurrent, &ch, 1);
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
