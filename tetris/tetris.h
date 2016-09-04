/* ---------------- INCLUDES  ----------------*/
#include <stdio.h>

#ifdef WINDOWS
	#include <windows.h>
	#include <conio.h>
#else
	/* custom conio.h for compatibility with original code
	 * used for Linux */
	#include "lconio.h"
#endif

#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

/* ---------------- CONSTANTS  ----------------*/
#define MATRIX_SIZE			5				// make sure there is enough space for rotation
#define PIVOT_POINT 			22				// marks the pivot point of rotation
#define SHAPE_ELEMENTS			4				// how many elements does the shape have?
#define FIELD_ROWS 			21				// first row always empty
#define FIELD_COLS 			10				// 10 columns is classic tetris width
#define FRAME_BUFF_ROWS 		FIELD_ROWS - 1
#define FRAME_BUFF_COLS 		FIELD_COLS
#define LEFT				0				// LEFT and RIGHT are used to distinguish rotation
#define RIGHT				1

// what the shapes and the field looks like
#define SQUARE				'0'
#define CANDY_CANE			'0'
#define Z_SHAPE				'0'
#define TRAFFIC_SIGN			'0'
#define STICK				'0'
#define NUMBER_OF_SHAPES		7
#define FIELD				' '
#define BORDER				'#'

// controls
#define MOVE_LEFT			'a'
#define MOVE_RIGHT			'd'
#define FAST_DOWN			's'
#define ROTATE_LEFT			'w'
#define ROTATE_RIGHT			'e'
#define QUIT_GAME			'q'
#define RESET_GAME			'r'
#define PAUSE_GAME			'p'

// gameplay
#define SLEEP_MS			16				// check input 1000 / SLEEP_MS times a second
#define FPS_RATE			1000 / SLEEP_MS			// fps rate
#define LEVEL_1_SPEED			FPS_RATE / 2			// half a second starting speed
#define LEVEL_UP			10				// how many points level you up
#define MAX_LEVEL			15				// max level
#define DOWN_QUICKER			10				// move DOWN_QUICKER times faster
#define SPEED_RESOLUTION		LEVEL_1_SPEED / MAX_LEVEL 	// calculates equal speed increments
/* LEVEL_1_SPEED / MAX_LEVEL = equalt speed decrements to max speed
 * of FPS_RATE times a second */

// movement
const int left[2] 		= 	{0, -1};	// left, right, and down are used for movement by
const int right[2] 		= 	{0, 1};		// adding them to the pivot point field coordinates
const int down[2] 		= 	{1, 0};		// and recalculating the shape position

/* ---------------- STRUCTS  ----------------*/
// the shape on the field
struct shape {
	int (* grid)[MATRIX_SIZE];			// grid pointer
	int coordinates[SHAPE_ELEMENTS][2]; 		// places of shape elements in it's grid
	int (*field_coords)[2];				// the current coordinates on the field
	int length;					// the longest part of the shape
	const int * pivot;				// pivot point of rotation
	int code;					// code for the shape
};

// the data for the shape on the field
struct shape_data {
	int grid[MATRIX_SIZE][MATRIX_SIZE];		// holds the grid for the shape
	int pivot[2];					// holds the pivot point of rotation
	int code;					// holds the code for the shape elements
};

/* Note: the shape code replaces the non-zero elements of the grid on run time */

/* ---------------- TYPEDEFS  ----------------*/
typedef struct shape shape;
typedef struct shape_data shape_data;

/* ---------------- FUNCTIONS  ----------------*/
// core functions
void calculate_field_coordinates(shape * tetr, int row, int col);
void scan_field(void);				// scans for full rows, removes them, and updates the score
void initialize_field(void);
void reset_field(void);

void rotate_90d(shape * tetr, int direction);
void get_shape_from_grid(shape * tetr);		// gets the shape coordinates from it's original grid
void initialize_shape(shape * s, shape_data * s_data);
void create_new_shape(shape * tetr);
void code_shape(shape * s);
int shape_rand(void);
void place_shape_on_field(shape * s);
void clear_current_coordinates(shape * s);	// temporary removes the shape from the field
void put_shape_back(shape * s);
void move(shape * s, const int * direction);

