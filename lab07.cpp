// Name:  Lorrin Smith
// Class: CSCI-C435
// Date:  2/15/2026
// Assignment: Lab #7 (modified Lab #4)

// Command: g++ lab07.cpp -lpthread -lncurses

#include <iostream>
#include <pthread.h>			// needed for using the pthread library
#include <assert.h>
#include <time.h>
#include <unistd.h>				// needed for sleep()
#include <unistd.h>				// needed for sleep()
#include <ncurses.h>			// needed for Curses windowing
#include <stdarg.h>				// needed for formatted output to window
#include <termios.h>
#include <fcntl.h>

using namespace std;

// State information for each thread
const int STARTED    = 0;
const int READY      = 1;
const int RUNNING    = 2;
const int BLOCKED    = 3;
const int TERMINATED = 4;

// forward declaration
void display_screen_data();

WINDOW *create_window(int height, int width, int starty, int startx);
void write_window(WINDOW * Win, const char* text);
void write_window(WINDOW * Win, int x, int y, const char* text);
void display_help(WINDOW * Win);

void *perform_simple_output(void *arguments);
void *perform_cpu_work(void *arguments);
void *perform_io_work(void *arguments);

// Data Structure for each thread. Each thread has access to its own WINDOW.
struct thread_data
{
	int thread_no;
	int thread_state;
	WINDOW *thread_win;
	bool kill_signal;
	int sleep_time;
	int thread_results;
	// other stuff
};

// global mutual exclusion
pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER;

// utility function
void display_screen_data()
{
	int Y, X;
	int Max_X, Max_Y;
	
	start_color();
	// define color pairs: (pair_number, foreground, background)
	init_pair(1, COLOR_RED,   COLOR_BLACK);
	init_pair(2, COLOR_GREEN, COLOR_BLACK);
	
	sleep(1);						// sleep() call added
	attron(COLOR_PAIR(1));			// use color pair 1
	getmaxyx(stdscr, Max_Y, Max_X);	// get screen size
	wprintw(stdscr, "Initial Screen Height = %d, Initial Screen Width = %d\n", Max_Y, Max_X);
	attroff(COLOR_PAIR(1));
	
	sleep(1);						// sleep() call added
	attron(COLOR_PAIR(2));			// use color pair 2
	getyx(stdscr, Y, X);	// get current (y,x) coordinate of the cursor
	wprintw(stdscr, "Current Y = %d, Current X = %d\n", Y, X);
	attroff(COLOR_PAIR(2));
	
	refresh();
}

WINDOW *create_window(int height, int width, int starty, int startx)
{
	pthread_mutex_lock(&myMutex);	// LOCK
	
	WINDOW *Win;
	
	Win = newwin(height, width, starty, startx);
	scrollok(Win, TRUE);			// allow scrolling of the window
	scroll(Win);					// scroll the window
	box(Win, 0, 0);					// (0,0) gives default characters for vertical and horizontal lines
	
	sleep(1);						// sleep() call added
	wrefresh(Win);					// draw the window
	
	pthread_mutex_unlock(&myMutex);	// UNLOCK
	
	return Win;
}

void write_window(WINDOW * Win, const char* text)
{
	pthread_mutex_lock(&myMutex);	// LOCK
	
	wprintw(Win, text);
	box(Win, 0, 0);
	
	sleep(1);						// sleep() call added
	wrefresh(Win);					// draw the window
	pthread_mutex_unlock(&myMutex);	// UNLOCK
}

void write_window(WINDOW * Win, int y, int x, const char* text)
{
	pthread_mutex_lock(&myMutex);	// LOCK
	
	mvwprintw(Win, y, x, text);
	box(Win, 0, 0);
	
	sleep(1);						// sleep() call added
	wrefresh(Win);					// draw the window
	pthread_mutex_unlock(&myMutex);	// UNLOCK
}

void display_help(WINDOW * Win)
{
	wclear(Win);
	write_window(Win, 1, 1, "...Help...");
	write_window(Win, 2, 1, "1= Kill T1");
	write_window(Win, 3, 1, "2= Kill T2");
	
	sleep(1);						// sleep() call added
	write_window(Win, 4, 1, "3= Kill T3");
	write_window(Win, 5, 1, "c= clear screen");
	write_window(Win, 6, 1, "h= help screen");
	write_window(Win, 7, 1, "q= Quit");
}

void *perform_simple_output(void *arguments)
{
	// extract the thread arguments: (method 1)
	// cast arguments into thread_data
	thread_data *td = (thread_data *) arguments;
	
	int thread_no		= td->thread_no;
	int sleep_time		= td->sleep_time;
	WINDOW * Win		= td->thread_win;
	bool kill_signal	= td->kill_signal;
	
	int CPU_Quantum = 0;
	char buff[256];
	
	while(!td->kill_signal)
	{
		sprintf(buff, " Task-%d running #%d\n", thread_no, CPU_Quantum++);
		write_window(Win, buff);
		sleep(thread_no*2);
	}
	td->thread_state = TERMINATED;		// thread is entering TERMINATED state
	write_window(Win, " TERMINATED");
	return(NULL);
}

