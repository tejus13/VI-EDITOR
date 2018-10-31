//20172066 TEJUS AGARWAL
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <termios.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/wait.h>
using namespace std;
void normal_Mode();
void editorProcessKeypress();
struct termios terminos_orig;
struct winsize w;
struct editorConfig
{ 
    int cx,cy;
    int screenrows;
    int screencols;
    struct termios terminos_orig;
};
struct editorConfig E;

int getWindowSize(int *rows, int *cols)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
    {
        return -1;
    }
    else
    {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

void die(const char *s)
{
    
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(s);
    exit(1);
}
void signalHandler(int num){
  struct winsize w;
  printf("\x1b[%d;1H",E.screenrows-1);
  fflush(stdout);
  write(STDOUT_FILENO, "\x1b[K",3);//clear that line
  if(ioctl(STDOUT_FILENO,TIOCGWINSZ,&w)!=-1){
    E.screenrows=w.ws_row;
  }
  printf("\x1b[%d;H",E.screenrows-1);
  fflush(stdout);  
}

void initEditor()
{
    E.cx=1;
    if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

void Drawing()
{
    for(int i=0; i<=E.screenrows; i++)
    {
        write(STDOUT_FILENO, "~\r\n", 3);

    }
}
void editorRefreshScreen()
{
    write(STDOUT_FILENO, "\x1b[2J", 4);//clear whole screen
    write(STDOUT_FILENO, "\x1b[H", 3);//take cursor to first index
    Drawing();
    write(STDOUT_FILENO, "\x1b[H", 3);
}
void newRawMode()
{
    if (tcgetattr(STDIN_FILENO, &E.terminos_orig) == -1) die("tcgetattr");
    struct termios raw = E.terminos_orig;
    raw.c_lflag &=~(ICANON)&&~(ISIG);
    tcsetattr(STDIN_FILENO,TCSAFLUSH, &E.terminos_orig);
}

void disableRawMode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.terminos_orig) == -1)
        die("tcsetattr");
}

