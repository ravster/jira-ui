//  gcc -Wall -g -o concat_strings concat_strings.c && time ./concat_strings

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Ruby String#+
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

// Returns a NULL terminated list of strings.
char** str_split(const char* in, const char* delim) {
  char** out = calloc(100, sizeof(char*));
  char* in2 = strdup(in);
  int count = 0;

  char* el = strtok(in2, delim);
  while(el != NULL) {
    printf("Count is %d\n", count);
    out[count] = strdup(el);
    count++;
    el = strtok(NULL, delim);
  }
  free(in2);
  out[count] = NULL;

  return out;
}

// Ruby String#<<
char* str_append(char* in1, char* in2) {
  int outlen = strlen(in1) + strlen(in2) + 1;
  char* out = calloc(outlen, sizeof(char));
  strcat(out, in1);
  strcat(out, in2);
  return out;
}

typedef struct {
  int capacity;
  int length;
  char* string;
} r_string;
r_string* r_string_append(r_string* out, const char* in) {
  int inlen = strlen(in);
  int newlen = 1 + out->length + inlen;
  if (newlen > out->capacity) {
    out->string = realloc(out->string, newlen * 3);
    // 3 is magic num.  Pulled out of thin air.
    out->capacity = newlen * 3;
  }
  strcat(out->string, in);
  out->length = newlen;
  return out;
}
r_string* r_string_new(const char* in) {
  r_string* out = malloc(sizeof(r_string));
  out->capacity = 500; // magic num
  out->length = 0;
  out->string = calloc(out->capacity, sizeof(char));
  out->string[0] = 0;

  return r_string_append(out, in);
}
void r_string_free(r_string* it) {
  free(it->string);
  free(it);
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

  char** split = str_split("foo,bar,baz,qux", ",");
  printf("split=\n");
  for(int i = 0; i < 100; i++) {
    char* el = split[i];
    if (el == NULL) {
      break;
    }
    printf("  %s\n", el);
    free(el);
  }
  free(split);

  char* append = str_append("foo", " bar");
  printf("append='%s'\n", append);
  free(append);

  char* arr2[] = {"foo", "bar", "baz", "qux", 0};
  r_string* lis = r_string_new("");
  char* foo = calloc(30, sizeof(char)); foo[0] = 0;
  for(int i = 0; ; i++) { // magic num
    char* el = arr2[i];
    if (el == 0) { break; }
    sprintf(foo, "<li>%s</li>\n", el);
    r_string_append(lis, foo);
  }
  free(foo);
  printf("<ul>\n%s</ul>", lis->string);
  r_string_free(lis);

  return 0;
}
