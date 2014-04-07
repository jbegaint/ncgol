#ifndef _NCGOL_H_
#define _NCGOL_H_

#define KEY_ESCAPE 27
#define SLEEP_DFLT 300

typedef struct cell_t {
	char isalive;
	int age; /* useless, for now */
} cell_t;

typedef struct grid_t {
	cell_t **cells;
	int col;
	int row;
} grid_t;

#define	IS_CELL_ALIVE(grid, x, y) (((grid->cells)[x][y]).isalive)
#define USED(x) (void)(x)

#endif
