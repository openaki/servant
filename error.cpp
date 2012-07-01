#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "error.h"

// function print_error_and_exit
//   A utility function that print a message and exit the program   
// arguments
//   msg: error message to print
//
void print_error_and_exit(const char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}

void print_error(const char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
}
