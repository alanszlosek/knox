#include <sys/select.h>
#include <iterm/core.h>
#include <iterm/unix/ttyio.h>

#include <slang/slang.h>

void vtDrawText(VTScreenView *view, int col, int row, char *mbstring, int length, int width) {
	SLsmg_gotorc(row, col);
	SLsmg_write_nchars(mbstring, length);
	SLsmg_gotorc(row, col + length - 1);
	//SLsmg_refresh();

	/*
	mbstring[length] = 0;
	printf("text. col: %d row: %d length: %d width: %d string: %s\n", col, row, length, width, mbstring);
	*/
}

void vtMoveCursor(VTScreenView *view, int col, int row) {
	SLsmg_gotorc(row, col);
	//SLsmg_refresh();
}

void vtClearRect(VTScreenView *view, int s_col, int s_row, int e_col, int e_row) {
	int i,j;
	for(i = s_row; i <= e_row; i++) {
		SLsmg_gotorc(i, s_col);
		for(j = s_col; j < e_col; j++)
			SLsmg_write_char(' ');
		
		//SLsmg_erase_eol();
	}
	//SLsmg_refresh();
}

void vtScroll(VTScreenView *view, int dest_row, int src_row, int num_line) {
	int i,j;
	SLsmg_gotorc(0,0);
	SLsmg_printf("dest: %d src: %d num_line: %d\n", dest_row, src_row, num_line);
	

	//SLsmg_refresh();
}

int main() {
	TerminalIO *tio;
	VTCore *vt;
	VTScreenView *view;

	char *argv[] = {"/bin/bash", 0};
	//char input[65];
	unsigned int input;
	char ch[2];
	int fd;
	fd_set fileDescriptors;
	struct timeval timeout;

	// Initialize Terminal
	SLtt_get_terminfo ();
	SLang_init_tty (-1, 0, 0);
	SLsmg_init_smg ();
	SLsmg_refresh();

	// iTerm Setup
	// new iTerm TerminalIO object
	tio = TtyTerminalIO_new(SLtt_Screen_Cols, SLtt_Screen_Rows, "/bin/bash", argv); // width, height, program, argv
	if(tio == NULL) {
		printf("Error initializing TerminalIO\n");
		return 1;
	}

	// new Virtual Terminal Screen View object
	view = malloc (sizeof (VTScreenView));
	if (view == NULL) {
		perror ("VTScreenView_new");
		return 1;
	}

	VTScreenView_init (view);
	view->draw_text = vtDrawText;
	view->update_cursor_position = vtMoveCursor;
	view->clear_rect = vtClearRect;
	view->scroll_view = vtScroll;


	// Start the iTerm core
	vt = VTCore_new(tio, SLtt_Screen_Cols, SLtt_Screen_Rows, 10); // terminalIO, cols, rows, history
	if(vt == NULL) {
		printf("Error initializing VTCore\n");
		return 1;
	}

	// Tell the core with view to use ... can we have multiple?
	VTCore_set_screen_view(vt, view);

	timeout.tv_sec = 0;
	timeout.tv_usec = 100000;
	fd = TtyTerminalIO_get_associated_fd(tio);

	while(1) {
		FD_ZERO(&fileDescriptors);
		FD_SET(0, &fileDescriptors);
		FD_SET( TtyTerminalIO_get_associated_fd(tio), &fileDescriptors);
		timeout.tv_sec = 0;
		timeout.tv_usec = 100000;
		select(fd + 1, &fileDescriptors, NULL, NULL, &timeout); 

		if(FD_ISSET(fd, &fileDescriptors)) {
			VTCore_dispatch(vt);
			SLsmg_refresh();
		}
		if(FD_ISSET(0, &fileDescriptors)) {
			input = SLang_getkey();

			//printf("input: %c\n", (char)input);
			ch[0] = (char) input;
			ch[1] = 0;

			write(fd, &ch, 1);

			if(ch[0] == 'Q')
				break;
		}
	}
		

	free(view);
	VTCore_destroy(vt);
	TtyTerminalIO_destroy(tio);

	// Shut down s-lang IO
	SLsmg_reset_smg ();
	SLang_reset_tty ();

	return 0;
}
