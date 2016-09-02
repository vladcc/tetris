//#define LINUX / WINDOWS

#include "tetris.h"

int main(void)
{	
	/* game start and main loop */
	
	shape current_shape;			// our shape
	shape_data * shapes[NUMBER_OF_SHAPES];	// used for randomizing
	int sleep;				
	int init_lvl_speed;		
	int level_speed;		
	int time_to_move;	// controls when automatic movement happens
	int score_backup;
	int seed;
	bool is_time_to_render;
	
	#ifdef LINUX
		disable_echo();
	#endif
	
	// goto here when restarting the game (I know, I know)
	restart:
	
	sleep = SLEEP_MS;					// check for input 60 times a second
	init_lvl_speed = LEVEL_1_SPEED;		// wait 0.5 seconds to move down, also the number of levels
	seed = time(NULL);
	
	shapes[0] = &square;
	shapes[1] = &candy_cane_left;
	shapes[2] = &candy_cane_right;
	shapes[3] = &z_shape_left;
	shapes[4] = &z_shape_right;
	shapes[5] = &traffic_sign;
	shapes[6] = &stick;

	score = 0;
	score_backup = score;
	is_score_new = true;	// does score need re-rendering
	
	level = 1;				
	is_level_new = true;	// does level need re-rendering
	
	time_to_move = 0;
	level_speed = init_lvl_speed;
	is_game_over = false;
	did_shape_hit_bottom = false;
	is_time_to_render = false;
	
	// screen coordinates for rendering
	render_row = 4;
	render_col = 40 - (FIELD_COLS) / 2;

	srand(seed);
	clear_screen();
	show_cursor(false);
	show_controls(render_row, 5);
	
	
	get_field();	// save backup of the active field
	initialize_shape(&current_shape, shapes[shape_rand()]);
	
	/* render the first frame on the screen
	 * from here on rendering happens on need only basis */
	initialize_field();	

	next_shape = shapes[shape_rand()];
	is_next_updated = true;	// does the next shape screen needs re-rendering
	
	/* check if something that needs displaying
	 * has changed and print it */
	render();
	
	// main loop
	while (true)
	{
		sleep_ms(sleep);
		++time_to_move;
		
		if (kbhit())	// check if something was pressed
		{
			switch (getch())	// get what was pressed
			{
				case MOVE_LEFT:
					move(&current_shape, left);
					is_time_to_render = true;
					break;
				case MOVE_RIGHT:
					move(&current_shape, right);
					is_time_to_render = true;
					break;
				case ROTATE_LEFT:
					rotate_90d(&current_shape, LEFT);
					is_time_to_render = true;
					break;
				case ROTATE_RIGHT:
					rotate_90d(&current_shape, RIGHT);
					is_time_to_render = true;
					break;
				case FAST_DOWN: // DOWN_QUICKER times faster than the level speed
					level_speed /= DOWN_QUICKER;
					break;
				case QUIT_GAME:
					if (ask_player("Quit? (y/n)"))
					{
						move_cursor_xy(0, 0);
						show_cursor(true);
						exit_game();	
					}
					break;
				case PAUSE_GAME:
					pause_game();
					break;
				case RESET_GAME:
					if (ask_player("Reset? (y/n)"))
					{
						reset_field();
						goto restart;	
					}
				default:
					break;
			}
		}
		
		if (level_speed <= time_to_move)
		{
			// automatic movement
			move(&current_shape, down);
			is_time_to_render = true;
			time_to_move = 0;
		}
		
		if (did_shape_hit_bottom)
		{
			// check for full rows
			scan_field();
			
			if ((score_backup + LEVEL_UP <= score) && (level < MAX_LEVEL))
			{
				++level;
				is_level_new = true;
				score_backup = score; // store for comparison
			}
			
			initialize_shape(&current_shape, next_shape);
			next_shape = shapes[shape_rand()];
			is_next_updated = true;
			
			/* initialize_shape() checks for valid position
			 * if the position is not - the game is over */
			if (is_game_over)
				end_game();
			
			level_speed = init_lvl_speed - (SPEED_RESOLUTION * level);
			did_shape_hit_bottom = false;
			is_time_to_render = true;
		}
		
		if (is_time_to_render)
		{
			render();
			is_time_to_render = false;
		}
	}
	
	return 0;
}

void sleep_ms(int ms)
{
#ifdef WINDOWS
	Sleep(ms);
#else
	usleep(ms * 1000);
#endif
}

