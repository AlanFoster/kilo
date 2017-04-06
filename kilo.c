#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

void die(const char *s) {
  perror(s);
  exit(1);
}

int isFailure(int result) {
  return result == -1;
}

struct termios original_termios;
void saveCurrentTerminalMode() {
  if (isFailure(tcgetattr(STDIN_FILENO, &original_termios))) {
    die("saveCurrentTerminalMode failure");
  }
}

void restoreTerminalMode() {
  if (isFailure(tcgetattr(STDIN_FILENO, &original_termios))) {
    die("restoreTerminalMode failure");
  }
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

  if (isFailure(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw))) {
    die("enableRawMode failure");
  }
}

int main() {
  saveCurrentTerminalMode();
  enableRawMode();
  atexit(restoreTerminalMode);

  while (1) {
    char c = '\0';
    if (isFailure(read(STDIN_FILENO, &c, 1)) && errno != EAGAIN) {
      die("reading");
    }

    if (isprint(c)) {
      printf("%d ('%c')\r\n", c, c);
    } else { 
      printf("%d\r\n", c);
    }
 
    if (c == 'q') break;   
  }

  return 0;
}
