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
 */
void parse_line(char* line, char** words, int capacity, int& nword) {
	nword = 0;
	char* cur = line;
	char* end = line + strlen(line);
	char* next;
	while (cur < end) {
		// skip spaces
		while (cur != end && isspace(*cur)) {
			++cur;
		}
		if (cur == end) {
			break;
		}

		next = cur;
		while (next != end && !isspace(*next)) {
			++next;
		}
		*next = '\0';
		assert(nword < capacity);
		words[nword++] = cur;
		cur = next + 1;
	}
}

void execute_command(char** words, int nword) {
  int redirect_in = 0;
  int redirect_out = 1;
  int current_in, current_out;

  int childpid, exit_status;

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
      goto out;
    }
  }

  if (outfile) {
    redirect_out = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (redirect_out < 0) {
      printf("Fail to open out file '%s'\n", outfile);
      goto out;
    }
  }

  // TODO revise for pipe
  current_in = redirect_in;
  current_out = redirect_out;

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
	int wstatus;
	waitpid(childpid, &wstatus, 0);
	exit_status = WEXITSTATUS(wstatus);
	if (exit_status != 0) {
		printf("Fail to run command, exit status %d\n", exit_status);
	}

out:
  if (redirect_in != 0) {
    close(redirect_in);
  }
  if (redirect_out != 1) {
    close(redirect_out);
  }
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

		// TODO support pipe
		// TODO support chaining multiple pipes
	}
	debug("\nbye\n");
	return 0;
}