void get_field(void)
{
	/* backups the main field in field_buffer before 
	 * updating field to a new state */
	copy_matrix(FIELD_ROWS, FIELD_COLS, field, field_buffer);
}

void reset_field(void)
{
	/* zero out both fields */
	int row, col;
	
	for (row = 0; row < FIELD_ROWS; ++row)
		for (col = 0; col < FIELD_COLS; ++col)
			field[row][col] = 0;
			
	copy_matrix(FIELD_ROWS, FIELD_COLS, field, field_buffer);
}

bool ask_player(char * msg)
{
	/* asks the player a yes or no question
	 * used for the quit and reset messages */
	char ch;
	int msg_len = strlen(msg);
	int i;
	
	// go above field
	move_cursor_xy(render_row - 1, render_col + FIELD_COLS / 2 - msg_len / 2);
	// print the message
	puts(msg);
	
	// get input without echo
	while ((ch = getch()) != 'y' && ch != 'n')
		continue;
	
	// delete message
	for (i = 0; i < msg_len; ++i)
	{
		move_cursor_xy(render_row - 1, render_col + FIELD_COLS / 2 - msg_len / 2 + i);
		putchar(' ');
	}	
	
	if ('y' == ch)
		return true;
		
	return false;
}

void pause_game(void)
{
	/* pauses the game */
	char msg[] = "Paused";
	int msg_len = strlen(msg);
	int i;
	
	// move above field
	move_cursor_xy(render_row - 1, render_col + FIELD_COLS / 2 - msg_len / 2);
	// print pause message
	puts(msg);
	// get any key without echo
	getch();
	
	// remote pause message
	for (i = 0; i < msg_len; ++i )
	{
		move_cursor_xy(render_row - 1, render_col + FIELD_COLS / 2 - msg_len / 2 + i);
		putchar(' ');
	}
}

void show_controls(int row, int col)
{
	/* prints the controls at row, col */
	move_cursor_xy(row, col);
	printf("%c - move left\n", MOVE_LEFT);
	move_cursor_xy(++row, col);
	printf("%c - move right\n", MOVE_RIGHT);
	move_cursor_xy(++row, col);
	printf("%c - move down faster\n", FAST_DOWN);
	move_cursor_xy(row += 2, col);
	printf("%c - rotate left\n", ROTATE_LEFT);
	move_cursor_xy(++row, col);
	printf("%c - rotate right\n", ROTATE_RIGHT);
	move_cursor_xy(row += 2, col);
	printf("%c - pause\n", PAUSE_GAME);
	move_cursor_xy(++row, col);
	printf("%c - reset\n", RESET_GAME);
	move_cursor_xy(++row, col);
	printf("%c - quit\n", QUIT_GAME);
}

void show_score(int score, int row, int col)
{
	/* prints the score at row, col */
	move_cursor_xy(row, col);
	printf("Score: %-5d", score);	
}

void show_level(int level, int row, int col)
{
	/* prints the level at row, col */
	move_cursor_xy(row, col);
	printf("Level: %-2d", level);	
}

void show_next(shape_data * sd, int row, int col)
{
	/* prints the next shape at row, col */
	int m_r, m_c;
	move_cursor_xy(row, col);
	
	puts("Next:");
	for (m_r = 0; m_r < MATRIX_SIZE - 1; ++m_r)
	{
		move_cursor_xy(++row, col);
		for (m_c = 0; m_c < MATRIX_SIZE; ++m_c)
			if (0 == sd->grid[m_r][m_c])
				putchar(' ');
			else
				putchar(sd->code);
	}	
}

void exit_game(void)
{
#ifdef WINDOWS
	exit(0);
#else
	enable_echo();
	exit(0);
#endif
}

void end_game(void)
{
	/* game over */
	char msg[] = "Game Over";
	int msg_len = strlen(msg);
	
	move_cursor_xy(render_row - 1, render_col + FIELD_COLS / 2 - msg_len / 2 );
	puts(msg);

	show_cursor(true);
	// reset the cursor for the console
	move_cursor_xy(0, 0);
	exit_game();
}

int shape_rand(void)
{
	/* return a pseudo random number for indexing
	 * in the shape_data pointer array */
	return rand() % NUMBER_OF_SHAPES;
}

