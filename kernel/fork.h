#pragma once

int dumbfork();
int cowfork();

/*
 * spawn is the combination of fork and exec.
 * It has the advantage that we don't need to create an address space via fork
 * and then immediately destroy that address space to recreate a new one for exec.
 */
int spawn(const char* path, const char** argv);
