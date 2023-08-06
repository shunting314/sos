#include <kernel/fileapi.h>
#include <kernel/user_process.h>
#include <kernel/simfs.h>
#include <kernel/keyboard.h>
#include <assert.h>

// TODO: fail if the path is for a dir
// TODO: currently only support absolute path since we don't have
// the concept of current-working-directory for each process.
int file_open(const char* path, int oflags) {
	int rwflags = (oflags & FD_FLAG_RW);
	if (rwflags == 0) {
		return -1; // fail
	}
	auto dent = SimFs::get().walkPath(path);
  if (!dent && (oflags & FD_FLAG_WR)) {
    // try to create the file
    dent = SimFs::get().createFile(path);
  }

	if (!dent) {
		return -1; // file does not exist for read or fail to create for write
	}
	int fd = UserProcess::current()->allocFd(path, rwflags);
	return fd;
}

int file_read(int fd, void *buf, int nbyte) {
	if (nbyte <= 0 || !buf) {
		return -1;
	}

  // TODO need revise this part once we support IO redirection
  if (fd == 0) {
    char* s = (char*) buf;
    int cnt = 0;
    while (cnt < nbyte) {
      char ch = keyboardGetChar(false);
      if (ch == -1) {
        break;
      }
      s[cnt++] = ch;
    }
    return cnt;
  }
	return UserProcess::current()->getFdptr(fd)->read(buf, nbyte);
}

int file_close(int fd) {
  return UserProcess::current()->releaseFd(fd);
}

int file_write(int fd, const void* buf, int nbyte) {
  // TODO need revise this part once we support IO redirection
  if (fd == 1) {
    char *s = (char*) buf;
    for (int i = 0; i < nbyte; ++i) {
      putchar(s[i]);
    }
    return nbyte;
  }
  return UserProcess::current()->getFdptr(fd)->write(buf, nbyte);
}