void initialize_field(void)
{
	/* create the field and print the first frame */
	int row, col, f_row;
	int row_rend, col_rend;
	
	row_rend = render_row;
	col_rend = render_col - 1; // one less because of border
	
	move_cursor_xy(row_rend, col_rend);
	
	// draw borders
	for (row = 0; row < FRAME_BUFF_ROWS; ++row)
	{	
		putchar(BORDER);
		
		move_cursor_xy(row_rend, col_rend + FIELD_COLS + 1);
	
		putchar(BORDER);
		move_cursor_xy(++row_rend, col_rend);
	}
	
	for (col = 0; col < FRAME_BUFF_COLS + 2; ++col)
		putchar(BORDER);
	
	// draw first frame
	row_rend = render_row;
	col_rend = render_col;
	
	move_cursor_xy(row_rend, col_rend);
	for (row = 0, f_row = row + 1; row < FRAME_BUFF_ROWS; ++row, ++f_row)
	{	
		for (col = 0; col < FRAME_BUFF_COLS; ++col)
		{
			if (0 == field[f_row][col])
				frame_buffer[row][col] = FIELD;
			else
				frame_buffer[row][col] = (char)field[f_row][col];
			
			putchar(frame_buffer[row][col]);
		}
		move_cursor_xy(++row_rend, col_rend);
	}
}

void render(void)
{
	/* checks for difference between field and field_buffer and draws only what's new
	 * also checks and handles if score, level, and next need re-drawing */
	int row, f_row, col;
	int row_rend, col_rend;
	
	row_rend = render_row;
	col_rend = render_col;
	
	move_cursor_xy(row_rend, col_rend);
	
	for (row = 0, f_row = row + 1; row < FRAME_BUFF_ROWS; ++row, ++f_row)
	{	
		for (col = 0; col < FRAME_BUFF_COLS; ++col)
		{
			// see what's new
			if (field[f_row][col] != field_buffer[f_row][col])
			{
				// go to new content cell
				move_cursor_xy(row_rend + row, col_rend + col);
				// place in the frame buffer
				frame_buffer[row][col] = (char)field[f_row][col];
				
				// print
				if (0 != frame_buffer[row][col])
					putchar(frame_buffer[row][col]);
				else
					putchar(FIELD);
			}
		}
	}
	
	// check and draw HUD
	if (is_next_updated)
	{
		show_next(next_shape, render_row, render_col + FIELD_COLS + 2);
		is_next_updated = false;
	}
		
	if (is_score_new)
	{
		show_score(score, render_row + MATRIX_SIZE, render_col + FIELD_COLS + 2);
		is_score_new = false;
	}
	
	if (is_level_new)
	{
		show_level(level, render_row + MATRIX_SIZE + 2, render_col + FIELD_COLS + 2);
		is_level_new = false;
	}
	
	// backup the new state of field	
	get_field();
}

void move_cursor_xy(int row, int col)
{
	/* move the console cursor to row, col */
#ifdef WINDOWS
	HANDLE h;
	COORD c;
	
	h = GetStdHandle(STD_OUTPUT_HANDLE);
	c.X = col;
	c.Y = row;
	SetConsoleCursorPosition(h, c);
#else
	printf("\e[%d;%dH", row, col);
#endif
}

void show_cursor(bool show)
{
	/* show/hide the console cursor */
#ifdef WINDOWS
	HANDLE h;
	CONSOLE_CURSOR_INFO inf;
	
	h = GetStdHandle(STD_OUTPUT_HANDLE);
	inf.dwSize = 25; // default size
	inf.bVisible = show;
	SetConsoleCursorInfo(h, &inf);
#else
	// ANSI escape
	if (show)
		printf("\e[?25h");	// show
	else
		printf("\e[?25l");	// hide
#endif
}

void clear_screen(void)
{
#ifdef WINDOWS
	system("cls");
#else
	// ANSI escape
	printf("\e[2J");
#endif
}

void scan_field(void)
{
	/* scans the field and removes full rows */
	int row, col;
	bool is_row_full;
	
	for (row = FIELD_ROWS - 1; row >= 0; --row)
	{
		is_row_full = true;
	
		for (col = 0; col < FIELD_COLS; ++col)
		{
			 if (0 == field[row][col])
			 {
				 is_row_full = false;
				 break;
			 }
		}
			
		if (is_row_full)
		{
			++score;
			is_score_new = true;
			
			int r1, r2, c;
			// move upper row to current row
			for (r1 = row, r2 = r1 - 1; r1 > 0; --r1, --r2)
			{
				for (c = 0; c < FIELD_COLS; ++c)
					field[r1][c] = field[r2][c];
			}
			
			// start from the bottom row again
			row = FIELD_ROWS; // one more because for loop will decrement
		}
	}
}

