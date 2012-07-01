#ifndef STARTUP_H__
#define STARTUP_H__
#include <vector>

class Startup
{
public:

	uint16_t port;
	uint32_t location;
	char *homedir;
	char *logfile;
	int autoshutdown;
	int ttl;
	int msglifetime;
	int initneighbor;
	int jointo;
	int getmsglifetime;
	int keepaliveto;
	int minneighbor;
	int nocheck;
	double cacheprob;	// for GTRS
	double storeprob;	// for STRQ
	double nstoreprob;	// for STRQ
	long cachesize;
	int permsize;
	int retry;
	char *hostname;
	char *node_id;
	char *node_inst_id;
	vector< string > beacon_name;
	vector< int > beacon_port;
	string file_path;

	Startup()
	{
		homedir = NULL;
		logfile = (char *)calloc(255, sizeof(char)); //255 is the size of filename
		strcpy(logfile, "servant.log");
		ttl = 30;
		msglifetime = 30;
		getmsglifetime = 300;
		initneighbor = 3;
		jointo = 15;
		keepaliveto = 60;
		minneighbor = 2;
		nocheck = 0;
		cacheprob = 0.1;
		storeprob = 0.1;
		nstoreprob = 0.2;
		cachesize = 500;
		permsize = 0;
		hostname = NULL;
		node_id = NULL;
		node_inst_id = NULL;
		file_path = "";
	}
};

//Startup ini;	

#endif
