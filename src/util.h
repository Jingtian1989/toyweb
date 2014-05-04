#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

void unix_error(char *msg);
void posix_error(int code, char *msg);

#endif