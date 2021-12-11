//  gcc -Wall -g -o concat_strings concat_strings.c && time ./concat_strings

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char* merge(int count, char** arr) {
  char* new = calloc(5000, sizeof(char));
  strcat(new, "");
  for(int i = 0; i < count; i++) {
    char* el = arr[i];
    printf("i=%d, new=%s, arr[i]=%s\n---\n",
	   i, new, el);
    strcat(new, el);
  }
  return new;
}

int main() {
  int count = 5;
  char* arr[] = {"a", "b", "c", "d", "e"};
  char* c = merge(count, arr);
  printf("c=%s\n", c);
  free(c);
  return 0;
}
