#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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


struct outputBuffer {
  char *start;
  int length;
};

#define EMPTY_OUTPUT_BUFFER {NULL, 0}

void appendBuffer(struct outputBuffer *output_buffer, const char *s, int length) {
  char *new_start = realloc(output_buffer->start, output_buffer->length + length);

  if (new_start == NULL) return;
  memcpy(&new_start[output_buffer->length], s, length);
  output_buffer->start = new_start;
  output_buffer->length += length;
}

void clearBuffer(struct outputBuffer *output_buffer) {
  free(output_buffer->start);
}

void clearScreen() {
  // Clear Screen
  write(STDOUT_FILENO, "\x1b[2J", 4);
  // Place cursor top left
  write(STDOUT_FILENO, "\x1b[H", 3);
}

void clearScreenWithBuffer(struct outputBuffer *output_buffer) {
  // Clear Screen
  appendBuffer(output_buffer, "\x1b[2J", 4);
  // Place cursor top left
  appendBuffer(output_buffer, "\x1b[H", 3);
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

  if (isFailure(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)) || ws.ws_col == 0) {
    return -1;
  }

  *cols = ws.ws_col;
  *rows = ws.ws_row;
  return 0;
}

void drawRows(struct outputBuffer *output_buffer) {
  int y;
  for (y = 0; y < editor_config.screen_rows; y++) {
    appendBuffer(output_buffer, "\x1b[31m", 5);

    if (y < editor_config.screen_rows - 1) {
      appendBuffer(output_buffer, "~\r\n", 3);
    }
  }
}

void refreshScreen() {
  struct outputBuffer output_buffer = EMPTY_OUTPUT_BUFFER;

  clearScreenWithBuffer(&output_buffer);
  drawRows(&output_buffer);
  appendBuffer(&output_buffer, "\x1b[H", 3);

  write(STDOUT_FILENO, output_buffer.start, output_buffer.length);

  clearBuffer(&output_buffer);
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

