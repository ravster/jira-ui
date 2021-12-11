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
  for(int i = 0; i<count; i++) {
    free(arr[i]);
  }
  //free(arr);
  free(c);
}

int v1() {
  int count = 5;
  char** arr = calloc(count, sizeof(char*));
  arr[0] = strdup("foo");
  arr[1] = strdup("bar");
  arr[2] = strdup("baz");
  arr[3] = strdup("qux");
  arr[4] = strdup("quux");
  char* c = merge(count, arr);
  printf("c=%s\n", c);
  for(int i = 0; i<count; i++) {
    free(arr[i]);
  }
  free(arr);
  free(c);
  return 0;
}
