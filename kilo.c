#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#define isFailure(result) (result == -1)
#define hasInput(result) (result == 1)
#define CTRL_KEY(key) ((key) & 0x1f)

struct editorConfig {
  int screen_rows;
  int screen_cols;

  struct termios initial_termios;
};

struct editorConfig editor_config;

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

void saveCurrentTerminalMode() {
  if (isFailure(tcgetattr(STDIN_FILENO, &editor_config.initial_termios))) {
    die("saveCurrentTerminalMode failure");
  }
}

void restoreTerminalMode() {
  if (isFailure(tcgetattr(STDIN_FILENO, &editor_config.initial_termios))) {
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

int getWindowSize(int *rows, int *cols) {
  struct winsize ws;
  
  if (isFailure(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) || ws.ws_col == 0)) {
    return -1;
  }

  *cols = ws.ws_col;
  *rows = ws.ws_row;
  return 0;
}

void drawRows() {
  int y;
  for (y = 0; y < editor_config.screen_rows; y++) {
    write(STDOUT_FILENO, "\x1b[31m", 5);
    
    if (y < editor_config.screen_rows - 1) {
      write(STDOUT_FILENO, "~\r\n", 3);
    }
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

void initEditor() {
  if (isFailure(getWindowSize(&editor_config.screen_rows, &editor_config.screen_cols))) {
    die("initEditor");
  }
}

int main() {
  initEditor();

  saveCurrentTerminalMode();
  enableRawMode();
  atexit(restoreTerminalMode);

  while (1) {
    refreshScreen();
    handleKeypress(readKey());
  }

  return 0;
}

