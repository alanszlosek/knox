iterm_ncurses: main.c config.h iterm_ncurses_vt.o divideRectangle.o
	gcc -o knox main.c iterm_ncurses_vt.o divideRectangle.o -lm -lncurses -literm

tesi_ncurses: main.c config.h tesi.o tesi_ncurses_vt.o divideRectangle.o
	gcc -o knox main.c tesi.o tesi_ncurses_vt.o divideRectangle.o -lm -lncurses

tesi_slang: main.c config.h tesi.o tesi_slang_vt.o divideRectangle.o
	gcc -o knox main.c tesi.o tesi_slang_vt.o divideRectangle.o -lm -lncurses

tesi.o: third_party/tesi/tesi.c third_party/tesi/tesi.h
	gcc -c third_party/tesi/tesi.c

iterm_ncurses_vt.o: iterm_ncurses/vt.c iterm_ncurses/vt.h
	gcc -c iterm_ncurses/vt.c -o iterm_ncurses_vt.o

tesi_ncurses_vt.o: tesi_ncurses/vt.c tesi_ncurses/vt.h
	gcc -c tesi_ncurses/vt.c -o tesi_ncurses_vt.o

tesi_slang_vt.o: tesi_slang/vt.c tesi_slang/vt.h
	gcc -c tesi_slang/vt.c -o tesi_slang_vt.o

divideRectangle.o: ../divideRectangle/divideRectangle.c ../divideRectangle/divideRectangle.h
	gcc -c ../divideRectangle/divideRectangle.c -lm
