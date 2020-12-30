// gcc -o jira -I/usr/include/x86_64-linux-gnu jira.c -lcurl -ljansson

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>
#include <errno.h>

// Growable string.
// This could be in a seperate library.
struct growable_string {
  char *memory;
  size_t length, capacity;
};
void growable_string_new(struct growable_string *out) {
  size_t capacity = 20481;
  char *data = malloc(capacity);
  data[0] = '\0';

  out->memory = data;
  out->length = 0;
  out->capacity = capacity;
}

int growable_string_append(struct growable_string *string, const char *in, size_t new_amount) {
  if (string->capacity < string->length + new_amount + 1) {
    // grow string
    size_t new_cap = (new_amount * 2) + string->capacity;
    char *new_mem = realloc(string->memory, new_cap);
    if (new_mem == NULL) {
      fprintf(stderr, "Couldn't get %lu bytes of memory to grow a string.\n", new_cap);
      exit(3);
    }
    string->memory = new_mem;
    string->capacity = new_cap;
  }

  strncat(string->memory, in, new_amount);
  string->length += new_amount;

  return 1;
}

CURL *curl;
char *subdomain;

// Current issue.  Maybe make it a struct?
char *description, *summary, **g_comments, *issue_id;
size_t g_comment_length;

char* get_command() {
  size_t len = 500;
  char* line = malloc(len);

  printf("> ");
  int ret = getline(&line, &len, stdin);
  if (ret == -1) {
    printf("GOT return value %d.  errno = %d. %s.\n", ret, errno, line);
    perror("Can't read from stdin! wat?\n");
    exit(errno);
  }
  return line;
}

void remove_trailing_newline(char *in) {
  char *newline = strchr(in, '\n');
  newline[0] = 0;
}

void get_config(char** out) {
  FILE* credfile = fopen("creds", "r");
  char *creds, *subdomain;
  size_t length = 0;
  int err = getline(&creds, &length, credfile);
  length = 0;
  err = getline(&subdomain, &length, credfile);
  if (err == -1) {
    fprintf(stderr, "Couldn't read from credfile\n");
    exit(-1);
  }
  remove_trailing_newline(creds);
  remove_trailing_newline(subdomain);

  out[0] = creds;
  out[1] = subdomain;
}

size_t append_growable_string(void *body, size_t size, size_t num, void *store) {
  size_t total = size * num;
  struct growable_string *store1 = store;

  if (store1->length + total > store1->capacity) {
    size_t new_capacity = (total * 2) + store1->capacity;
    char *new_mem = realloc(store1->memory, new_capacity);
    if (new_mem == NULL) {
      fprintf(stderr, "Couldn't get %lu bytes of memory for saving the response.\n", new_capacity);
      exit(3);
    }
    store1->memory = new_mem;
    store1->capacity = new_capacity;
  }

  strcat(store1->memory, body);
  store1->length += total;

  return total;
}

int extract_comments(json_t *fields) {
  free(g_comments); // OK, so this isn't enough, and we need to clean out comment-memory completely.
  json_t *comment = json_object_get(fields, "comment");
  json_t *comments = json_object_get(comment, "comments");
  json_t *element;
  size_t length = json_array_size(comments);
  if (length == 0) {
    fprintf(stderr, "Comments are not an array, or there are no comments.\n");
    return 0;
  }

  char **comments0 = malloc(2 * length * sizeof(char *));
  for (int i = 0; i < length; i = i + 2) {
    element = json_array_get(comments, i);
    json_t *author = json_object_get(element, "author");
    json_t *display_name = json_object_get(author, "displayName");
    char *display_name1 = strdup(json_string_value(display_name));
    json_t *body = json_object_get(element, "body");
    char *body1 = strdup(json_string_value(body));

    comments0[i] = display_name1;
    comments0[i + 1] = body1;
  }
  g_comments = comments0;
  g_comment_length = length;

  json_decref(comments);
  json_decref(comment);

  return 1;
}

char *get_description(char *json) {
  json_t *root;
  json_error_t error;

  root = json_loads(json, 0, &error);
  if(!root) {
    fprintf(stderr, "Can't parse json: Line %d : %s\n", error.line, error.text);
    return "FAIL on json-parse";
  }

  json_t *fields = json_object_get(root, "fields");

  //Description
  json_t *d1 = json_object_get(fields, "description");
  free(description);
  description = strdup(json_string_value(d1));
  json_decref(d1);

  // Summary
  d1 = json_object_get(fields, "summary");
  free(summary);
  summary = strdup(json_string_value(d1));
  json_decref(d1);

  extract_comments(fields);

  return description;
}

