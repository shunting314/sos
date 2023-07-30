/*
Example command:
  cat shell.cpp | awk '{print NF}' | sort | uniq -c | sort -nr
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DEBUG 0
#if DEBUG
#define debug(...) printf(__VA_ARGS__)
#else
#define debug(...)
#endif

#define CHECK_STATUS(status) \
  if (status != 0) { \
    printf("Error happens: status %d, errno %d, err string: %s\n", status, errno, strerror(errno)); \
    return status; \
  }

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

/*
 * Run a single command with the specified stdin and out
 */
int run_single_command(char** words, int nword, int current_in, int current_out) {
  int childpid;

	childpid = fork();
	if (childpid < 0) {
		assert(false && "fail to fork");
	}
	if (childpid == 0) {
		// in child process
    if (current_in != 0) {
      dup2(current_in, 0);
      close(current_in);
    }
    if (current_out != 1) {
      dup2(current_out, 1);
      close(current_out);
    }
    // printf("execvp '%s'\n", words[0]);
		execvp(words[0], words);
		assert(false && "fail to run exec");
	}

  // close curernt_in/current_out in the parent process
  if (current_in != 0) {
    close(current_in);
  }
  if (current_out != 1) {
    close(current_out);
  }
  return childpid;
}

int execute_command(char** words, int nword) {
  int redirect_in = 0;
  int redirect_out = 1;
  int remaining_in;
  int current_in, current_out;

	assert(nword > 0);

  // TODO only support '>' as a standalone word for now.
  // '>/tmp/out' as a single word is not support currently.
  char* outfile = nullptr;
  char* infile = nullptr;
  while (nword >= 3) {
    if (strcmp(words[nword - 2], ">") == 0) {
      outfile = words[nword - 1];
      nword -= 2;
    } else if (strcmp(words[nword - 2], "<") == 0) {
      infile = words[nword - 1];
      nword -= 2;
    } else {
      break;
    }
  }
  words[nword] = nullptr;

  if (infile) {
    redirect_in = open(infile, O_RDONLY);
    if (redirect_in < 0) {
      printf("Fail to open in file '%s'\n", infile);
      return -1;
    }
  }

  if (outfile) {
    redirect_out = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (redirect_out < 0) {
      printf("Fail to open out file '%s'\n", outfile);
      if (redirect_in != 0) {
        close(redirect_in);
      }
      return -1;
    }
  }

  remaining_in = redirect_in;

  int start_pos = 0;
  int last_child = -1;
  assert(nword > 0);
  while (start_pos < nword) {
    // find the next pipe
    int pipe_pos = start_pos;
    for (; pipe_pos < nword && strcmp(words[pipe_pos], "|") != 0; ++pipe_pos) {
    }
    if (pipe_pos == nword) {
      // no more pipe
      current_in = remaining_in;
      current_out = redirect_out;
    } else {
      // found a pipe
      int pipe_fds[2];
      CHECK_STATUS(pipe(pipe_fds));
      current_in = remaining_in;
      current_out = pipe_fds[1];

      remaining_in = pipe_fds[0];
    }

    // start child process for words between [start_pos, pipe_pos)
    // with current_in/current_out as stdin/out
    words[pipe_pos] = nullptr;
    last_child = run_single_command(words + start_pos, pipe_pos - start_pos, current_in, current_out);

    start_pos = pipe_pos + 1;
  }

  assert(last_child >= 0);

  // wait for the last child
	int wstatus, exit_status;
	waitpid(last_child, &wstatus, 0);
	exit_status = WEXITSTATUS(wstatus);
	if (exit_status != 0) {
		printf("Fail to run command, exit status %d\n", exit_status);
    return -1;
	}
  return 0;
}

typedef int builtin_fn_t(char** words, int nword);
struct builtin_entry {
  const char* name;
  builtin_fn_t* fn_ptr;
};

int builtin_cd(char** words, int nword) {
  int status;
  if (nword != 1) {
    printf("Only support a single argument right now\n");
    return -1;
  }
  const char* new_path = words[0];
  assert(new_path[0]);
  if (new_path[0] == '/') {
    // absolute directory
    status = chdir(new_path);
    CHECK_STATUS(status);
  } else {
    char abs_path[1024];
    if (!getcwd(abs_path, sizeof(abs_path))) {
      printf("Change director to '%s' failed\n", abs_path);
      return -1;
    }

    strcat(abs_path, "/");
    strcat(abs_path, new_path); // TODO avoid overflow
    debug("Destination directory %s\n", abs_path);
    status = chdir(abs_path);
    CHECK_STATUS(status);
  }
  return 0;
}

builtin_entry _builtin_list[] = {
  // {"pwd", builtin_pwd},  // pwd is not necessary to be a builtin since child process inherit the current working directory of the parent process
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

/* 
 * Core features needed from the operation system:
 * 1. fork & exec
 * 2. dup2 syscall
 * 3. pipe syscall
 */
int main(void) {
	char line[512];
	char* words[64];
	int nword;

	while (true) {
		printf("> ");
		if (!fgets(line, sizeof(line), stdin)) {
			break; // EOF
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

    builtin_fn_t* builtin_fn_ptr = find_builtin(words[0]);
    if (builtin_fn_ptr) {
      // skip the first word which is the builtin name
      builtin_fn_ptr(words + 1, nword - 1);
      continue;
    }
		execute_command(words, nword);
	}
	debug("\nbye\n");
	return 0;
}