void copy_matrix(int rows, int cols, int m_s[rows][cols], int m_d[rows][cols])
{
	/* copies a source matrix to an equal size destination */
	int row, col;
	
	for (row = 0; row < rows; ++row)
	{
		for (col = 0; col < cols; ++col)
			m_d[row][col] = m_s[row][col];
	}
}

bool is_position_legal(shape * s, int (* coordinates)[2])
{
	/* checks if we can place the shape at coordinates */
	if ( (is_position_collision(s, coordinates)) || is_position_out_of_bounds(s, coordinates) )
		return false;
	
	return true;
}

bool is_position_collision(shape * s, int (* coordinates)[2])
{
	/* see if we are hitting something */
	int i, row, col;
	bool result = false;
	
	for (i = 0; i < SHAPE_ELEMENTS; ++i)
	{
		row = coordinates[i][0];
		col = coordinates[i][1];
		
		if (0 != field[row][col])
		{
			result = true;
			break;
		}
	}
	
	return result;
}

bool is_position_out_of_bounds(shape * s, int (* coordinates)[2])
{
	/* see if we are getting out of the field bounds */
	int i, row, col;
	bool result = false;
	
	for (i = 0; i < SHAPE_ELEMENTS; ++i)
	{
		row = coordinates[i][0];
		col = coordinates[i][1];
		
		if ( (row < 0 || row > FIELD_ROWS - 1) || (col < 0 || col > FIELD_COLS - 1) )
		{
			result = true;
			break;
		}	
	}
 
	return result;
}

void move(shape * s, const int * direction)
{
	/* move shape in direction */
	int (* coords_backup)[2];		// backup the coordinates buffer address
	int curr_pos[2];				// field coordinates for the pivot point of the shape
	
	// backup original coodinates buffer
	coords_backup = s->field_coords;
	
	// remove current shape for now
	clear_current_coordinates(s);
	
	// switch to new buffer
	s->field_coords = buffer_shape_field_coords;

	// copy coordiantes
	copy_matrix(SHAPE_ELEMENTS, 2, coords_backup, s->field_coords);
	
	// add direction to pivot point
	curr_pos[0] = s->field_coords[0][0] += direction[0];
	curr_pos[1] = s->field_coords[0][1] += direction[1];
	
	// get new field coordinates
	calculate_field_coordinates(s, curr_pos[0], curr_pos[1]);
	
	if (!(is_position_legal(s, s->field_coords)))
	{
		// get back original coordinates
		s->field_coords = coords_backup;
		put_shape_back(s);
		
			// check if we were moving down
			if (1 == direction[0])
				did_shape_hit_bottom = true;
			
		return;
	}
	
	// commint
	copy_matrix(SHAPE_ELEMENTS, 2, s->field_coords, coords_backup);
	// get original coordinates pointer back
	s->field_coords = coords_backup;
	// put shape at new position
	place_shape_on_field(s);
}

void initialize_shape(shape * s, shape_data * s_data)
{
	/* fill the shape data structure with needed information */
	
	// copy the shape's gird
	copy_matrix(MATRIX_SIZE, MATRIX_SIZE, s_data->grid, buffer_matrix);
	
	// point to the copy
	s->grid = buffer_matrix;
	// point to the field coordinates buffer
	s->field_coords = shape_field_coordinates;
	// get the pivot point of rotation
	s->pivot = s_data->pivot;
	// get the thing that makes out the shape
	s->code = s_data->code;
	// find where the shape elements are in it's own grid
	get_shape_from_grid(s);
	// replace the 1s in the grid with it's original elements
	code_shape(s);
	// put it on the field
	create_new_shape(s);
}

void create_new_shape(shape * s)
{
	/* places the shape on it's starting position
	 * or ends the game */
	int row, col;
	
	row = 2; // always put pivot on second row
	
	// this makes the stick and the square appear in the middle
	col = (FIELD_COLS / 2) - (s->length / 2) + s->pivot[0] - s->pivot[1];
	
	// find the actual coordinates in the field matrix
	calculate_field_coordinates(s, row, col);
	
	if (!(is_position_legal(s, s->field_coords)))
		is_game_over = true;
	
	// if ok put it out there
	place_shape_on_field(s);
}

