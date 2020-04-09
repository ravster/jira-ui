#include <stdio.h>
#include <stdlib.h>

int main(void) {
  int position = 0;
  char* buffer = malloc(sizeof(char) * 2048);
  int c;

  while (1) {
    c = getchar();

    if (c == EOF) {
      buffer[position] = '\0';
      printf("We just got\n%s", buffer);
      return 0;
    } else {
      buffer[position] = c;
    }

    position++;
  }
}
