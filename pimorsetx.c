/**********************************************************************
* Filename    : BtnAndLed.c
* Description : Controlling an led by button.
* Author      : Robot
* E-mail      : support@sunfounder.com
* website     : www.sunfounder.com
* Date        : 2014/08/27
**********************************************************************/
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
#include <wiringPi.h>
#include <stdio.h>

#ifndef NOPI
#include <wiringPi.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include  <signal.h>
#include <unistd.h>
#include <sys/time.h> // for clock_gettime()
#include <time.h> // for clock_gettime()
#include <sys/select.h>
#include <termios.h>
#include <ctype.h>

#define BeepPin 0
#define ButtonPin 1
#define LedPin    3

#define MAXMORSELENGTH 11
#define MAXMESSAGELENGTH 250

const unsigned char Kalfav[] = { 'A','B','C','D','E','F','G','H','I','J',
            'K','L','M','N','O','P','Q','R','S','T',
            'U','V','W','X','Y','Z', // Alphabet chars
            '1','2','3','4','5','6','7','8','9','0',' ', // Numbers + space
            '.',',',':','?','=','-','(',')','"','\'','/','_','@','!' // Symbols
};

const char Kmorsev[][MAXMORSELENGTH] = {
    ".-","-...","-.-.","-..",".","..-.","--.","....","..",".---",
    "-.-",".-..","--","-.","---",".--.","--.-",".-.","...","-",
    "..-","...-",".-- ","-..-","-.--","--..", // Alphabet chars
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

typedef unsigned char bool;

bool Is_seq_started=0, State_pressed=0;
                         
struct termios orig_termios;

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

int getch()
{
  int r;
  unsigned char c;
  if ((r = read(0, &c, sizeof(c))) < 0) {
    return r;
  } else {
    return c;
  }
}

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

long microseconds2(struct timespec start, struct timespec end)
{
  long secs_used,micros_used;

  printf("start: %ld secs, %ld nsecs,",start.tv_sec,start.tv_nsec);
  printf("end: %ld secs, %ld nsecs,",end.tv_sec,end.tv_nsec);

  secs_used=(end.tv_sec - start.tv_sec); //avoid overflow by subtracting first
  micros_used= ((secs_used*1000000) + (end.tv_nsec/1000)) - (start.tv_nsec/1000);

  printf("micros_used: %ld\r\n",micros_used); // GABODebug

  return micros_used;
}

int main(void)
{
  // struct timeval start, end;
  struct timespec ts_start,ts_end;
  long micros_used;
  int c;

  set_conio_terminal_mode();

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



  pinMode(LedPin, OUTPUT); 
  pinMode(ButtonPin, INPUT);
  pinMode(BeepPin, OUTPUT);   //set GPIO0 output

  pullUpDnControl(ButtonPin, PUD_UP);  //pull up to 3.3V,make GPIO1 a stable level
  digitalWrite(BeepPin, HIGH); //beep off
  digitalWrite(LedPin, HIGH); // Led off
  printf("\r\n Waiting for keyboard input. Press 'Q' to quit.\r\n");
  while(1){
    if(kbhit()) {
      c=getch();
      printf(" Pressed key (#%d) = %c\r\n",c,c);
      if(toupper(c)=='Q') break;
    }
    if(digitalRead(ButtonPin) == 0){ //indicate that button has pressed down
      digitalWrite(LedPin, LOW);   //led on
      digitalWrite(BeepPin, LOW);  //beep on

      if(Is_seq_started) {
        if(!State_pressed){
          // gettimeofday(&end, NULL);
          clock_gettime(CLOCK_MONOTONIC, &ts_end);
          // micros_used=microseconds(start,end);
          micros_used=microseconds2(ts_start,ts_end);
          printf("- Microsecond passed in last SILENCE (dot or line) : %ld (millis=%ld)\r\n\r\n",micros_used,micros_used/1000L);
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
          micros_used=microseconds2(ts_start,ts_end);
          printf("+ Microsecond passed in last SIGN (dot or line) : %ld (millis=%ld)\r\n",micros_used,micros_used/1000L);
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
  return 0;
}

/* 
 * ex: nohls ts=2 sts=2 sw=2 expandtab mouse-=a:
 **/
