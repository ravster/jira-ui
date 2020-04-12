// gcc -o jira -I/usr/include/x86_64-linux-gnu jira.c -lcurl -ljansson

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>

struct incoming_bytes {
  char *memory;
  size_t size;
};

CURL *curl;
char *subdomain;

// Current issue.  Maybe make it a struct?
char *description, *summary;

char* get_command() {
  char* line = NULL;
  size_t len;

  printf("> ");
  getline(&line, &len, stdin);
  printf("We just got\n%s", line);
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

size_t save_response(void *body, size_t size, size_t num, void *store) {
  size_t total = size * num;
  struct incoming_bytes *store1 = store;

  char *new_mem = realloc(store1->memory, store1->size + 1 + total);
  if (new_mem == NULL) {
    fprintf(stderr, "Couldn't get %lu bytes of memory for saving the response.\n", store1->size + 1 + total);
    exit(3);
  }

  store1->memory = new_mem;
  memcpy(&(store1->memory[store1->size]), body, total);
  store1->size += total;
  store1->memory[store1->size] = 0;

  return total;
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
  json_t *d1 = json_object_get(fields, "description");
  free(description);
  description = strdup(json_string_value(d1));
  return description;
}

void get_issue(char *issue_id) {
  CURLcode res;
  char url[256];
  snprintf(url, 256, "https://%s.atlassian.net/rest/api/latest/issue/%s", subdomain, issue_id);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, save_response);
  struct incoming_bytes response;
  response.memory = malloc(1);
  response.size = 0;
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

  res = curl_easy_perform(curl);
  if(res != CURLE_OK)
    fprintf(stderr, "curl_easy_perform() failed: %s\n",
	    curl_easy_strerror(res));

  printf("Size of get_issue = %lu bytes\n", response.size);

  get_description(response.memory);

  free(response.memory);
  response.size = 0;
}

void eval_command(char *in) {
  if (0 == strncmp("g ", in, 2)) {
    // If first 2 chars are "g ".
    remove_trailing_newline(in);
    strtok(in, " "); // To get rid of the leading "g".
    char *issue_id = strtok(NULL, " ");
    if (issue_id == NULL) {
      fprintf(stderr, "The 'g' command requires an issue-id after it.\n");
      return;
    }
    printf("getting issue %s\n", issue_id);

    get_issue(issue_id);
  } else if (0 == strncmp("d", in, 1)) {
    printf("Description:\n%s\n\n", description);
  }
}

int main(void) {
  char **config = malloc(sizeof(char*) * 2);
  get_config(config);
  char *creds = config[0];
  subdomain = config[1];

  CURLcode res;
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