void get_shape_from_grid(shape * s)
{
	/* gets the coordinates for every shape element in it's own grid 
	 * also the length of the longest part of the shape */
	int i, row, col, coord_row, len;
	
	// initialize length to 0
	s->length = len = 0;
	
	for (row = 0, coord_row = 0; row < MATRIX_SIZE; ++row)
	{
		for (col = 0; col < MATRIX_SIZE; ++col)
		{
			if (0 != s->grid[row][col])
			{
				/* write down the coordinates of each element of 
				 * the shape in the matrix and count the length */
				s->coordinates[coord_row][0] = row;
				s->coordinates[coord_row][1] = col;
				++coord_row;
				++len;
			}
		}
		
		/* if the last row had more elements than the previous
		 * save that and reset the length counter */
		if (s->length < len)
			s->length = len;
			
		len = 0;
	}
	
	/* place the pivot point as the first coordinate 
	 * this is very important */
	for (i = 0; i < SHAPE_ELEMENTS; ++i)
	{
		if (s->coordinates[i][0] == s->pivot[0] &&
			s->coordinates[i][1] == s->pivot[1])
			{
				int temp;
				temp = s->coordinates[0][0];
				s->coordinates[0][0] = s->pivot[0];
				s->coordinates[i][0] = temp;

				temp = s->coordinates[0][1];
				s->coordinates[0][1] = s->pivot[1];
				s->coordinates[i][1] = temp;
			}
	}
}

void code_shape(shape * s)
{
	/* replaces the 1s in the grid with
	 * the actual shape representation */
	int i, row, col;
	
	for (i = 0; i < SHAPE_ELEMENTS; ++i)
	{
		row = s->coordinates[i][0];
		col = s->coordinates[i][1];
		
		s->grid[row][col] = s->code;
	}
}

void clear_current_coordinates(shape * s)
{
	/* removes the shape from the field */
	int i, row, col;
	
	for (i = 0; i < SHAPE_ELEMENTS; ++i)
	{
		row = s->field_coords[i][0];
		col = s->field_coords[i][1];
		field[row][col] = 0;
	}
}

void put_shape_back(shape * s)
{
	/* places the shape back on the field */
	int i, row, col;
	
	for (i = 0; i < SHAPE_ELEMENTS; ++i)
	{
		row = s->field_coords[i][0];
		col = s->field_coords[i][1];
		field[row][col] = s->code;
	}
}

void calculate_field_coordinates(shape * s, int row, int col)
{
	/* finds out the actual field coordinates for every shape element */
	int i; 
	int start_row, start_col; 
	int field_row, field_col;
	int shape_row, shape_col;
	int zero_position[2];
	
	// used to divorce the shape from it's own coordinates
	zero_position[0] = s->coordinates[0][0];
	zero_position[1] = s->coordinates[0][1];
	
	// used to center (somewhat) when intializing
	start_row = row;
	start_col = col;
	
	for (i = 0; i < SHAPE_ELEMENTS; ++i)
	{
		shape_row = s->coordinates[i][0];
		shape_col = s->coordinates[i][1];
		
		field_row = start_row + shape_row - zero_position[0];
		field_col = start_col + shape_col - zero_position[1];
		
		// write down the field coordinates
		s->field_coords[i][0] = field_row;
		s->field_coords[i][1] = field_col;
	}
}

void place_shape_on_field(shape * s)
{
	/* place the shape in the field matrix */
	int i, field_row, field_col, shape_row, shape_col;
	
	for (i = 0; i < SHAPE_ELEMENTS; ++i)
	{
		shape_row = s->coordinates[i][0];
		shape_col = s->coordinates[i][1];
		field_row = s->field_coords[i][0];
		field_col = s->field_coords[i][1];
		
		/* extract the coordinates from the grid to their
		 * proper place on the field */
		field[field_row][field_col] = s->grid[shape_row][shape_col];	
	}
}

