#ifndef __UTIL_H__
#define __UTIL_H__

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


void unix_error(char *msg);
void posix_error(int code, char *msg);

#endif