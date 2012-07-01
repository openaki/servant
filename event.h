#ifndef EVENT_H__
#define EVENT_H__

#include "includes.h"
#include "msg.h"

class Event 
{
public:
	char type;
	int sd;
	Hdr* hdr;
	void* vmsg;
	char rfs; // Receive, Forward, Send

	Event();
	Event(int _type, int _sd = 0, Hdr* _hdr = NULL, void* _vmsg = NULL, char _rfs = 'r');
	~Event();
	void cleanup();
};


class Connection 
{
public:
	pthread_t rth;
	pthread_t wth;
	pthread_mutex_t mutex;
	pthread_cond_t cv;
	std::queue< class Event > que;
	char time2die; //'-1' dont die :) 
	int is_beacon;
	string peerid;

	Connection(char _time2die = -1, int _is_beacon = true);
	~Connection();
};


class Port_sd
{
public:
	int port;
	int sd;

	Port_sd(int _port = 0, int sd = 0);
};



#endif
