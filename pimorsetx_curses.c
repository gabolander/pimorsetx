/**********************************************************************
* Filename    : pimorsetx.c
* Description : 
* Author      : Gabriele "Gabolander" Zappi
* E-mail      : gabodevelop@gmail.com
* website     : (not useful here)
* Date        : 2017/04/30
* License     : This source is released under the terms of GNU General
*               Public License V. 3.0. 
*               Permissions of this strong copyleft license are condi-
*               tioned on making available complete source code of 
*               licensed works and modifications, which include larger 
*               works using a licensed work, under the same license. 
*               Copyright and license notices must be preserved. Con-
*               tributors provide an express grant of patent rights.
*               Please read LICENSE.txt released with this code for 
*               further details.
**********************************************************************/
#ifndef NOPI
#include <wiringPi.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
// #include <sys/time.h> // for gettimeofday()
#include <time.h> // for clock_gettime()
#include <sys/select.h>
#include <termios.h>
#include <ctype.h>
#include <curses.h>


#define BeepPin 0
#define ButtonPin 1
#define LedPin    3

#define MAXMORSELENGTH 11
#define MAXMESSAGELENGTH 250

#define MC_CODE_SPACE   0
#define MC_CODE_SIGN    1
#define MAX_MORSE_BUFFER_SIZE 4097 // To create a 4K morse buffer

const unsigned char Kalfav[] = { 'A','B','C','D','E','F','G','H','I','J',
            'K','L','M','N','O','P','Q','R','S','T',
            'U','V','W','X','Y','Z', // Alphabet chars
            '1','2','3','4','5','6','7','8','9','0',' ', // Numbers + space
            '.',',',':','?','=','-','(',')','"','\'','/','_','@','!' // Symbols
};

const char Kmorsev[][MAXMORSELENGTH] = {
    ".-","-...","-.-.","-..",".","..-.","--.","....","..",".---",
    "-.-",".-..","--","-.","---",".--.","--.-",".-.","...","-",
    "..-","...-",".--","-..-","-.--","--..", // Alphabet chars
    ".----","..---","...--","....-",".....",
    "-....","--...","---..","----.","-----"," ", // Numbers + space
    ".-.-.-","--..--","---...","..--..","-...-","-....-","-.--.",
    "-.--.-",".-..-.",".----.","-..-.","..--.-",".--.-.","-.-.--" // Symbols
};
                         
                         
// 16-04 changed to 80 ms
#define DOT_MS_LEN  150   // "BASE" DOT LENGTH: This is the main 'dot' length which all
                          // the other delay/durations are based on.
                          // Decrease it, to make it playing faster.
                          // Increase it, to make it slower.
                          // Don't touch other DASH_* DELAY_* values instead, because
                          // they are multipliers of DOT_MS_LEN.
#define DASH_MS_LEN (DOT_MS_LEN*3)
#define DELAY_MS_DOTS (DOT_MS_LEN*1)
#define DELAY_MS_LETTERS (DOT_MS_LEN*3)
#define DELAY_MS_WORDS (DOT_MS_LEN*5)

#define SEC_TIME_MS 5

#define VECT_BEAT_MAX_ANALYSIS 10

// typedef unsigned char bool;

bool Is_seq_started=0, State_pressed=0;

long Dot_ms_len;
long Dash_ms_len;
long Delay_ms_dots;
long Delay_ms_letters;
long Delay_ms_words;
WINDOW * Win;

void reset_delays(long);
unsigned char evaluate_event(char *, long, unsigned char);

struct termios orig_termios;

void reset_delays(long dot_delay) {
    // char tmp[1025];
    if( dot_delay == Dot_ms_len ) return;
    Dot_ms_len=dot_delay;
    Dash_ms_len=dot_delay*3L;
    Delay_ms_dots=dot_delay;
    Delay_ms_letters=dot_delay*3L;
    Delay_ms_words=dot_delay*5L;
// #ifdef DEBUG
    // move(0, COLS/2+1); 	hline(' ',COLS);
    // mvwprintw(Win,0,COLS/2+1,"New dot delay is %ld\r\n",Dot_ms_len);
    mvprintw(0,COLS/2+1,"New dot delay is %ld\r\n",Dot_ms_len);
    // printf("DOT %ld",Dot_ms_len);
    fflush(stdout);
//     sprintf(tmp,"New dot delay is %ld\r\n",Dot_ms_len);
//     printw(tmp);
// #endif    
}

char decode_buffer(char * buffer){
    register int i,vlen;
    vlen=(int)sizeof(Kalfav);
    for(i=0;i<vlen;i++) {
        if(!strcmp(buffer,Kmorsev[i]))
            break;
    }
    return (i>=vlen)?'#':Kalfav[i];
}

