//  gcc -Wall -g -o concat_strings concat_strings.c && time ./concat_strings

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char* merge(int count, char** arr) {
  char* new = calloc(5000, sizeof(char));
  strcat(new, "");
  for(int i = 0; i < count; i++) {
    char* el = arr[i];
    strcat(new, el);
  }
  return new;
}

char* str_join(int count, char** input, char* sep) {
  int sep_length = strlen(sep);
  int tot_length = 0;
  for(int i = 0; i < count; i++) {
    tot_length += strlen(input[i]) + sep_length;
  }
  char* out = calloc(tot_length, sizeof(char));
  strcat(out, "");
  for(int i = 0; i<count; i++) {
    char* el = input[i];
    strcat(out, el);
    if (i < (count-1)) {
      strcat(out, sep);
    }
  }
  return(out);
}

int main() {
  int count = 5;
  char* arr[] = {"a", "b", "c", "d", "e"};
  char* c = merge(count, arr);
  char* d = str_join(count, arr, " and ");
  printf("c=%s\n", c);
  printf("d=%s\n", d);
  free(c);
  free(d);
  return 0;
}
