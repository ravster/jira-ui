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

char* str_mult(int count, char* in) {
  char* out = calloc((count+1)*strlen(in), sizeof(char));
  out[0] = 0; // Ensure the first byte is a \0, so it's a well-behaved string.

  for(int i = 0; i<count; i++) {
    strcat(out, in);
  }
  return(out);
}

char* str_center(char* input, int width, char* pad) {
  int len1 = strlen(input);
  int extra = width - len1;
  int half = extra/2;
  if ((len1 >= width) || (half < 1)) {
    char* out = strdup(input);
    return out;
  }

  char * pad2 = str_mult(half, pad);
  char* pad3 = strndup(pad2, half);
  free(pad2);
  char* arr[3] = {
    pad3,
    input,
    pad3
  };
  char* out = merge(3, arr);
  free(pad3);

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

  char* mult = str_mult(77, "foo");
  printf("mult=%s\n", mult);
  free(mult);

  char* center = str_center("foo", 20, ",");
  printf("center='%s'\n", center);
  free(center);
  return 0;
}
