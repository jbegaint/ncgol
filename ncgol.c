#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <ncurses.h>

#include "ncgol.h"

void update_grid(grid_t *grid, grid_t *buf_grid, int x, int y)
{
	int i, j;
	int c = 0;

	i = ((x - 1) >= 0) ? x - 1 : 0;

	for (; i <= x + 1 && i < buf_grid->row; ++i) {
		j = ((y - 1) >= 0) ? y - 1 : 0;

		for (; j <= y + 1 && j < buf_grid->col; ++j) {
			if (IS_CELL_ALIVE(buf_grid, i, j)) {
				c++;
			}
		}
	}

	if (!IS_CELL_ALIVE(buf_grid, x, y) && c == 3) {
		/* reproduction */
		((grid->cells)[x][y]).isalive = 1;
	} 
	else if (IS_CELL_ALIVE(buf_grid, x, y) && (c == 3 || c == 4)) {
		/* survival */
		((grid->cells)[x][y]).age += 1;
		((grid->cells)[x][y]).isalive = 1;
	} 
	else {
		/* under-population or overcrowding */
		((grid->cells)[x][y]).age += 0;
		((grid->cells)[x][y]).isalive = 0;
	}
}

void print_center(char *msg, int attr)
{
	int row, col, len;
	len = strlen(msg);

	getmaxyx(stdscr, row, col);
	attron(attr);
	mvprintw(row / 2, (col - len) / 2, msg);
	attroff(attr);
	refresh();
}

grid_t *init_grid(int col, int row)
{
	int i;
	grid_t *grid;

	/* init grid */
	grid = calloc(1, sizeof(*grid));

	grid->row = row;
	grid->col = col;

	grid->cells = (cell_t **) calloc(row, sizeof(cell_t *));
	*(grid->cells) = (cell_t *) calloc(col * row, sizeof(cell_t));

	/* contiguous allocation */
	for (i = 1; i < row; ++i) {
		(grid->cells)[i] = (grid->cells)[i - 1] + col;
	}

	return grid;
}

void free_grid(grid_t *grid)
{
	/* free the mallocs... */
	free(*(grid->cells));
	free(grid->cells);
	free(grid);
}

void randomize_grid(grid_t *grid)
{
	int i, j;

	/* init seed */
	srand(time(NULL));

	for (j = 0; j < grid->col; ++j) {
		for (i = 0; i < grid->row; ++i) {
			((grid->cells)[i][j]).isalive = (rand() % 2 == 0) ? 0 : 1;
		}
	}
}

void copy_grid(grid_t *dest, grid_t *src)
{
	memcpy(*(dest->cells), *(src->cells), src->row * src->col * sizeof(cell_t));
}

void toggle_help(int ymax, int xmax)
{
	WINDOW *help_win;

	help_win = newwin(ymax / 2, xmax / 2, ymax / 4, xmax / 4);
	box(help_win, 0, 0);

	/* print title */
	attron(A_BOLD);
	mvwprintw(help_win, 1, xmax / 4 - 2, "Help");
	attroff(A_BOLD);

	/* print text */
	mvwprintw(help_win, 2, 2, "q: quit");
	mvwprintw(help_win, 3, 2, "escape: quit");
	mvwprintw(help_win, 4, 2, "r: restart");
	mvwprintw(help_win, 5, 2, "h: show this help");
	mvwprintw(help_win, ymax / 2 - 2, xmax / 8 - 2,
		  "press any key to close this window");

	wrefresh(help_win);

	/* waiting for a key */
	wgetch(help_win);
	delwin(help_win);
}

int get_cells_alive(grid_t *grid)
{
	int i, j;
	int c = 0;

	for (j = 0; j < grid->col; ++j) {
		for (i = 0; i < grid->row; ++i) {
			if (IS_CELL_ALIVE(grid, i, j)) {
				c++;
			}
		}
	}

	return c;
}

int main(void)
{
	int col, row;
	int i, j;
	int ymax, xmax;
	char key;
	grid_t *grid, *buf_grid;
	WINDOW *w, *main_w;
	int running = 1;

	struct timespec sleep_time;
	int sleep = SLEEP_DFLT;

	sleep_time.tv_sec = 0;
	/* init curses */
	main_w = initscr();
	cbreak();
	noecho();
	clear();
	curs_set(0);
	refresh();

	getmaxyx(stdscr, ymax, xmax);

	/* init windows */
	row = ymax - 2;
	col = xmax - 2;
	w = newwin(row, col, 1, 1);
	wrefresh(w);

	/* init grids */
	grid = init_grid(col, row);
	buf_grid = init_grid(col, row);

	randomize_grid(grid);

	print_center("Press any key to continue", A_BOLD | A_BLINK);
	wgetch(w);
	wrefresh(w);
	wclear(w);

	nodelay(w, 1);

	while ((key = wgetch(w)) != KEY_ESCAPE && key != 'q') {

		if (running) {
			copy_grid(buf_grid, grid);

			for (j = 0; j < col; ++j) {
				for (i = 0; i < row; ++i) {

					/* current state */
					if (IS_CELL_ALIVE(grid, i, j)) {
						mvwaddch(w, i, j, ' ' | A_REVERSE);
					}
					else {
						/* wclear() somehow flashes the screen */
						mvwaddch(w, i, j, ' ');
					}

					/* next state */
					update_grid(grid, buf_grid, i, j);
				}
			}
			box(w, 0, 0);
			wrefresh(w);

			/* print infos */
			mvwprintw(main_w, 0, xmax / 2 - 10, "Conway's Game Of Life");
			mvwprintw(main_w, ymax - 1, 1, "speed: %lf", 1 / (double) sleep);
			mvwprintw(main_w, ymax - 1, 1, "speed: %lf", 1 / (double) sleep);
			mvwprintw(main_w, ymax - 1, xmax / 2 - 10, "cells alive: %d", get_cells_alive(grid));
			mvwprintw(main_w, ymax - 1, xmax - 17, "press h for help");
			wrefresh(main_w);

			sleep_time.tv_nsec = sleep * 1000000;
			nanosleep(&sleep_time, NULL);
		}

		/* additionnal controls */
		switch (key) {
			case 'p':
				running = !running;
				if (!running) {
					print_center("PAUSED", A_BOLD);
				}
				break;

			case 'r':
				flash();
				sleep = SLEEP_DFLT;
				randomize_grid(grid);
				break;

			case 'h':
				toggle_help(ymax, xmax);
				break;

			case '+':
				sleep = sleep / 2;
				break;
			
			case '-':
				sleep = sleep * 2;
				break;

		}
	}

	delwin(w);
	endwin();

	free_grid(grid);
	free_grid(buf_grid);

	exit(EXIT_SUCCESS);
}
