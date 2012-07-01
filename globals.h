#ifndef GLOBALS_H__
#define GLOBALS_H__

#include "lru.h"

#include "startup.h"
#include "neighbor_list.h"
#include "event.h"

using namespace std;

/////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////

extern Startup ini;
extern bool		g_reset;			 // true if -reset option is given.
extern int			g_listen_sock;
extern FILE*		g_init_nbr_file;
extern FILE*		g_log_file;
extern int			g_flag;
extern Neighbor_list g_neighbor_list; // for beacon and non-beacon node
extern Neighbor_list g_beacon_list;   // for beacon node only
extern bool g_beacon;

// for Level 1 queue.
extern std::queue<Event> g_que1;
extern pthread_cond_t g_cv1;
extern pthread_mutex_t g_mutex1;

// for Level 2 queues.
extern std::map<int, Connection* > g_sd2conn;
extern pthread_mutex_t g_mutex_sd2conn;

// for Level 2 queue for Command Line Interface
extern std::queue<Event> g_cmd_que;
extern pthread_mutex_t g_mutex_cmd_que;
extern pthread_cond_t g_cond_cmd_que;

// vector of beacon status
extern std::vector<bool> g_beacon_status;
extern pthread_mutex_t g_mutex_beacon_status;

// map of host_name -> port & sd
extern std::map<string, Port_sd> g_hostname2port_sd;
extern pthread_mutex_t g_mutex_hostname2port_sd; 

// map of host_id -> sock_fd
extern std::map<string, int> g_hostid2sd;
extern pthread_mutex_t  g_mutex_hostid2sd;

// map of Status_req's uoid -> sock_fd
extern map<string, int> g_suoid2sd;
extern pthread_mutex_t g_mutex_suoid2sd;

// for msg logging
extern pthread_mutex_t  g_mutex_log_msg;

// time to dies
extern char g_time2die; // Can have [-1 ~ 3]. 3 is self restart

//
extern int g_curr_neighbor;
extern pthread_mutex_t g_mutex_curr_neighbor;

//the temperory file counter and its mutex;
extern int g_temp_fileno;
extern pthread_mutex_t g_mutex_temp_fileno;

//for mini file system.
extern Lru g_lru;

extern int g_fileno;
extern pthread_mutex_t g_mutex_fileno;

extern map<int, int> g_fileno2ref;
extern pthread_mutex_t g_mutex_fileno2ref;

extern map<int, bool> g_fileno2perm;
extern pthread_mutex_t g_mutex_fileno2perm;

extern map<string, int> g_fileid2fileno;
extern map<int, string> g_fileno2fileid;

// is still connected to the core network?
extern bool g_is_connected;

extern int short_circuit;
extern FILE *fp_nam;



/////////////////////////////////////////////////
// Initializers for global variables.
/////////////////////////////////////////////////

extern void init_globals();
void init_mutexes();
extern void destroy_mutexes();

#endif