void analyze_times(long dot_millis) {
    static int cursor=3; // starts from 4th position of array
    static long vect[VECT_BEAT_MAX_ANALYSIS]={120L,120L,100L,0L,0L,0L,0L,0L,0L,0L};
    long sum=0L;
    long new_dot_len;
    int counter;
    
    vect[cursor++]=dot_millis;
    if(cursor>=VECT_BEAT_MAX_ANALYSIS) cursor=0;
    
    for(counter=0;counter<VECT_BEAT_MAX_ANALYSIS;counter++) {
        if(!vect[counter]) return; // if 0L, quits at once
        sum+=vect[counter];
    }
    
    new_dot_len=sum/VECT_BEAT_MAX_ANALYSIS;
    reset_delays(new_dot_len);
}


/* e.g.:          res=evaluate_event(sz_morse_buffer, micros_used, MC_CODE_SPACE); */
unsigned char evaluate_event(char * buffer, long micros, unsigned char chartype)
{
    unsigned char ret=0x00;
    long millis=micros/1000L;
    char c;
    char s[4]="";
    /*
     * return value "ret" can be:
     *  0x01 - registered a SILENCE gap between two signs of the same letter 
     *        (dot or line)
     *  0x02 - registered a SILENCE between two letters 
     *  0x04 - registered a SILENCE between two words
     *  0x10 - registered a DOT SIGN
     *  0x20 - registered a LINE SIGN
     */

    if( chartype == MC_CODE_SPACE ) {
        // Detecting end of word (put a space between word) ...
        if(millis>Delay_ms_words || 
            millis>(Delay_ms_letters+((Delay_ms_words-Delay_ms_letters)/2))) 
        {
            // I must feed of one SPACE, and empty buffer
            // ...
            ret=0x04;
            buffer[0]='\0';
            putchar(' ');
        } else if(millis>(Delay_ms_dots*2L)) {
            // Detecting end of letter ...
                ret=0x02;
                c=decode_buffer(buffer);
                buffer[0]='\0';
                putchar(c);
                fflush(stdout);
            } else {
                // Detecting end of sign (DOT or LINE) ...
                ret=0x01;
            }
    } else { /* ( chartype == MC_CODE_SIGN ) */
        if(millis>Dash_ms_len || 
            millis>(Dot_ms_len*2)) 
        {
            // Add a dash
            strcpy(s,"-");
            ret=0x20;
        } else {
            // Add a dot
            strcpy(s,".");
            ret=0x10;
            analyze_times(millis);
        }
        if(strlen(buffer)<=(MAX_MORSE_BUFFER_SIZE-1))
            strcat(buffer,s);
        
    }

#ifdef DEBUG
    printf("Update buffer value: [%s]\r\n",buffer);
#endif    
    
    
    return ret;
}



void reset_terminal_mode()
{
    tcsetattr(0, TCSANOW, &orig_termios);
}

void set_conio_terminal_mode()
{
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(reset_terminal_mode);
    cfmakeraw(&new_termios);
    tcsetattr(0, TCSANOW, &new_termios);
}

int kbhit()
{
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv);
}

// int getch()
// {
//   int r;
//   unsigned char c;
//   if ((r = read(0, &c, sizeof(c))) < 0) {
//     return r;
//   } else {
//     return c;
//   }
// }

/* this function is for use with gettimeofday() function and
 * struct timeval struct - but I want to avoid to use 
 * gettimeofday() but clock_gettime() instead and timespec
 * structure *
long microseconds(struct timeval start, struct timeval end)
{
  long secs_used,micros_used;

  printf("start: %ld secs, %ld usecs,",start.tv_sec,start.tv_usec);
  printf("end: %ld secs, %ld usecs,",end.tv_sec,end.tv_usec);

  secs_used=(end.tv_sec - start.tv_sec); //avoid overflow by subtracting first
  micros_used= ((secs_used*1000000) + end.tv_usec) - (start.tv_usec);

  printf("micros_used: %ld\r\n",micros_used); // GABODebug

  return micros_used;
}
*/

long microseconds(struct timespec start, struct timespec end)
{
  long secs_used,micros_used;

#ifdef DEBUG
  printf("start: %ld secs, %ld nsecs,",start.tv_sec,start.tv_nsec);
  printf("end: %ld secs, %ld nsecs,",end.tv_sec,end.tv_nsec);
#endif    

  secs_used=(end.tv_sec - start.tv_sec); //avoid overflow by subtracting first
  micros_used= ((secs_used*1000000) + (end.tv_nsec/1000)) - (start.tv_nsec/1000);

#ifdef DEBUG
  printf("micros_used: %ld\r\n",micros_used); // GABODebug
#endif

  return micros_used;
}
int base_mask( void )
{
	register int i;
	int pos;
	/* Traccio le linee orizzontali */
	color_set( 1 ,NULL);
	move(2,0);  hline( ACS_HLINE, COLS );
	move(LINES-4,0); hline( ACS_HLINE, COLS );
	move(LINES-2,0); hline( ACS_HLINE, COLS );

	/* Cancello il corpo della videata */
	color_set( 0 ,NULL);
	for(i=3;i<(LINES-4);i++) {
		mvhline( i,0, ' ', 80 );
	}

	return 0;
}

