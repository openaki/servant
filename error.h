#ifndef ERROR_H__
#define ERROR_H__

#define NO_ERR				0
#define ERR                  -1     // General error
#define ERR_ARG_NULL		-2

#define ERR_RECV_HDR         -10     // Error in reading message header
#define ERR_RECV_DATA        -11     // Error in reading message data
#define ERR_RECV_TIME_OUT    -12     // Time out while receiving data
#define ERR_RECV_SOCK_CLOSE  -13     // socket closed while receiving data

#define ERR_SOCK_TIMEOUT	-21
#define ERR_SOCK_READ		-22
#define ERR_SOCK_WRITE		-23

#define ERR_FILE_WRITE		-31

void print_error_and_exit(const char *);
void print_error(const char *);


#endif
