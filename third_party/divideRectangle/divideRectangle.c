// divideRectangle v0.2
// - last rectangle in row expands to fill remaining space
// - last line of rectangles expands to fill remaining space

#include <stdio.h>
#include "divideRectangle.h"

int* divideRectangle_favorWidth(int areaWidth, int areaHeight, int number, int gutter) {
	int columns, rows;
	columns = (int) round( sqrt( (double)number) );
	rows = (int) ceil( (double) number / columns );
	//printf("Rows: %d Cols: %d\n", rows, columns);
	return divideRectangle(areaWidth, areaHeight, columns, rows, number, gutter);
}

int* divideRectangle_favorHeight(int areaWidth, int areaHeight, int number, int gutter) {
	int columns, rows;
	rows = (int) round( sqrt( (double)number) );
	columns = (int) ceil( (double) number / rows );
	//printf("Rows: %d Cols: %d\n", rows, columns);
	return divideRectangle(areaWidth, areaHeight, columns, rows, number, gutter);
}

int* divideRectangle(int areaWidth, int areaHeight, int rectangleColumns, int rectangleRows, int number, int gutter) {
	int rectangleWidth, rectangleHeight;
	int i, j, gutterTemp, row, x, y;
	// startCoordinates will be an array with size of 4*number
	// element 0 is X coord for first rectangle
	// element 1 is Y coord
	// element 2 is width
	// element 3 is height
	// element 4 is X coord for second rectangle and so on
	int *startCoordinates;

	//printf("areaWidth %d areaHeight %d rectCols %d rectRows %d number %d gutter %d\n", areaWidth, areaHeight, rectangleColumns, rectangleRows, number, gutter);

	//startCoordinates = malloc(sizeof(int) * 4 * number);
	startCoordinates = calloc(4 * number, sizeof(int));
	if(startCoordinates == NULL) {
		printf("Error\n");
		return NULL;
	}
	rectangleHeight = areaHeight / rectangleRows;
	x = y = gutterTemp = 0;
	row = -1;

	for(i = 0, j = 0; i < number; i++) {
		rectangleWidth = areaWidth / rectangleColumns;
		// for the first rectangle on a row...
		if(i % rectangleColumns == 0) {
			row++;
			y = row * rectangleHeight;
			// at last row and last of the odd number of rectangles
			if(row == rectangleRows - 1) {
				if(number % rectangleColumns != 0) {
					rectangleColumns = number % rectangleColumns;
					rectangleWidth = areaWidth / rectangleColumns;
					i = 0;
					number = rectangleColumns;
				}
				// fill up the remaining available height
				rectangleHeight = (areaHeight / rectangleRows) + (areaHeight % rectangleRows);
			}
		}

		// detect last on a row
		if(i % rectangleColumns == rectangleColumns - 1) {
			// fill up the remaining available width
			rectangleWidth += (areaWidth % rectangleColumns);
		}

		x = (i % rectangleColumns) * rectangleWidth;
		*(startCoordinates + j) = x;
		*(startCoordinates + j + 1) = y;
		// if not last on line
		//if(col < cols - 1)
			*(startCoordinates + j + 2) = rectangleWidth - gutterTemp; // width
		// if not on last line
		//if(row < rows - 1)
			*(startCoordinates + j + 3) = rectangleHeight - gutterTemp; // height
		j += 4;
		//printf("\n\t\t\tRect#: %d X: %d Y: %d", i, x, y);
	}
	return startCoordinates;
}
