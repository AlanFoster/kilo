#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define isFailure(result) (result == -1)
#define hasInput(result) (result == 1)
#define CTRL_KEY(key) ((key) & 0x1f)

void clearScreen() {
  // Clear Screen
  write(STDOUT_FILENO, "\x1b[2J", 4);
  // Place cursor top left
  write(STDOUT_FILENO, "\x1b[H", 3);
}

void die(const char *s) {
  clearScreen();
  perror(s);
  exit(1);
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

void drawRows() {
  int y;
  for (y = 0; y < 80; y++) {
    write(STDOUT_FILENO, "~\r\n", 3);
  }
}

void refreshScreen() {
  clearScreen();
  drawRows();
  write(STDOUT_FILENO, "\x1b[H", 3);
}

char readKey() {
  int readStatusCode;
  char c;
  do {
    readStatusCode = read(STDIN_FILENO, &c, 1);
    if (isFailure(readStatusCode) && errno != EAGAIN) {
      die("readKey failure");
    }
  } while (!hasInput(readStatusCode));
  
  return c;
}

void handleKeypress(char key) {
  switch (key) {
    case CTRL_KEY('q'):
     clearScreen(); 
     exit(0);
     break;
  }
}

int main() {
  saveCurrentTerminalMode();
  enableRawMode();
  atexit(restoreTerminalMode);

  while (1) {
    refreshScreen();
    handleKeypress(readKey());
  }

  return 0;
}

