#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <ncurses.h>
#include <signal.h>

#include "ncgol.h"

void handler_sigwinch(int sig)
{
	USED(sig);
	endwin();
	refresh();
	clear();
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
		((grid->cells)[x][y]).age = 0;
	} else if (IS_CELL_ALIVE(buf_grid, x, y) && (c == 3 || c == 4)) {
		/* survival */
		((grid->cells)[x][y]).age += 1;
	} else {
		/* under-population or overcrowding */
		((grid->cells)[x][y]).age = 0;
		((grid->cells)[x][y]).isalive = 0;
	}
}

grid_t *grid_init(int col, int row)
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

void grid_free(grid_t *grid)
{
	/* free the mallocs... */
	free(*(grid->cells));
	free(grid->cells);
	free(grid);
}

void grid_randomize(grid_t *grid)
{
	int i, j;


	for (j = 0; j < grid->col; ++j) {
		for (i = 0; i < grid->row; ++i) {
			((grid->cells)[i][j]).isalive = (rand() % 2 == 0) ? 0 : 1;
			((grid->cells)[i][j]).age = 0;
		}
	}
}

void grid_copy(grid_t *dest, grid_t *src)
{
	memcpy(*(dest->cells), *(src->cells), src->row * src->col * 
			sizeof(cell_t));
}

void disp_help(int ymax, int xmax)
{
	WINDOW *help_win;

	int w_help_y = 12;
	int w_help_x = 40;

	if ((ymax < w_help_y) || (xmax < w_help_x)) {
		print_center("term window too small", A_BOLD | A_BLINK | A_UNDERLINE);
		getch();
		endwin();
		exit(EXIT_FAILURE);
	}

	help_win = newwin(w_help_y, w_help_x, (ymax - w_help_y )/ 2, (xmax -
				w_help_x) / 2); 
	box(help_win, 0, 0);

	/* print title */
	attron(A_BOLD);
	mvwprintw(help_win, 1, w_help_x / 2 - 2, "Help");
	attroff(A_BOLD);

	/* print text */
	mvwprintw(help_win, 2, 2, "q: quit");
	mvwprintw(help_win, 3, 2, "escape: quit");
	mvwprintw(help_win, 4, 2, "r: restart");
	mvwprintw(help_win, 5, 2, "R: restart and reset speed");
	mvwprintw(help_win, 6, 2, "h: show this help");
	mvwprintw(help_win, 7, 2, "+: increase speed");
	mvwprintw(help_win, 8, 2, "-: decrease speed");
	mvwprintw(help_win, w_help_y - 2, 3,
		  "press any key to close this window");

	wrefresh(help_win);

	/* waiting for a key to close */
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

void wait(double speed)
{
	struct timespec sleep_time;

	sleep_time.tv_sec = 0;
	sleep_time.tv_nsec = (1 / speed) * (1000 * 1000 * 100 / SLEEP_CYCLE);

	nanosleep(&sleep_time, NULL);
}

void increase_speed(double *sleep)
{
	*sleep = *sleep * 1.5;
}

void decrease_speed(double *sleep)
{
	*sleep = *sleep / 1.5;
}

int main(void)
{
	int col, row;
	int i, j;
	int ymax, xmax;
	int alives;
	int c = 0;
	char key;
	grid_t *grid, *buf_grid;
	WINDOW *w, *main_w;
	int running = 1;
	double speed = SPEED_DFLT;

	/* trap sigwinch (term window resize) */
	signal(SIGWINCH, handler_sigwinch);

	/* init curses */
	main_w = initscr();
	start_color();
	cbreak();
	noecho();
	clear();
	curs_set(0);
	getmaxyx(stdscr, ymax, xmax);

	init_pair(1, COLOR_GREEN, COLOR_BLACK);
	refresh();

	/* init windows */
	row = ymax - 2;
	col = xmax - 2;
	w = newwin(row, col, 1, 1);
	wrefresh(w);

	/* init seed */
	srand(time(NULL));

	/* init grids */
	grid = grid_init(col, row);
	buf_grid = grid_init(col, row);
	grid_randomize(grid);

	/* start message */
	print_center("Press any key to continue", A_BOLD | A_BLINK);

	wgetch(w);
	wrefresh(w);
	wclear(w);

	nodelay(w, 1);

	while ((key = wgetch(w)) != KEY_ESCAPE && key != 'q') {

		if (running && (c % SLEEP_CYCLE) == 0) {

			/* reset counter */
			c = 0;

			/* save buffer */
			grid_copy(buf_grid, grid);

			/* draw and compute grids */
			for (j = 0; j < grid->col; ++j) {
				for (i = 0; i < grid->row; ++i) {

					/* current state */
					if (IS_CELL_ALIVE(grid, i, j)) {
						if (((grid->cells)[i][j]).age > 0) {
							mvwaddch(w, i, j, 'o');
						}
						else {
							wattron(w, COLOR_PAIR(1));
							mvwaddch(w, i, j, 'o');
							wattroff(w, COLOR_PAIR(1));
						}
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

			/* print infos */
			alives = get_cells_alive(grid);
			mvwprintw(main_w, 0, xmax / 2 - 10, "Conway's Game Of Life");
			mvwprintw(main_w, ymax - 1, 1, "speed: %06.02lf %%", speed / SPEED_DFLT 
					* 100);
			mvwprintw(main_w, ymax - 1, xmax / 2 - 10, 
					"cells alive: %5d (%0.2lf%%)", alives, (double) alives / 
					(row*col));
			mvwprintw(main_w, ymax - 1, xmax - 17, "press h for help");

			wrefresh(w);
			wrefresh(main_w);
		}

		/* additionnal controls */
		switch (key) {
			case ' ':
			case 'p':
				running = !running;
				if (!running) {
					print_center(" PAUSED ", A_BOLD);
				}
				else {
					/* reset cycle counter */
					c = SLEEP_CYCLE- 1;
				}
				break;

			case 'r':
				flash();
				grid_randomize(grid);
				break;

			case 'R':
				flash();
				speed = SPEED_DFLT;
				grid_randomize(grid);
				break;

			case 'h':
				disp_help(ymax, xmax);
				break;

			case '+':
				increase_speed(&speed);
				break;
			
			case '-':
				decrease_speed(&speed);
				break;
		}

		wait(speed);
		c++;
	}

	/* cleanup */
	delwin(w);
	endwin();

	grid_free(grid);
	grid_free(buf_grid);

	exit(EXIT_SUCCESS);
}
