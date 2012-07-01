#include "includes.h"
#include "globals.h"
#include "error.h"

/////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////

Startup ini;
bool		g_reset;			 // true if -reset option is given.
int			g_listen_sock;
FILE*		g_init_nbr_file;
FILE*		g_log_file;
int			g_flag;
Neighbor_list g_neighbor_list; // for beacon and non-beacon node
Neighbor_list g_beacon_list;   // for beacon node only
bool g_beacon;

// for Level 1 queue.
std::queue<Event> g_que1;
pthread_cond_t g_cv1;
pthread_mutex_t g_mutex1;

// for Level 2 queues.
std::map<int, Connection* > g_sd2conn;
pthread_mutex_t g_mutex_sd2conn;

// for Level 2 queue for Command Line Interface
std::queue<Event> g_cmd_que;
pthread_mutex_t g_mutex_cmd_que;
pthread_cond_t g_cond_cmd_que;

// vector of beacon status
std::vector<bool> g_beacon_status;
pthread_mutex_t g_mutex_beacon_status;

// map of host_name -> port & sd
std::map<string, Port_sd> g_hostname2port_sd;
pthread_mutex_t g_mutex_hostname2port_sd; 

// map of host_id -> sock_fd
std::map<string, int> g_hostid2sd;
pthread_mutex_t  g_mutex_hostid2sd;

// map of Status_req's uoid -> sock_fd
map<string, int> g_suoid2sd;
pthread_mutex_t g_mutex_suoid2sd;

// for msg logging
pthread_mutex_t  g_mutex_log_msg;

// time to dies
char g_time2die; // Can have [-1 ~ 3]. 3 is self restart

//
int g_curr_neighbor;
pthread_mutex_t g_mutex_curr_neighbor;

//the temperory file counter and its mutex;
int g_temp_fileno;
pthread_mutex_t g_mutex_temp_fileno;

//for mini file system.
Lru g_lru;

int g_fileno;
pthread_mutex_t g_mutex_fileno; // Not used.

map<int, int> g_fileno2ref;
pthread_mutex_t g_mutex_fileno2ref;

map<int, bool> g_fileno2perm;
pthread_mutex_t g_mutex_fileno2perm;

map<string, int> g_fileid2fileno;
map<int, string> g_fileno2fileid;

// is still connected to the core network?
bool g_is_connected;

int short_circuit;
FILE *fp_nam;



/////////////////////////////////////////////////
// Initializers for global variables.
/////////////////////////////////////////////////

void init_globals() {
	g_reset = false;			 // true if -reset option is given.
	g_listen_sock = 0;
	g_init_nbr_file = NULL;
	g_log_file = NULL;
	g_flag = 0;
	g_beacon = false;

	g_time2die = 3; // Can have [-1 ~ 3]. 3 is self restart
	g_curr_neighbor = 0;
	g_is_connected = true;
	g_fileno = 0;
	g_temp_fileno = 0;
	short_circuit = 0;
	fp_nam = NULL;
	struct timeval tv;
	gettimeofday(&tv, NULL);

	srand48(tv.tv_usec);

}


void init_mutexes() {
	int state = 0;	

	state = pthread_mutex_init(&g_mutex1, NULL);
	if(state) {
		print_error_and_exit("init_mutexes(): mutex init failed.");
	}	

	state = pthread_mutex_init(&g_mutex_sd2conn, NULL);
	if(state) {
		print_error_and_exit("init_mutexes(): mutex init failed.");
	}	

	state = pthread_mutex_init(&g_mutex_beacon_status, NULL);
	if(state) {
		print_error_and_exit("init_mutexes(): mutex init failed.");
	}	

	state = pthread_mutex_init(&g_mutex_hostname2port_sd, NULL);
	if(state) {
		print_error_and_exit("init_mutexes(): mutex init failed.");
	}	

	state = pthread_mutex_init(&g_mutex_hostid2sd, NULL);
	if(state) {
		print_error_and_exit("init_mutexes(): mutex init failed.");
	}	

	state = pthread_mutex_init(&g_mutex_log_msg, NULL);
	if(state) {
		print_error_and_exit("init_mutexes(): mutex init failed.");
	}	

	state = pthread_mutex_init(&g_mutex_cmd_que, NULL);
	if(state) {
		print_error_and_exit("init_mutexes(): mutex init failed.");
	}	

	state = pthread_mutex_init(&g_mutex_curr_neighbor, NULL);
	if(state) {
		print_error_and_exit("init_mutexes(): mutex init failed.");
	}	
    state = pthread_mutex_init(&g_mutex_temp_fileno, NULL);
    if(state) {
        print_error_and_exit("init_mutexes(): mutex init failed.");
    }
    state = pthread_mutex_init(&g_mutex_fileno, NULL);
    if(state) {
        print_error_and_exit("init_mutexes(): mutex init failed.");
    }

	state = pthread_cond_init(&g_cv1, NULL);
	if(state) {
		print_error_and_exit("init_mutexes(): cond var init failed.");
	}
	
	state = pthread_cond_init(&g_cond_cmd_que, NULL);
	if(state) {
		print_error_and_exit("init_mutexes(): cond var init failed.");
	}

	state = pthread_mutex_init(&g_mutex_fileno2ref, NULL);
	if(state) {
        print_error_and_exit("init_mutexes(): mutex init failed.");	
	}

	state = pthread_mutex_init(&g_mutex_fileno2perm, NULL);
	if(state) {
        print_error_and_exit("init_mutexes(): mutex init failed.");	
	}


} // end of init_mutexes()


void destroy_mutexes() {
	pthread_mutex_destroy(&g_mutex1);
	pthread_mutex_destroy(&g_mutex_sd2conn);
	pthread_mutex_destroy(&g_mutex_beacon_status);
	pthread_mutex_destroy(&g_mutex_hostname2port_sd);
	pthread_mutex_destroy(&g_mutex_hostid2sd);
	pthread_mutex_destroy(&g_mutex_log_msg);
	pthread_mutex_destroy(&g_mutex_fileno2ref);
	pthread_mutex_destroy(&g_mutex_fileno2perm);
} // end of destory_mutexes()