void enableRawMode()
{
    if (tcgetattr(STDIN_FILENO, &E.terminos_orig) == -1) die("tcgetattr");
    atexit(disableRawMode);
    struct termios raw = E.terminos_orig;
    raw.c_cflag |= (CS8);
    raw.c_iflag &= ~(BRKINT|ICRNL|INPCK|ISTRIP|IXON);
    raw.c_oflag &= ~(OPOST);

    raw.c_lflag &= ~(ECHO)&&~(ICANON)&&~(ISIG)&&~(IEXTEN);
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}
void command_mode(){
  char c,ks;
  read(0,&c,1);
  if(c==17){
    write(STDOUT_FILENO, "\x1b[2J", 4);//screen fflush
        write(STDOUT_FILENO, "\x1b[H", 3);//cursor relocate
        exit(0);
    }
  if(c=='q'){

        write(STDOUT_FILENO, "\x1b[2J", 4);//screen fflush
        write(STDOUT_FILENO, "\x1b[H", 3);//cursor relocate
        exit(0);
  }
  
  if(c=='!'){
    write(STDOUT_FILENO, "\x1b[2J", 4);
    int i=0;
    char cmd[100];
   
       read(0,&ks,1);
  
    while(ks!=13){
      
          cmd[i]=ks;
          write(STDOUT_FILENO,&cmd[i],1);
          i++;
          read(0,&ks,1);
      }

   int pid=fork();
   if(pid==0){
     write(STDOUT_FILENO, "\x1b[2J", 4);//screen fflush
        write(STDOUT_FILENO, "\x1b[H", 3);
        int pd=0;
        //disableRawMode();
        execl("/bin/bash","bash","-c",cmd,(char *)NULL);
      waitpid(pid,&pd,0);
   }
   

   int y;
        read(0,&y,1);
   write(STDOUT_FILENO, "\x1b[2J", 4);

   Drawing();
   normal_Mode();     

}
}
void normal_Mode(){       
  write(STDOUT_FILENO, "\x1b[H", 3); 
   for(int i=0;i<E.screenrows;i++){
        printf("\033[%d;%dH",E.screenrows,0);
        fflush(stdin);
      }
      
      write(STDOUT_FILENO, "\x1b[H", 3);
  while(1)
        {
            editorProcessKeypress();
        }
write(STDOUT_FILENO, "\x1b[H", 3);
}
void insertmode()
{ 
    newRawMode();
    for(int i=0;i<E.screenrows;i++){
        printf("\033[%d;%dH",E.screenrows,0);
        fflush(stdout);
      }
      write(STDOUT_FILENO, "==INSERT MODE==", 15);
      write(STDOUT_FILENO, "\x1b[H", 3);
      
       
        
    char c;
    while (read(STDIN_FILENO, &c, 1))
    {
       if (c==27)
        {
       write(STDOUT_FILENO, "\x1b[H", 3);
       for(int i=0;i<E.screenrows;i++){
        printf("\033[%d;%dH",E.screenrows,0);
        fflush(stdout);
      }
      write(STDOUT_FILENO, "\x1b[K",3);

          normal_Mode();

        }
        else 
        {
            if(c==13)
            {
                printf( "\r\n");
            }
            else
            {
                printf("%c",c);
                fflush(stdout);
            }
        }
    }
}
char editorReadKey()
{
    int nr;
    char c;
    while ((nr = read(STDIN_FILENO, &c, 1)) != 1)
    {
        if (nr == -1 && errno != EAGAIN) die("read");
    }
    if (c == '\x1b')
    {
        char seq[2];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
        if (seq[0] == '[')
        {
            switch (seq[1])//for mapping arrow keys
            {
            case 'A':
                return 'k';
            case 'B':
                return 'j';
            case 'C':
                return 'l';
            case 'D':
                return 'h';

            }
        }
        return '\x1b';
    }

    else
    {
        return c;
    }
}
void editorProcessKeypress()
{
  char key;

    char c = editorReadKey();
    switch (c)
    {
    case 17:
        write(STDOUT_FILENO, "\x1b[2J", 4);//screen fflush
        write(STDOUT_FILENO, "\x1b[H", 3);//cursor relocate
        exit(1);
        break;
    case 'h':
    write(STDOUT_FILENO,"\033[D",3);
        if(E.cx>1)
            E.cx--;
        break;
    case 'l':
        write(STDOUT_FILENO,"\033[C",3);
        E.cx++;
        break;

    case 'j':
        write(STDOUT_FILENO,"\033[B",3);
        //E.cy++;
        break;

    case 'k':
        write(STDOUT_FILENO,"\033[A",3);
        //E.cy--;
        break;
    case 'g':
        read(0,&key,1);
        if(key=='g')
            write(STDOUT_FILENO,"\033[H",3);
        break;
    case 'i':
    write(STDOUT_FILENO, "\x1b[H", 3);
         for(int j=0; j<E.screenrows; j++)
        {
            printf("\r\n");
            fflush(stdout);
        }
        write(STDOUT_FILENO, "\x1b[K",3);
        write(STDOUT_FILENO, "==INSERT MODE==", 15);
        write(STDOUT_FILENO, "\x1b[H", 3);
        insertmode();
        break;
    case 'G':
        printf("\x1b[%d;%dH",E.screenrows-1,1);
        fflush(stdout);
        break;
    case ':':
        for(int j=0; j<E.screenrows; j++)
        {
            printf("\r\n");
            fflush(stdout);
        }
        write(STDOUT_FILENO, "\x1b[K",3);
        write(STDOUT_FILENO, "==COMMAND MODE==", 15);
        write(STDOUT_FILENO, "\x1b[H", 3);
        command_mode();
        break;
    case 'r':
        read(0,&key,1);

        write(STDOUT_FILENO,&key,1);
        break;
    case 9://for tab key
        if(E.cx%4==0){
            write(STDOUT_FILENO,"\033[C",3);
            write(STDOUT_FILENO,"\033[C",3);
            write(STDOUT_FILENO,"\033[C",3);
            write(STDOUT_FILENO,"\033[C",3);
            E.cx=E.cx+4;
          //  printf("%d",E.cx);
            //fflush(stdout);
        }
        else if(E.cx%4==1){
            write(STDOUT_FILENO,"\033[C",3);
            write(STDOUT_FILENO,"\033[C",3);
            write(STDOUT_FILENO,"\033[C",3);
            E.cx=E.cx+3;
            //printf("%d",E.cx);
            //fflush(stdout);
        }
        else if(E.cx%4==2){
            write(STDOUT_FILENO,"\033[C",3);
            write(STDOUT_FILENO,"\033[C",3);
            E.cx=E.cx+2;
            //printf("%d",E.cx);
            //fflush(stdout);
        }
        else{
            write(STDOUT_FILENO,"\033[C",3);
        
        E.cx=E.cx+1;
        //printf("%d",E.cx);
        //fflush(stdout);
    }
    default:
        break;
    }
}
int main(int argc, char *argv[])
{
    signal(SIGWINCH, signalHandler);
    initEditor();
    if ( argc != 2 )
    {
      printf( "usage: %s filename", argv[0] );
    }
    else
    {
      editorRefreshScreen();
        FILE *file = fopen( argv[1], "r" );

        if ( file == 0 )
        {
            printf( "Could not open file\n" );
        }
        else
        {
            int x;
            enableRawMode();
            while  ( ( x = fgetc( file ) ) != EOF )
            {
                if(x=='\n')
                {
                    write(STDOUT_FILENO, "\r", 1);
                }
                printf("%c",x);
                fflush(stdout);
            }
            fclose( file );
            write(STDOUT_FILENO, "\x1b[H", 3);
            while(1)
            {
                editorProcessKeypress();
            }
        }
    }
    return 0;
}
