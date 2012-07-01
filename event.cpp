#include "event.h"
#include "error.h"

Event::Event() {
	type = 0xFF;
	hdr = NULL;
	vmsg = NULL;
	rfs = 'r';
}


Event::Event(int _type, int _sd, Hdr* _hdr, void* _vmsg, char _rfs) {
	type = _type;
	sd = _sd;
	hdr = _hdr;
	vmsg = _vmsg;
	rfs = _rfs;
}

Event::~Event() {
	//cleanup(); // Need to free data members explicitly!
}

void Event::cleanup() {
	// We need to check if the following works on the target platform
	/*
	delete hdr;
	delete vmsg;
	delete cmd;
	*/

	// Or, we have to move on to the following
	char msg_type = hdr->msg_type;
	switch(msg_type) {
	case HLLO:
		delete (Hello*) vmsg;
		break;
	case JNRQ:
		delete (Join_req*) vmsg;
		break;
 	case JNRS:
		delete (Join_res*) vmsg;
		break;
	case KPAV:
		break;
	default:
		break;
	
	}
	
	vmsg = NULL;

	delete hdr;
	hdr = NULL;
}


Connection::Connection(char _time2die, int _is_beacon) {

	time2die = _time2die;
	is_beacon = _is_beacon;

	// Initialize mutex
	int state1 = pthread_mutex_init(&mutex, NULL);
	if(state1) {
		print_error_and_exit("Connection::Connection(): mutex init failed.");
	}
	
	int	state2 = pthread_cond_init(&cv, NULL);
	if(state2) {
		print_error_and_exit("Connection::Connection(): condigion var init failed.");
	}
}


Connection::~Connection() {
	// Destory mutex
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cv);
}


Port_sd::Port_sd(int _port, int _sd) {
	port = _port;
	sd = _sd;
}
