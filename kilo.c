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
  raw.c_lflag &= ~(ECHO | ICANON);

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() {
  saveCurrentTerminalMode();
  enableRawMode();
  atexit(restoreTerminalMode);

  char c;

  while(read(STDIN_FILENO, &c, 1) == 1 && c != 'q');

  return 0;
}