bool is_position_legal(shape * s, int (* coordinates)[2]);
bool is_position_collision(shape * s, int (* coordinates)[2]);
bool is_position_out_of_bounds(shape * s, int (* coordinates)[2]);

void copy_matrix(int rows, int cols, int m_s[rows][cols], int m_d[rows][cols]);
void get_field(void);	// copies field in field_buffer

// gameplay
void render(void);
void sleep_ms(int ms);
void move_cursor_xy(int row, int col);
void clear_screen(void);
void show_cursor(bool show);
void end_game(void);
void exit_game(void);
void show_next(shape_data * sd, int row, int col);
void show_score(int score, int row, int col);
void show_level(int level, int row, int col);
void show_controls(int row, int col);
void pause_game(void);
bool ask_player(char * msg);

void print_m(int row, int col, int m[row][col]); 	// for debugging

// globals
int field[FIELD_ROWS][FIELD_COLS];			// the game field
int field_buffer[FIELD_ROWS][FIELD_COLS];		// used for comparison with the previous field state on render
int buffer_matrix[MATRIX_SIZE][MATRIX_SIZE];		// the shape grid goes in here
int buffer_matrix_copy[MATRIX_SIZE][MATRIX_SIZE];	// used as a copy for pre-calculating and checking position
int m_temp[MATRIX_SIZE][MATRIX_SIZE];			// used to hold temporary matrix transformations
int shape_field_coordinates[SHAPE_ELEMENTS][2];		// the coodinats of every element of the shape on the field
int buffer_shape_field_coords[SHAPE_ELEMENTS][2];	// used for pre-calculating and checking validity

char frame_buffer[FRAME_BUFF_ROWS][FRAME_BUFF_COLS];

int score;
int level;
bool is_game_over;
bool did_shape_hit_bottom;

/* next three are used to check if score, level,
 * and the next shape screen need re-rendering */
bool is_score_new;
bool is_level_new;
bool is_next_updated;

// used to move the cursor
int render_row, render_col;

// next shape ponter
shape_data * next_shape;

// structures defining the shapes
shape_data square	=  {	.grid 	= 	{	{0, 0, 0, 0}, 
							{0, 1, 1, 0},
							{0, 1, 1, 0},
							{0, 0, 0, 0}	},
									
							.pivot 	= 	{0, 0},	 // 0, 0 - don't rotate
							.code 	= 	SQUARE	};
						  
shape_data candy_cane_left	=	{	.grid	=	{	{0, 0, 0 ,0},
									{0, 0, 0, 1},
									{0, 1, 1 ,1},
									{0, 0, 0, 0}	},
													  
									.pivot	=	{2, 2},
									.code	=	CANDY_CANE	};

shape_data candy_cane_right	=	{	.grid	=	{	{0, 0, 0, 0},
									{0, 0, 0, 1},
									{0, 1, 1, 1},
									{0, 0, 0, 0}	},
									
									.pivot	=	{2, 2},
									.code	=	CANDY_CANE	};
						  
shape_data traffic_sign	=	{	.grid	=	{	{0, 0, 0, 0},
								{0, 0, 1, 0},
								{0, 1, 1, 1},
								{0, 0, 0, 0}	},
												
								.pivot	=	{2, 2},
								.code	=	TRAFFIC_SIGN	};
							 
shape_data z_shape_left	=	{	.grid	=	{	{0, 0, 0, 0},
								{0, 1, 1, 0},
								{0, 0, 1, 1},
								{0, 0, 0, 0}	},
												
								.pivot	=	{2, 2},
								.code	=	Z_SHAPE		};
								  
shape_data z_shape_right	=	{	.grid	=	{	{0, 0, 0, 0},
									{0, 0, 1, 1},
									{0, 1, 1, 0},
									{0, 0, 0, 0}	},
									
									.pivot	=	{2, 2},
									.code	=	Z_SHAPE	};

shape_data stick	=	{	.grid	=	{	{0, 0, 0, 0, 0},
								{0, 0, 0, 0, 0},
								{0, 1, 1, 1, 1},
								{0, 0, 0, 0, 0}	},
											
							.pivot	=	{2, 2},
							.code	=	STICK	};
