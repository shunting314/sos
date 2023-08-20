#include <stdio.h>
#include <syscall.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#define DEBUG 0
#if DEBUG
#define debug(...) printf(__VA_ARGS__)
#else
#define debug(...)
#endif

/*
 * 'line' may be mutated.
 * Have basic support of quotation.
 */
void parse_line(char* line, char** words, int capacity, int& nword) {
	nword = 0;
	char* cur = line;
	char* end = line + strlen(line);
	char* next;
  char start_quote = 0;

	while (cur < end) {
		// skip spaces
		while (cur != end && isspace(*cur)) {
			++cur;
		}
		if (cur == end) {
			break;
		}

    start_quote = 0;
    if (*cur == '\'' || *cur == '"') {
      start_quote = *cur;
      ++cur;
    }
		next = cur;
		while (next != end && (!start_quote && !isspace(*next) || start_quote && *next != start_quote)) {
			++next;
		}
		*next = '\0';
		assert(nword < capacity);
		words[nword++] = cur;
		cur = next + 1;
	}
}

int main(void) {
  char line[512];
  char* words[64];
  int nword;

  printf("Enter the main function of shell\n");
  while (true) {
    printf("$ ");
    if (!fgets(line, sizeof(line), 0)) {
      break;
    }
    debug("Got line %s\n", line);
		parse_line(line, words, sizeof(words) / sizeof(*words), nword);

		debug("Got %d words\n", nword);
		for (int i = 0; i < nword; ++i) {
			debug(" - %s\n", words[i]);
		}

    assert(nword > 0);
		assert(nword < sizeof(words) / sizeof(*words));
		words[nword] = nullptr;

    int child_pid = spawn(words[0]);
    if (child_pid < 0) {
      printf("Fail to spawn %s\n", words[0]);
      continue;
    }
    int child_status;
    int rc;
    rc = waitpid(child_pid, &child_status, 0);
    assert(rc == child_pid);
  }
  printf("bye\n");
  return 0;
}