void get_issue() {
  CURLcode res;
  char url[256];
  snprintf(url, 256, "https://%s.atlassian.net/rest/api/latest/issue/%s", subdomain, issue_id);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, append_growable_string);
  struct growable_string response;
  response.memory = malloc(20480); // 20KB.  Each "g" call usually pulls in 11-12KB.
  response.memory[0] = '\0'; // Set first char to '\0' so that it's a valid empty string.
  response.length = 0;
  response.capacity = 20479;
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

  res = curl_easy_perform(curl);
  if(res != CURLE_OK)
    fprintf(stderr, "curl_easy_perform() failed: %s\n",
	    curl_easy_strerror(res));

  printf("Size of get_issue = %lu bytes\n", response.length);

  get_description(response.memory);

  free(response.memory);
  response.capacity = 0;
}

void print_comments() {
  printf("COMMENTS -----\n");
  for (int i = 0; i < g_comment_length; i = i+2) {
    printf("%s: %s\n", g_comments[i], g_comments[i + 1]);
  }
}

void write_to_file(char *filename, char *body) {
  FILE *f = fopen(filename, "w");

  fwrite(body, strlen(body), 1, f);
  fclose(f);
}

void replace_string(struct growable_string *out, const char *in, char *pattern, char *replacement) {
  char *in2 = strdup(in);
  char *p = strtok(in2, pattern);
  if (p == NULL) {
    growable_string_append(out, in, out->length + strlen(in));
    return;
  }

  do {
    growable_string_append(out, p, out->length + strlen(p));
    growable_string_append(out, replacement, out->length + strlen(replacement));
  } while (p != NULL);
}

void write_comment() {
  printf("Input your comment and end with 'eof' on a newline.\n");
  char *line = NULL;
  size_t len = 500, ret = 0;
  struct growable_string string = {};
  growable_string_new(&string);

  while(1) {
    line = malloc(len);
    ret = getline(&line, &len, stdin);
    if (0 == strncmp("eof", line, 3)) {
      break;
    }

    growable_string_append(&string, line, ret);
    free(line);
  }
  struct growable_string foo;
  growable_string_new(&foo);
  replace_string(&foo, string.memory, "\n", "\\n");
  printf("foo is: %sNNN\n", foo.memory);
  char body[foo.length + 20];
  sprintf(body, "{\"body\":\"%s\"}", foo.memory);
  printf("body is: %sNNN\n", body);
  return;

  CURLcode res;
  char url[256];
  printf("in wc, issue-id is '%s'\n", issue_id);
  snprintf(url, 256, "https://%s.atlassian.net/rest/api/2/issue/%s/comment", subdomain, issue_id);
  printf("posting <%s> to %s\n", body, url);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Content-Type:application/json");
  if (headers == NULL) {
    fprintf(stderr, "Couldn't add content-type json when adding a comment");
    free(string.memory);
  }
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, append_growable_string);
  struct growable_string response;
  growable_string_new(&response);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

  char errbuf[CURL_ERROR_SIZE];
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
  res = curl_easy_perform(curl);
  printf("err %s, res = %d\n", errbuf, res);
  if (res != CURLE_OK) {
    fprintf(stderr, "Could not post comment: %s\n", curl_easy_strerror(res));
  }
  printf("Response is: %s\n", response.memory);

  free(string.memory);
}

void eval_command(char *in) {
  if (0 == strncmp("g ", in, 2)) {
    // If first 2 chars are "g ".
    remove_trailing_newline(in);
    strtok(in, " "); // To get rid of the leading "g".
    char *id = strtok(NULL, " ");
    if (id == NULL) {
      fprintf(stderr, "The 'g' command requires an issue-id after it.\n");
      return;
    }
    printf("getting issue %s\n", id);
    free(issue_id);
    issue_id = strdup(id);
    printf("issue_id is '%s'\nid is '%s'\n", issue_id, id);

    get_issue();
  } else if (0 == strncmp("d", in, 1)) {
    printf("Description:\n%s\n\n", description);
  } else if (0 == strncmp("s", in, 1)) {
    printf("Summary:\n%s\n\n", summary);
  } else if (0 == strncmp("c", in, 1)) {
    print_comments();
  } else if (0 == strncmp("mc", in, 2)) {
    write_comment();
  } else {
    printf("Didn't understand command:%s:\n", in);
  }
}

int main(void) {
  char **config = malloc(sizeof(char*) * 2);
  get_config(config);
  char *creds = config[0];
  subdomain = config[1];

  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();
  if (!curl) {
    fprintf(stderr, "Couldn't create a CURL instance\n");
    exit(2);
  }
  curl_easy_setopt(curl, CURLOPT_USERPWD, creds);

  char* a1;
  while(1) {
    a1 = get_command();
    eval_command(a1);
    free(a1);
  }

  curl_easy_cleanup(curl);
  curl_global_cleanup();
  free(creds);

  return 0;
}
