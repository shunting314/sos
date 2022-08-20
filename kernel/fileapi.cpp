#include <kernel/fileapi.h>
#include <kernel/user_process.h>
#include <kernel/simfs.h>
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
	if (!dent) {
		return -1; // file does not exist
	}
	int fd = UserProcess::current()->allocFd(path, rwflags);
	return fd;
}
