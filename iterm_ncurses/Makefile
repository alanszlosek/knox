knox: main.c vt.o list.o divideRectangle.o
	gcc -g -o knox main.c vt.o list.o divideRectangle.o -lncurses -lm -literm

vt.o: vt.c vt.h
	gcc -g -c vt.c

list.o: ../../linkedList/list.c ../../linkedList/list.h
	gcc -g -c ../../linkedList/list.c

divideRectangle.o: ../../divideRectangle/divideRectangle.c ../../divideRectangle/divideRectangle.h
	gcc -g -c ../../divideRectangle/divideRectangle.c
