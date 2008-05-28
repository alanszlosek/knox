knox: main.c tesi.o vt.o divideRectangle.o
	gcc -o knox main.c tesi.o vt.o divideRectangle.o -lm -lslang -lpcre

tesi.o: ../../tesi/tesi.c ../../tesi/tesi.h
	gcc -c ../../tesi/tesi.c

vt.o: vt.c vt.h
	gcc -c vt.c

divideRectangle.o: ../../divideRectangle/divideRectangle.c ../../divideRectangle/divideRectangle.h
	gcc -c ../../divideRectangle/divideRectangle.c


debug: main.c tesi.o vt.o divideRectangle.o
	gcc -g -o knox main.c tesi.o vt.o divideRectangle.o -lm -lslang -lpcre

tesi.o: ../../tesi/tesi.c ../../tesi/tesi.h
	gcc -g -c ../../tesi/tesi.c

vt.o: vt.c vt.h
	gcc -g -c vt.c

divideRectangle.o: ../../divideRectangle/divideRectangle.c ../../divideRectangle/divideRectangle.h
	gcc -g -c ../../divideRectangle/divideRectangle.c