void *perform_io_work(void *arguments)
{
	// cast arguments into thread_data
	thread_data *td = (thread_data *) arguments;
	
	int thread_no		= td->thread_no;
	int sleep_time		= td->sleep_time;
	WINDOW * Win		= td->thread_win;
	bool kill_signal	= td->kill_signal;
	
	// do some I/O, may get an interrupt
	char buff[256];
	sprintf(buff, " T-%d Started\n", thread_no);
	write_window(Win, buff);
	
	sprintf(buff, " Sleeping for %d seconds\n", sleep_time);
	write_window(Win, buff);
	sleep(sleep_time);
	
	sprintf(buff, " T-%d Results = %d\n", thread_no, td->thread_results);
	write_window(Win, buff);
	
	sprintf(buff, " T-%d Finished its work\n", thread_no);
	write_window(Win, buff);
	
	td->thread_state = TERMINATED;		// thread is ending and entering TERMINATED state
	write_window(Win, " TERMINATED");
	return(NULL);
}

void *perform_cpu_work(void *arguments)
{
	// cast arguments into thread_data
	thread_data *td = (thread_data *) arguments;
	
	int thread_no		= td->thread_no;
	int sleep_time		= td->sleep_time;
	WINDOW * Win		= td->thread_win;
	
	char buff[256];
	
	for (int i = 0; i < 10; i++)
	{
		if (td->kill_signal == true)
			break;
		srand (time(NULL));		// initialize random seed
		td->thread_results += i * (rand() % 10);
		sprintf(buff, " T-%d Result=%d\n", thread_no, td->thread_results);
		write_window(Win, buff);
		sleep(thread_no*2);
	}
	
	td->thread_state = TERMINATED;		// thread is ending and entering TERMINATED state
	write_window(Win, " TERMINATED");
	return(NULL);
}

