// dispatch.h

#ifndef DISPATCH_H__
#define DISPATCH_H__

#include <queue>
#include "event.h"

using namespace std;

void* ckrs_timer( void *);
bool is_cache_available(int file_no);
void save_in_filesystem(int file_no, bool is_permanent);
bool process_event(Event& ev1, std::queue<Event>& tmp_que);
void* dispatch_th(void* arg);

#endif