int main(void)
{
  // struct timeval start, end;
  struct timespec ts_start,ts_end;
  long micros_used;
  int c,i,j;
  unsigned char res=0x0;
  
  char sz_morse_buffer[MAX_MORSE_BUFFER_SIZE];

  // curses stuff - start
  Win=initscr();
  start_color();
  // crmode();
  noecho();
//   curs_set(0);
//   keypad(stdscr,TRUE);
//   erase();
//   refresh();
  cbreak();         // don't interrupt for user input
  // timeout(500);     // wait 500ms for key press
  timeout(1);     // wait 1ms for key press

  for(i=0;i<COLORS;i++)
    for(j=0;j<COLORS;j++)
        init_pair(i*COLORS+j,j, i);
  // curses stuff - end
  
  
  // set_conio_terminal_mode();
  base_mask();

  if(wiringPiSetup() == -1){ //when initialize wiring failed,print messageto screen
    printf("setup wiringPi failed !");
    return 1; 
  }

  /** PER MISURARE IL TEMPO - INIZIO **/
#if 0
  gettimeofday(&start, NULL);
  usleep(1250000); // Do the stuff you want to time here
  gettimeofday(&end, NULL);

  printf("start: %d secs, %d usecs\n",start.tv_sec,start.tv_usec);
  printf("end: %d secs, %d usecs\n",end.tv_sec,end.tv_usec);

  secs_used=(end.tv_sec - start.tv_sec); //avoid overflow by subtracting first
  micros_used= ((secs_used*1000000) + end.tv_usec) - (start.tv_usec);

  printf("micros_used: %d\n",micros_used);
#endif
  /** PER MISURARE IL TEMPO - FINE **/

  // I reset global values for measuring the times
  reset_delays((long)DOT_MS_LEN);
  sz_morse_buffer[0]='\0';

  pinMode(LedPin, OUTPUT); 
  pinMode(ButtonPin, INPUT);
  pinMode(BeepPin, OUTPUT);   //set GPIO0 output

  pullUpDnControl(ButtonPin, PUD_UP);  //pull up to 3.3V,make GPIO1 a stable level
  digitalWrite(BeepPin, HIGH); //beep off
  digitalWrite(LedPin, HIGH); // Led off
  printf("\r\n Waiting for keyboard input. Press 'Q' to quit.\r\n");
  while(1){
    c=getch();
    if(c>0)
        printf(" Pressed key (#%d) = %c\r\n",c,c);
    if(toupper(c)=='Q') break;
    res=0x00;
    if(digitalRead(ButtonPin) == 0){ //indicate that button has pressed down
      digitalWrite(LedPin, LOW);   //led on
      digitalWrite(BeepPin, LOW);  //beep on

      if(Is_seq_started) {
        if(!State_pressed){
          // gettimeofday(&end, NULL);
          clock_gettime(CLOCK_MONOTONIC, &ts_end);
          // micros_used=microseconds(start,end);
          micros_used=microseconds(ts_start,ts_end);
#ifdef DEBUG          
          printf("- Microsecond passed in last SILENCE (dot or line) : %ld (millis=%ld)\r\n\r\n",micros_used,micros_used/1000L);
#endif          
          
          // here stats space's length
          res=evaluate_event(sz_morse_buffer, micros_used, MC_CODE_SPACE);

          // gettimeofday(&start, NULL);
          clock_gettime(CLOCK_MONOTONIC, &ts_start);
        }
      } else {
        // gettimeofday(&start, NULL);
        clock_gettime(CLOCK_MONOTONIC, &ts_start);
        Is_seq_started=1;
      }

      State_pressed=1;
    }
    else {  // Button is unpressed
      digitalWrite(BeepPin, HIGH); //beep off
      digitalWrite(LedPin, HIGH);
      if(Is_seq_started) {
        if(State_pressed) {
          // gettimeofday(&end, NULL);
          clock_gettime(CLOCK_MONOTONIC, &ts_end);
          // micros_used=microseconds(start,end);
          micros_used=microseconds(ts_start,ts_end);
#ifdef DEBUG          
          printf("+ Microsecond passed in last SIGN (dot or line) : %ld (millis=%ld)\r\n",micros_used,micros_used/1000L);
#endif
          // here stats DOR or LINE length
          res=evaluate_event(sz_morse_buffer, micros_used, MC_CODE_SIGN);
          
          // gettimeofday(&start, NULL);
          clock_gettime(CLOCK_MONOTONIC, &ts_start);
        }
        State_pressed=0;
      }
    }
    delay(SEC_TIME_MS);
  }

  // (void)getch(); /* consume the character */
  digitalWrite(BeepPin, HIGH); //beep off
  digitalWrite(LedPin, HIGH); // Led off
  printf("\r\nProgram ended.\r\n");

  clear();
  refresh();
  nocbreak();
  endwin();
  return 0;
}

/* 
 * ex: nohls ts=2 sts=2 sw=2 expandtab mouse-=a:
 **/
