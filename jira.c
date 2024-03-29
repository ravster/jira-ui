// gcc -o jira -I/usr/include/x86_64-linux-gnu jira.c -lcurl -ljansson

#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <jansson.h>
#include <errno.h>

char *subdomain, *creds;

// Current issue.  Maybe make it a struct?
char *description, *summary, **g_comments, *issue_id;
size_t g_comment_length;
int in_loop = 1;

char* get_command(char* line, size_t len) {
  printf("> ");
  getline(&line, &len, stdin);
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

size_t append_to_string(void *body, size_t size, size_t num, void *orig) {
  size_t total = size * num;
  char* body1 = body;
  char** total_body_holder = orig;

  size_t new_size = total + 1 + strlen(*total_body_holder);

  /* We are doing a malloc and free here because realloc on *total_body_holder
     was not copying the old data over, and that would blow out the JSON string
     we are trying to build. */
  char* new_string = malloc(new_size);
  strcpy(new_string, "");
  strcat(new_string, *total_body_holder);
  strncat(new_string, body1, total);
  free(*total_body_holder);
  *total_body_holder = new_string;

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

  // comments0 is an array of [name, body] pairs.
  char **comments0 = malloc(2 * length * sizeof(char *));
  for (int i = 0; i < length; i++) {
    element = json_array_get(comments, i);
    json_t *author = json_object_get(element, "author");
    json_t *display_name = json_object_get(author, "displayName");
    char *display_name1 = strdup(json_string_value(display_name));
    json_t *body = json_object_get(element, "body");
    char *body1 = strdup(json_string_value(body));

    comments0[i * 2] = display_name1;
    comments0[i*2 + 1] = body1;
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
  if (!fields) {
    fprintf(stderr, "No fields in root object. json=%s\n", json);
    return "FAIL on json-parse";
  }

  //Description
  json_t *d1 = json_object_get(fields, "description");
  if (!fields) {
    fprintf(stderr, "No description in fields object. json=%s\n", json);
    return "FAIL on json-parse";
  }
  free(description);
  char* input_description = (char *)json_string_value(d1);
  if (input_description) {
    // Because JIRA sends over a null instead of an empty string, we have to
    // switch on the type.  Thanks JIRA.
    description = strdup(input_description);
  } else {
    description = (char*) malloc(5);
    strcpy(description, "");
  }
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
  /*
  This CURL pointer is static since I want it to persist past the lifetime of this function,
  because the POST CURL needs to be different from the GET CURL, since they have different
  options on them.
  */
  static CURL *curl;
  curl = curl_easy_init();

  char url[256];
  snprintf(url, 256, "https://%s.atlassian.net/rest/api/latest/issue/%s", subdomain, issue_id);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, append_to_string);
  curl_easy_setopt(curl, CURLOPT_USERPWD, creds);

  // This probably has to be a pointer to a string because the string itself
  // will be realloc'd to a bigger string as we get more information from
  // the internet.  But we can't just change that pointer because the handler
  // callback cannot change it.  Hence the second-layer of redirection.
  char* response = (char*)malloc(5);
  strcpy(response, "");
  char** response_holder = &response;
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)response_holder);

  printf("Making GET request to url=%s\n", url);
  CURLcode res;
  res = curl_easy_perform(curl);
  if(res != CURLE_OK)
    fprintf(stderr, "curl_easy_perform() failed: %s\n",
	    curl_easy_strerror(res));

  get_description(*response_holder);

  free(*response_holder);
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

char* replace_string(char *out, const char *in, char *pattern, char *replacement) {
  char *in2 = strdup(in);
  char *p = strtok(in2, pattern);
  if (p == NULL) {
    out = realloc(out, strlen(out) + strlen(in) + 1);
    if (!out) {
      exit(ENOMEM);
    }
    strcat(out, in);
    return(out);
  }

  do {
    out = realloc(out, 1 + strlen(out) + strlen(p) + strlen(replacement));
    if (!out) {
      exit(ENOMEM);
    }
    strcat(out, p);
    strcat(out, replacement);

    p = strtok(NULL, pattern);
  } while (p != NULL);

  return(out); // So the caller that made out, can then free the newest out
  // that 'out' has been realloc'd to.
}

void write_comment() {
  printf("Input your comment and end with 'eof' on a newline.\n");
  size_t len = 500, ret = 0;
  char* string = malloc(20); strcpy(string, "");
  char* line = malloc(20); strcpy(line, "");

  while(1) {
    ret = getline(&line, &len, stdin);
    if (0 == strncmp("eof", line, 3)) {
      break;
    }

    string = realloc(string, 1 + strlen(string) + ret);
    if (!string) {
      exit(ENOMEM);
    }
    strcat(string, line);
  }
  char* str_with_escaped_newlines = malloc(20); strcpy(str_with_escaped_newlines, "");
  str_with_escaped_newlines = replace_string(str_with_escaped_newlines, string, "\n", "\\n");
  char body[strlen(str_with_escaped_newlines) + 20];
  sprintf(body, "{\"body\":\"%s\"}", str_with_escaped_newlines);
  free(str_with_escaped_newlines);

  char url[256];
  printf("in wc, issue-id='%s'\n", issue_id);
  snprintf(url, 256, "https://%s.atlassian.net/rest/api/2/issue/%s/comment", subdomain, issue_id);
  printf("posting body=%s to url=%s\n", body, url);

  /* This CURL pointer is static too, since we don't share the object with the GET requests,
     and we want to reuse the same object through the life of the program to take advantage of
     CURLs HTTP keepalive functionality.

     It almost certainly doesn't matter for this program, but it's good practice, and it doesn't
     harm anything in this program for the very same reason, that this is a very open-and-shut
     type of program. */
  static CURL *curl;
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_USERPWD, creds);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  if (headers == NULL) {
    fprintf(stderr, "Couldn't add content-type json when adding a comment");
    free(string);
  }
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, append_to_string);
  char* response;
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
  char errbuf[CURL_ERROR_SIZE];
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
  CURLcode res;
  res = curl_easy_perform(curl);
  printf("err=%s, res=%d\n", errbuf, res);
  if (res != CURLE_OK) {
    fprintf(stderr, "Could not post comment: %s\n", curl_easy_strerror(res));
  }
  printf("Response=%s\n", response);
  curl_slist_free_all(headers);
  free(string);
  free(response);
}

void exit_loop() {
  in_loop = 0;
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
  } else if (0 == strncmp("q", in, 1)) {
    exit_loop();
  } else {
    printf("Didn't understand command:%s:\n", in);
  }
}

int main(void) {
  char **config = malloc(sizeof(char*) * 2);
  get_config(config);
  creds = strdup(config[0]);
  subdomain = config[1];

  curl_global_init(CURL_GLOBAL_DEFAULT);

  while(in_loop == 1) {
    char a2[500] = "";
    get_command(&a2[0], 500);
    eval_command(a2);
  }

  curl_global_cleanup();
  free(creds);
  printf("Exiting...\n");

  return 0;
}