void rotate_90d(shape * s, int direction)
{
	/* rotates the shape's grid at 90 degrees left or right 
	 * around it's pivot point */
	 
	/* 0, 0 pivot prevents rotation 
	 * we don't need to rotate the square */
	if (0 == s->pivot[0] && 0 == s->pivot[1])
		return;
	
	int (* backup)[MATRIX_SIZE];	// backup the matrix buffer address
	int (* coords_backup)[2];		// backup the coordinates buffer address
	int curr_pos[2];				// field coordinates for the pivot point of the shape
	int shift_value[2] = {0, 0};	// shift values for the rotated shape
	int row, col;
	
	// copy the original grid
	copy_matrix(MATRIX_SIZE, MATRIX_SIZE, s->grid, buffer_matrix_copy);
	
	// backup the original grid address
	backup = s->grid;
	
	// backup the original coords buffer address
	coords_backup = s->field_coords;
	
	/* save current field position for the pivot point 
	 * it's all we need */
	curr_pos[0] = s->field_coords[0][0];
	curr_pos[1] = s->field_coords[0][1];
	
	// point to a new matrix buffer
	s->grid = buffer_matrix_copy;
	// mark the pivot point
	s->grid[s->pivot[0]][s->pivot[1]] = PIVOT_POINT;
	
	// transpose
	for (row = 0; row < MATRIX_SIZE; ++row)
	{
		for (col = 0; col < MATRIX_SIZE; ++col)
			m_temp[col][row] = s->grid[row][col];
	}
	
	if (LEFT == direction)
	{
		// reverse cols - rotates left
		int row_temp;
		for (col = 0; col < MATRIX_SIZE; ++col)
		{
			for (row = 0, row_temp = MATRIX_SIZE - 1; row < MATRIX_SIZE; ++row, --row_temp)
			{
				s->grid[row][col] = m_temp[row_temp][col];
				m_temp[row_temp][col] = 0; // zero for later
			}
		}
	}
	else
	{
		// reverse rows - rotates right
		int col_temp;
		for (row = 0; row < MATRIX_SIZE; ++row)
		{
			for (col = 0, col_temp = MATRIX_SIZE - 1; col < MATRIX_SIZE; ++col, --col_temp)
			{
				s->grid[row][col] = m_temp[row][col_temp];
				m_temp[row][col_temp] = 0; // zero for later
			}
		}
	}
	
	// find pivot point
	for (row = 0; row < MATRIX_SIZE; ++row)
	{
		for (col = 0; col < MATRIX_SIZE; ++col)
			if (PIVOT_POINT == s->grid[row][col])
			{
				shift_value[0] = s->pivot[0] - row;
				shift_value[1] = s->pivot[1] - col;
				goto shift_shape; // exit the nested loop
			}
	}
	
	shift_shape: // shift the shape so we get rotation around the pivot
	for (row = 0; row < MATRIX_SIZE; ++row)
	{
		for (col = 0; col < MATRIX_SIZE; ++col)
			if (0 != s->grid[row][col])
			{
				m_temp[row + shift_value[0]][col + shift_value[1]] = s->grid[row][col];
				s->grid[row][col] = 0; // zero out for later
			}
	}
	

	// fill with the code for the shape
	for (row = 0; row < MATRIX_SIZE; ++row)
	{
		for (col = 0; col < MATRIX_SIZE; ++col)
			if (0 != m_temp[row][col])
				s->grid[row][col] = s->code;
	}

	
	// get new shape coordinates from it's grid
	get_shape_from_grid(s);
	
	// remove shape from field for now
	clear_current_coordinates(s);
	
	// point to new coordinates buffer
	s->field_coords = buffer_shape_field_coords;
	
	// get new field coordinates
	calculate_field_coordinates(s, curr_pos[0], curr_pos[1]);

	// check if we can actually place the shape there
	if (is_position_legal(s, s->field_coords))
	{
		// commint
		copy_matrix(MATRIX_SIZE, MATRIX_SIZE, s->grid, backup);
		copy_matrix(SHAPE_ELEMENTS, 2, s->field_coords, coords_backup);
		
		// switch back to original buffers
		s->field_coords = coords_backup;
		s->grid = backup;
		place_shape_on_field(s);
	}
	else
	{
		// switch back to original buffers
		s->field_coords = coords_backup;
		s->grid = backup;
		
		// get old shape coordinates
		get_shape_from_grid(s);
		// put the shape back in it's place
		put_shape_back(s);
	}
}

void print_m(int row, int col, int m[row][col])
{
	/* prints a row, col matrix for testing and
	 * debugging purposes */
	int i, j;
	
	for (i = 0; i < row; ++i)
	{
		for (j = 0; j < col; ++j)
			printf("%3d", m[i][j]);
		
		putchar('\n');
	}
	
	putchar('\n');
}
