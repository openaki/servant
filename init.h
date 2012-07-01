#ifndef INIT_H__
#define INIT_H__

// Private Functions
char * rem_space(char *word);

// Public Functions
int init(char *filename);
int readline(FILE *fp, char *buf, int *len);
#endif