int main()
{
	// create thread variables
	pthread_t thread_1, thread_2, thread_3;
	
	// create thread data
	thread_data thread_args_1, thread_args_2, thread_args_3;
	
	int result_code;
	
	initscr();				// start nCurses
	display_screen_data();	// display the stdscr display geometry
	
	/***** Heading Window *****/
	
	// create a window to display thread data in
	// Syntax: WINDOW * win = newwin(nlines, ncols, y0, x0);
	
	WINDOW * Heading_Win = newwin(12, 80, 3, 2);
	box(Heading_Win, 0,0);
	mvwprintw(Heading_Win, 2, 28, "ULTIMA 2.0 (Spring 2019-2026)");
	
	mvwprintw(Heading_Win, 4, 2, "Starting ULTIMA 2.0.....");
	mvwprintw(Heading_Win, 5, 2, "Starting Thread 1....");
	mvwprintw(Heading_Win, 6, 2, "Starting Thread 2....");
	mvwprintw(Heading_Win, 7, 2, "Starting Thread 3....");
	mvwprintw(Heading_Win, 9, 2, "Press 'q' or Ctrl-C to exit the program...");
	wrefresh(Heading_Win);	// changes to the window are refreshed
	
	/***** Log Window *****/
	WINDOW * Log_Win = create_window(10, 60, 30, 2);
	write_window(Log_Win, 1, 5, " ....Log Window....\n");
	write_window(Log_Win, " ........Main program started........\n");
	
	/***** Console Window *****/
	WINDOW * Console_Win = create_window(10, 20, 30, 62);
	write_window(Console_Win, 1, 1, "....Console....\n");
	write_window(Console_Win, 2, 1, "Ultima # ");
	
	/***** Thread_1 Window *****/
	
	// set the thread_args
	thread_args_1.thread_no = 1;
	thread_args_1.thread_state = RUNNING;
	thread_args_1.thread_win = create_window(15, 25, 15, 2);
	write_window(thread_args_1.thread_win, 6, 1, "Starting Thread 1.....\n");
	thread_args_1.sleep_time = 1 + rand() % 3;		// get an integer between 1 and 5
	thread_args_1.kill_signal = false;
	thread_args_1.thread_results = 0;				// initialize results
	
	// create the new thread to do CPU bound or IO bound stuff
	result_code = pthread_create(&thread_1, NULL, perform_simple_output, &thread_args_1);
	assert(!result_code);		// if any problems with result code, display it and end program
	write_window(Log_Win, " Thread 1 Created.. \n");	// write to log window
	
	/***** Thread_2 Window *****/
	
	// set the thread_args
	thread_args_2.thread_no = 2;
	thread_args_2.thread_state = RUNNING;
	thread_args_2.thread_win = create_window(15, 25, 15, 30);
	write_window(thread_args_2.thread_win, 6, 1, "Starting Thread 2.....\n");
	thread_args_2.kill_signal = false;
	thread_args_2.sleep_time = 1 + rand() % 3;		// get an integer between 1 and 5
	thread_args_2.thread_results = 0;				// initialize results
	
	// create the new thread to do CPU bound or IO bound stuff
	result_code = pthread_create(&thread_2, NULL, perform_simple_output, &thread_args_2);
	// result_code = pthread_create(&thread_2, NULL, perform_io_work, &thread_args_2);
	assert(!result_code);		// if any problems with result code, display it and end program
	write_window(Log_Win, " Thread 2 Created.. \n");	// write to log window
	
	/***** Thread_3 Window *****/
	
	// set the thread_args
	thread_args_3.thread_no = 3;
	thread_args_3.thread_state = RUNNING;
	thread_args_3.thread_win = create_window(15, 25, 15, 57);
	write_window(thread_args_3.thread_win, 6, 1, "Starting Thread 3.....\n");
	thread_args_3.kill_signal = false;
	thread_args_3.sleep_time = 1 + rand() % 3;		// get an integer between 1 and 5
	thread_args_3.thread_results = 0;				// initialize results
	
	// create the new thread to do CPU bound or IO bound stuff
	result_code = pthread_create(&thread_3, NULL, perform_simple_output, &thread_args_3);
	// result_code = pthread_create(&thread_3, NULL, perform_cpu_work, &thread_args_3);
	assert(!result_code);		// if any problems with result code, display it and end program
	write_window(Log_Win, " Thread 3 Created.. \n");	// write to log window
	
	write_window(Log_Win, " All threads have been created...\n");
	
	/***** Set up keyboard I/O processing *****/
	
	cbreak();			// disable line buffering
	noecho();			// disable automatic echo of characters read by getch(), wgetch()
	nodelay(Console_Win, TRUE);		// nodelay causes getch() to be a non-blocking call.
									// If no input is ready, getch() returns ERR
	keypad(Console_Win, TRUE);
	
	char buff[256];
	int input = -1;
	int CPU_Quantum = 1;
	
	while (input != 'q')
	{
		input = wgetch(Console_Win);
		
		switch(input)
		{
			case '1':
			case '2':
			case '3':
				if (input == '1')
					thread_args_1.kill_signal = true;
				else if (input == '2')
					thread_args_2.kill_signal = true;
				else if (input == '3')
					thread_args_3.kill_signal = true;
				
				sprintf(buff, " %c\n", input);
				write_window(Console_Win, buff);
				sprintf(buff, " Kill = %c\n", input);
				write_window(Console_Win, buff);
				write_window(Log_Win, buff);
				
				sleep(4);
				wclear(Console_Win);
				write_window(Console_Win, 1, 1, "Ultima # ");
				break;
			
			case 'c':
				// clear the console window
				refresh();				// clear the entire screen (in case it is corrupted)
				wclear(Console_Win);	// clear the console window
				write_window(Console_Win, 1, 1, "Ultima # ");
				break;
			
			case 'h':
				display_help(Console_Win);
				write_window(Console_Win, 8, 1, "Ultima # ");
				break;
			
			case 'q':
				// end the loop and the program
				write_window(Log_Win, " Quitting the main program....\n");
				write_window(Log_Win, " Signal the remaining child processes to stop as well.\n");
				
				// also signal the child threads to kill themselves...
				thread_args_1.kill_signal = true;
				thread_args_2.kill_signal = true;
				thread_args_3.kill_signal = true;
				break;
			
			case ERR:
				break;
			
			default:
				sprintf(buff, " %c\n", input);
				write_window(Console_Win, buff);
				write_window(Console_Win, " -Invalid Command\n");
				write_window(Log_Win, buff);
				write_window(Log_Win,	  " -Invalid Command\n");
				write_window(Console_Win, " Ultima # ");
				break;
		}
		
		sleep(1);
		CPU_Quantum++;
	}
	
	write_window(Log_Win, " Waiting for living threads to complete..\n");
	result_code = pthread_join(thread_1, NULL);
	result_code = pthread_join(thread_2, NULL);
	result_code = pthread_join(thread_3, NULL);
	
	write_window(Log_Win, " All threads have now ended.....\n");
	write_window(Log_Win, " ........Main program ended........\n");
	
	sprintf(buff, " Thread 1 State = %d\n", thread_args_1.thread_state);
	write_window(Log_Win, buff);
	sprintf(buff, " Thread 2 State = %d\n", thread_args_2.thread_state);
	write_window(Log_Win, buff);
	sprintf(buff, " Thread 3 State = %d\n", thread_args_3.thread_state);
	write_window(Log_Win, buff);
	
	sleep(5);			// sleep for 5 seconds before ending the program
	//getch();
	endwin();			// end the curses window
	return 0;
}