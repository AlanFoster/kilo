#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

struct termios original_termios;
void saveCurrentTerminalMode() {
  tcgetattr(STDIN_FILENO, &original_termios);
}

void restoreTerminalMode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}

/*
 * By default terminals read via 'canonical' mode
 * Preference raw mode for direct character stream manipulation
 */
void enableRawMode() {
  struct termios raw;
  tcgetattr(STDIN_FILENO, &raw);
 
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON); 
  raw.c_iflag &= ~(ICRNL | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() {
  saveCurrentTerminalMode();
  enableRawMode();
  atexit(restoreTerminalMode);

  while (1) {
    char c = '\0';
    read(STDIN_FILENO, &c, 1);
    if (isprint(c)) {
      printf("%d ('%c')\r\n", c, c);
    } else { 
      printf("%d\r\n", c);
    }
 
    if (c == 'q') break;   
  }

  return 0;
}
