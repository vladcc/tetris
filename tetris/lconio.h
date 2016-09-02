/* implementations of conio.h's kbhit() and getch() gathered around 
 * from the interenet and customized a bit
 * used when LINUX is defined */

// includes
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

// function declarations
void disable_echo(void);
void enable_echo(void);
int kbhit(void);
int getch(void);

// globals
struct termios oldattr, newattr;
int oldf;

// function definitions
void disable_echo(void)
{
	tcgetattr(STDIN_FILENO, &oldattr);
	newattr = oldattr;
	newattr.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
}

void enable_echo(void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
	fcntl(STDIN_FILENO, F_SETFL, oldf);
}

int kbhit(void)
{
	int ch;

	ch = getchar();

	if(ch != EOF)
	{
		ungetc(ch, stdin);
		return 1;
	}

	return 0;
}

int getch(void)
{
	return getchar();
}
