/*
 * Example command for Host OS:
 *   cat ../../user/shell.cpp | awk '{print NF}' | sort | uniq -c | sort -nr
 *
 * Core features needed from the operation system:
 * 1. fork & exec (or spawn)
 * 2. dup2 syscall (or fused into spawn)
 * 3. pipe syscall
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#ifdef HOST_OS
#include <unistd.h>
#include <errno.h>
#else
#include <syscall.h>
#endif

// TODO support user malloc by calling setup_malloc
#define SUPPORT_USER_MALLOC 0
#if SUPPORT_USER_MALLOC
#include <vector.h>
#endif

#define DEBUG 0
#if DEBUG
#define debug(...) printf(__VA_ARGS__)
#else
#define debug(...)
#endif

#ifdef HOST_OS
#define CHECK_STATUS(status) \
  if (status != 0) { \
    printf("Error happens: status %d, errno %d, err string: %s\n", status, errno, strerror(errno)); \
    return status; \
  }
#else
#define CHECK_STATUS(status) \
  if (status != 0) { \
    printf("Error happens: status %d\n", status); \
    return status; \
  }
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

#ifdef HOST_OS
int spawn(char* prog, const char **words, int current_in, int current_out) {
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
		execvp(prog, (char**) words);
		assert(false && "fail to run exec");
	}
  return childpid;
}
#endif

/*
 * Run a single command with the specified stdin and out
 */
int run_single_command(char** words, int nword, int current_in, int current_out) {
  int child_pid = spawn(words[0], (const char**) words, current_in, current_out);
  if (child_pid < 0) {
    printf("Fail to spawn %s\n", words[0]);
    return -1;
  }
  return child_pid;
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
    #if HOST_OS
    redirect_out = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    #else
    // TODO support flags O_CREAT
    // TODO support file permission like 0644
    redirect_out = open(outfile, O_WRONLY | O_TRUNC);
    #endif
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

  #if SUPPORT_USER_MALLOC
  vector<int> child_pid_list;
  #else
  #define LOCAL_BUF_SIZE 256
  int child_pid_list[LOCAL_BUF_SIZE];
  int child_pid_list_size = 0;
  #endif

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
    if (current_out != redirect_out) {
      // close the write end for a pipe early so it does not get inherited by
      // child processes created later in this loop.
      close(current_out);
    }
    if (current_in != redirect_in) {
      close(current_in);
    }
    if (last_child < 0) {
      return -1;
    }
    #if SUPPORT_USER_MALLOC
    child_pid_list.push_back(last_child);
    #else
    assert(child_pid_list_size + 1 <= LOCAL_BUF_SIZE);
    child_pid_list[child_pid_list_size++] = last_child;
    #endif

    start_pos = pipe_pos + 1;
  }

  int child_status;
  int rc;
  #if SUPPORT_USER_MALLOC
  for (auto child_pid : child_pid_list) {
    rc = waitpid(child_pid, &child_status, 0);
    assert(rc == child_pid);
  }
  child_pid_list.destruct();
  #else
  for (int i = 0; i < child_pid_list_size; ++i) {
    int child_pid = child_pid_list[i];
    rc = waitpid(child_pid, &child_status, 0);
    assert(rc == child_pid);
  }
  #endif

  if (redirect_in != 0) {
    close(redirect_in);
  }
  if (redirect_out != 1) {
    close(redirect_out);
  }
  return 0;
}

int main(int argc, char** argv) {
  char line[512];
  char* words[64];
  int nword;

  printf("Enter the main function of shell\n");
  while (true) {
    printf("$ ");
    #ifdef HOST_OS
    if (!fgets(line, sizeof(line), stdin)) {
    #else
    if (!fgets(line, sizeof(line), 0)) {
    #endif
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
    execute_command(words, nword);
  }
  printf("bye\n");
  return 0;
}
