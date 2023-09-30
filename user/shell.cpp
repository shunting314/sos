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

typedef int builtin_fn_t(char** words, int nword);
struct builtin_entry {
  const char* name;
  builtin_fn_t* fn_ptr;
};

/*
 * TODO: don't have the concept of home directory yet. So 'cd' will enter
 * '/' if no arguments are provided.
 */
int builtin_cd(char **words, int nword) {
  const char *path = "/";
  if (nword > 0) {
    path = words[0];
  }
  int r = chdir(path);
  if (r < 0) {
    printf("cd fails\n");
    return -1;
  }
  return 0;
}

int builtin_pwd(char **words, int nword) {
  char path[1024];
  auto r = getcwd(path, sizeof(path));
  if (!r) {
    printf("Failed to run getcwd\n");
    return -1;
  }
  printf("%s\n", r);
  return 0;
}

builtin_entry _builtin_list[] = {
  {"pwd", builtin_pwd},  // pwd is not required to be a builtin since child process inherit the current working directory of the parent process
  {"cd", builtin_cd},
  {nullptr, nullptr},
};

builtin_fn_t* find_builtin(const char* name) {
  for (int i = 0; _builtin_list[i].name; ++i) {
    if (strcmp(name, _builtin_list[i].name) == 0) {
      return _builtin_list[i].fn_ptr;
    }
  }
  return nullptr;
}

int main(int argc, char** argv) {
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

    // empty line
    if (nword == 0) {
      continue;
    }

    assert(nword > 0);
		assert(nword < sizeof(words) / sizeof(*words));
		words[nword] = nullptr;

    builtin_fn_t* builtin_fn_ptr = find_builtin(words[0]);
    if (builtin_fn_ptr) {
      // skip the first word which is the builtin name
      builtin_fn_ptr(words + 1, nword - 1);
      continue;
    }

    int child_pid = spawn(words[0], (const char**) words);
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
