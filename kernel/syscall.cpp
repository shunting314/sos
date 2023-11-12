#include <kernel/syscall.h>
#include <kernel/user_process.h>
#include <kernel/fork.h>
#include <kernel/fileapi.h>
#include <kernel/simfs.h>
#include <kernel/pipe.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <dirent.h>

int sys_write(int fd, void *buf, int cnt) {
  return file_write(fd, buf, cnt);
}

// terminate the current process
int sys_exit(int status) {
  UserProcess::terminate_current_process(status);
  assert(false && "never reach here");
  return 0;
}

int sys_dumbfork() {
  return dumbfork();
}

int sys_fork() {
  return cowfork();
}

int sys_spawn(const char* path, const char** argv, int fdin, int fdout) {
  return spawn(path, argv, fdin, fdout);
}

int sys_pipe(int fds[2]) {
  return pipe(fds);
}

int sys_getpid() {
  return UserProcess::current()->get_pid();
}

int sys_open(const char*path, int oflags) {
	return file_open(path, oflags);	
}

int sys_read(int fd, void *buf, int nbyte) {
	return file_read(fd, buf, nbyte);
}

int sys_close(int fd) {
  return file_close(fd);
}

// TODO dedup with ls function under kernel/simfs.cpp
int sys_readdir(const char* path, struct dirent* entlist, int capa) {
  auto dent = SimFs::get().walkPath(path);
  if (!dent) {
    printf("Path does not exist %s\n", path);
    return -1;
  }
  int cnt = 0;
  for (auto itr : dent) {
    if (cnt >= capa) {
      printf("dirent list out of capacity\n");
      return -1;
    }

    DirEnt* curEnt = &(*itr);
    struct dirent* dstEnt = entlist + cnt;

    strcpy(dstEnt->name, curEnt->name);
    dstEnt->file_size = curEnt->file_size;
    dstEnt->ent_type = curEnt->ent_type;

    ++cnt;
  }
  return cnt;
}

/*
 * Create an directory using an absolute path.
 * Only works if the parent directly already exists.
 */
int sys_mkdir(const char* path) {
  return SimFs::get().mkdir(path);
}

int sys_getcwd(char* path, int len) {
  auto cur = UserProcess::current();
  const char* cwd = cur->getCwd();
  assert(cwd);
  int reqlen = strlen(cwd) + 1;
  if (len < reqlen) {
    return -1;
  }
  memmove(path, cwd, reqlen);
  return 0;
}

/*
 * Return the child process id on success and -1 on error.
 * Note that if the child process is not terminated yet when this function is called,
 * we never gonna return to this call frame but the scheduler will resume the
 * parent process directly when the child process has teminated.
 */
int sys_waitpid(int pid, int *pstatus) {
  return UserProcess::current()->waitpid(pid, pstatus);
}

int sys_chdir(const char* path) {
  return UserProcess::current()->chdir(path);
}

int sys_unlink(const char* path) {
  return SimFs::get().unlink(path);
}

void *sc_handlers[NUM_SYS_CALL] = {
  nullptr, // number 0
  // "[SC_WRITE] = fptr; " seems work in C but is not supported by C++.
  // We need be super carefully with the order.
  /* [SC_WRITE] = */ (void*) sys_write,
  /* [SC_EXIT] = */ (void*) sys_exit,
  /* SC_DUMBFORK */ (void*) sys_dumbfork,
  /* SC_FORK */ (void*) sys_fork,
  /* SC_GETPID */ (void*) sys_getpid,
	/* SC_OPEN */ (void*) sys_open,
	/* SC_READ */ (void*) sys_read,
  /* SC_CLOSE */ (void*) sys_close,
  /* SC_WAITPID */ (void*) sys_waitpid,
  /* SC_SPAWN */ (void*) sys_spawn,
  /* SC_READDIR */ (void*) sys_readdir,
  /* SC_MKDIR */ (void*) sys_mkdir,
  /* SC_GETCWD */ (void*) sys_getcwd,
  /* SC_CHDIR */ (void *) sys_chdir,
  /* SC_PIPE */ (void *) sys_pipe,
  /* SC_UNLINK */ (void *) sys_unlink,
};

typedef int (*sc_handler_type)(int arg1, int arg2, int arg3, int arg4, int arg5);

int syscall(int sc_no, int arg1, int arg2, int arg3, int arg4, int arg5) {
  // printf("Handle syscall %d, args[0x%x, 0x%x, 0x%x, 0x%x, 0x%x]\n", sc_no, arg1, arg2, arg3, arg4, arg5);
  assert(sc_no < NUM_SYS_CALL && sc_handlers[sc_no]);
  return ((sc_handler_type)sc_handlers[sc_no])(arg1, arg2, arg3, arg4, arg5);
}
