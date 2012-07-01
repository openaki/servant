#include "sig_handlers.h"

extern int g_flag;

void sigalarm_handler()
{
	g_flag = 1;

}
