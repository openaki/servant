#include "includes.h"
#include "constants.h"
#include "startup.h"
#include "init.h"
#include "error.h"
#include "msg.h"
#include "neighbor_list.h"
#include "event.h"
#include "util.h"
#include "dispatch.h"
#include "bitvector.h"
#include "indice.h"
#include "globals.h"
#include "lru.h"

map<int, struct Meta> g_ui2meta;
string g_getextfile;
FILE *fp_stat_file;
void *keep_alive_th(void* arg)
{
	struct timeval _tv;
	_tv.tv_sec = ini.keepaliveto / 2;
	_tv.tv_usec = 0;
		
	while(1) {
		struct timeval tv = _tv;	
		select(0, NULL, NULL, NULL, &tv);

		if(g_time2die > -1) { //@
			return NULL;
		}
		
		// Create Hdr		
		char uoid[20];
		char obj_type[] = "msg";
		getuoid(ini.node_inst_id, obj_type, uoid, 20);
		// Hdr * hdr = new Hdr(KPAV, uoid, 2);
		Hdr * hdr = new Hdr(KPAV, uoid, 1); //!

		// Create Event
		Event ev(EV_MSG, SD_KPAV, hdr, NULL); // -1 for cmd_th. -2 for keep_alive_th. -3 for CKRQ
		
		pthread_mutex_lock(&g_mutex1);
		{
			g_que1.push(ev);
			pthread_cond_signal(&g_cv1);		
		}
		pthread_mutex_unlock(&g_mutex1);
		
		/* Old way of waking up sleeping threads .. efficient on network but may cause deadlocks
		pthread_mutex_lock(&g_mutex_hostid2sd);
		{
			if(DEBUG_PRO_EV) printf("/##  1\n");
			std::map<string, int>::iterator itr;

			// For every sd currently available,
			for(itr = g_hostid2sd.begin(); itr != g_hostid2sd.end(); itr++) {
				int sd = itr->second;
				pthread_mutex_lock(&g_mutex_sd2conn);
				{
					conn = g_sd2conn[sd];
					pthread_mutex_lock(&conn->mutex);
					{
						pthread_cond_signal(&conn->cv);
					}
					pthread_mutex_unlock(&conn->mutex);
				}
				pthread_mutex_unlock(&g_mutex_sd2conn);
			}
		}
		pthread_mutex_unlock(&g_mutex_hostid2sd);
		*/
	}
	return NULL;
}


void* read_th(void* arg) {
	int sd = *((int*) arg);
	Hdr* hdr = NULL;
	void* vmsg = NULL;
	int x;
	Connection *conn;

	pthread_mutex_lock(&g_mutex_sd2conn);
	{
		std::map<int, Connection* >::iterator sd2conn_iter;
		sd2conn_iter = g_sd2conn.find(sd);
		if(sd2conn_iter == g_sd2conn.end()) { 
			pthread_mutex_unlock(&g_mutex_sd2conn);
			return NULL;
		}
		
		conn = g_sd2conn[sd];
	}
	pthread_mutex_unlock(&g_mutex_sd2conn);

	while(1) {
		// Fill the message
		if(g_time2die > -1) { //@
			return NULL;
		}
		
		x = recv_msg(sd, &hdr, &vmsg);
		if(x == -1) {
			pthread_mutex_lock(&(conn->mutex));
			conn->time2die = 1;
			//printf("Error in recv message: in read th\n");
/*			
			if(conn->is_beacon != -1) {
				pthread_mutex_lock(&(g_mutex_beacon_status));
					g_beacon_status[conn->is_beacon] = 0;
				pthread_mutex_unlock(&(g_mutex_beacon_status));

				pthread_mutex_lock(&(g_mutex_hostid2sd));
					map<string, int>::iterator it = g_hostid2sd.find(conn->peerid);
					g_hostid2sd.erase(it);
				pthread_mutex_unlock(&(g_mutex_hostid2sd));
			}
*/			
			pthread_mutex_unlock(&(conn->mutex));
			return NULL ;
		}
		//Not enough space to remove the packet .. only happens when we are out of space for temp_file
		if(x == -2) {
			printf("Not Enough space to recive a file .. Please clear some space.\n");
			continue;
		}
		// Create an level-1 event
		Event ev1(EV_MSG, sd, hdr, vmsg);
		
		// Push the event into the level-1 event queue
		pthread_mutex_lock(&g_mutex1);
		{
			g_que1.push(ev1);
			pthread_cond_signal(&g_cv1);
		}
		pthread_mutex_unlock(&g_mutex1);
	}
	return NULL;
} // end of read_th


void* write_th(void* arg) {
	int sd = *((int*) arg);
	Connection *conn;
	Event ev;
	pthread_mutex_lock(&g_mutex_sd2conn);
	{
		std::map<int, Connection* >::iterator sd2conn_iter;
		sd2conn_iter = g_sd2conn.find(sd);
		if(sd2conn_iter == g_sd2conn.end()) { 
			pthread_mutex_unlock(&g_mutex_sd2conn);
			return NULL;
		}
	
		conn = g_sd2conn[sd];
	}
	pthread_mutex_unlock(&g_mutex_sd2conn);

	while(1) {
		// Get connection info.

		// Pop an Level-2 event 
		pthread_mutex_lock(&(conn->mutex));
		{
			// Wait while queue is empty
			while(conn->que.size() == 0) {
				// If time to die, send notify to the other node on the socket 
				if(conn->time2die > -1) {
					pthread_mutex_unlock(&(conn->mutex));
					//send_notify(conn->time2die, sd);
					return NULL;
				}
				if(g_time2die > -1) {
					pthread_mutex_unlock(&(conn->mutex));
					if(DEBUG) printf("write th():Sending Notify msg.\n");
					send_notify(g_time2die, sd);
					return NULL;
				}				

				pthread_cond_wait(&(conn->cv), &(conn->mutex));
			}
			// Retrieve an event from L2 queue
			ev = conn->que.front();
			conn->que.pop();
		
			if(DEBUG) printf("write th():Pop L2 event. msg_type: 0x%02x.\n", (unsigned char) ev.hdr->msg_type);
		}
		pthread_mutex_unlock(&(conn->mutex));
		
		if(conn->time2die > -1) {
			ev.cleanup();
			
			// Evacuate the Level-2 queue
			pthread_mutex_lock(&(conn->mutex));			
			{
				while(conn->que.size() > 0){
					ev = conn->que.front();
					conn->que.pop();
					ev.cleanup();
				} 
			}
			pthread_mutex_unlock(&(conn->mutex));

			return NULL;
		}

		if(g_time2die > -1) {
			// If time to die, send notify to the other node on the socket 
			if(DEBUG) printf("write th():Sending Notify msg.\n");
			send_notify(g_time2die, sd);
			ev.cleanup();
			
			// Evacuate the Level-2 queue
			pthread_mutex_lock(&(conn->mutex));			
			{
				while(conn->que.size() > 0){
					ev = conn->que.front();
					conn->que.pop();
					ev.cleanup();
				} 
			}
			pthread_mutex_unlock(&(conn->mutex));

			return NULL;
		}
		// Send the message in the event
		if(DEBUG) printf("write th():Sending msg: 0x%02x.\n", (unsigned char) ev.hdr->msg_type);
		int send_err = send_msg(ev.sd, ev.hdr, ev.vmsg);

		// Log sent msg
		if(send_err == 0) {
			pthread_mutex_lock(&g_mutex_log_msg);
				log_msg(g_log_file, ev.hdr, ev.vmsg, ev.rfs, ev.sd);
			pthread_mutex_unlock(&g_mutex_log_msg);
		}
		if(send_err == -1) {
			//printf("Error Sending message in write thread\n");
			continue;
		}
		ev.cleanup();
	}

	return NULL;
} // end of write_th()


int explore_neighbors() {
	string	beacon_name;
	unsigned short beacon_port;

	hostent* beacon = NULL;
	sockaddr_in beacon_addr;
	int result = ERR;
	int num_neighbors = 0;

	Hdr* request_hdr = NULL;
	Join_req* request_msg = NULL;

	Hdr* reply_hdr = NULL;
	Join_res* reply_msg = NULL;
	char init_neighbor_name[300] = "";
	
	g_neighbor_list.nodes.clear();
	sprintf(init_neighbor_name, "%sinit_neighbor_list", ini.homedir);
	// if reset option is not given
	if(g_reset == false) {
		struct stat statbuff;
		
		int status = stat(init_neighbor_name, &statbuff);

		// check if file already exists.
		if(status == 0) {
			// No need to explore neighbors. Just read them from init_neighbor_list file.
			g_neighbor_list.read_file();
			if(DEBUG) printf("I read init_neighbor_list file!\n");
			return 0;
		}
	}
	else {
		FILE *fp = fopen(init_neighbor_name, "w");
		fclose(fp);
		if(DEBUG) printf("init_neighbor_list file deleted.\n");
	}

	// Create a socket to send a join msg
	int join_sock = socket(PF_INET, SOCK_STREAM, 0);
	
	if(join_sock == -1) {
		print_error_and_exit("socket() error!\n");
	}

	// Try sending a join msg to one of the beacons
	unsigned int i = 0;
	for(i = 0; i < ini.beacon_name.size(); i++) {
		// Choose a beacon
		beacon_name = ini.beacon_name[i];
		beacon_port = ini.beacon_port[i];
		beacon = gethostbyname(beacon_name.c_str());

		// if the beacon is not reachable, try the next one
		if(!beacon) {
			if(DEBUG) print_error("Unable to resolve address of a beacon");
			continue; // Try next beacon
		}
		memcpy(&beacon_addr.sin_addr, beacon->h_addr, beacon->h_length);

		// Set beacon address and port
		beacon_addr.sin_family = AF_INET;
		beacon_addr.sin_port = beacon_port;

		// Connect to the beacon
		result = connect(join_sock, (struct sockaddr*) &beacon_addr, sizeof(beacon_addr));
		if(result == 0) {
			if(DEBUG) print_error("connect() success");
			break; // beacon connected
		}
		//perror("connect()");
	}
	
	//No beacon found for connection
	if(i == ini.beacon_name.size())
		return -1;

	// Store peer id of the beacon node... for one time use!
	pthread_mutex_lock(&g_mutex_sd2conn);
	{
		Connection* conn = new Connection(-1, false);
		conn->peerid = make_hostid(beacon_name, beacon_port);

		g_sd2conn[join_sock] = conn;
	}
	pthread_mutex_unlock(&g_mutex_sd2conn);
	
	// Create, send and delete a JNRQ message
	result = join_req(&request_hdr, (void **)&request_msg);
	int send_err = send_msg(join_sock, request_hdr, request_msg);

	// Log sent msg
	if(send_err == 0) {
		pthread_mutex_lock(&g_mutex_log_msg);
			log_msg(g_log_file, request_hdr, request_msg, 's', join_sock);
		pthread_mutex_unlock(&g_mutex_log_msg);
	}
	
	delete request_hdr;
	delete request_msg;
	request_hdr = NULL;
	request_msg = NULL;

	// Wait "join time out" for Join_res messages
	alarm(ini.jointo);

	while(1) {
		struct timeval time;
		time.tv_sec = 1;
		time.tv_usec = 0;

		fd_set rset;
		FD_ZERO(&rset);
		FD_SET(join_sock, &rset);
		result = 0;
		
		// Waiting for a packet		
		while(!result) {
			fd_set rset0 = rset;
			timeval time0 = time;
			result = select(join_sock + 1, &rset0, NULL, NULL, &time0);
			if(g_flag == 1) {
				break;
			}
		}
		if(g_flag == 1) {
			g_flag = 0;
			break;
		}
	
		// Receive Join_res message
		result = recv_msg(join_sock, &reply_hdr, (void **)&reply_msg);
		if(result == 0) {
			//Store the beacon info in g_neighbor_list
			g_neighbor_list.add_node(reply_msg);
			num_neighbors++;


			// log JNRS msg
			pthread_mutex_lock(&g_mutex_log_msg);
				log_msg(g_log_file, reply_hdr, reply_msg, 'r', join_sock);
			pthread_mutex_unlock(&g_mutex_log_msg);
			
			delete reply_hdr;
			delete reply_msg;
			reply_hdr = NULL;
			reply_msg = NULL;
		}
	}

	// Delete peer id of the beacon node.
	pthread_mutex_lock(&g_mutex_sd2conn);
	{
		Connection* conn = g_sd2conn[join_sock];
		delete conn;
		g_sd2conn.erase(join_sock);
	}
	pthread_mutex_unlock(&g_mutex_sd2conn);

	// Delete nodes except closest ones.
	result = g_neighbor_list.prune(ini.initneighbor);
	if(DEBUG) printf("Num of init neighbors: %d\n", result);

	// print of file: init_neighbor_list
	g_neighbor_list.write_file();
	
	// Close socket
	close(join_sock);


	return 0;
} // end of explore_neighbors()


// function printUsageAndExit
//	 Print commandline usage info.	
// arguments
//	 none
//
void print_usage_and_exit() {
	printf("Usage: sv_node [-reset] xxx.ini\n");
	printf("where xxx is the name of the initialization file\n");
	exit(1);
}


// function read_command_line
//	 read command line options	 
// arguments
//	 argc: from command line
//	 argv: from command line
//
void read_command_line(int argc, char *argv[]) {

	// valid range of argc is 2 ~ 3
	// ex) sv_node -reset startup.ini
	if (argc == 1 || argc > 3) {
		print_usage_and_exit();
	}
	g_reset = false;
	
	// Walk through options
	for(int i = 1; i < argc - 1; i++) {
		if(argv[i][0] == '-') {
			if(strcmp(&argv[i][1], "reset") == 0) {
				g_reset = true;
			}
			else {
				print_usage_and_exit();
			}
		}
		else {
			print_usage_and_exit();
		}
	}

	if(argv[argc-1][0] == '-') {
		print_usage_and_exit();
	}
} // end of read_command_line()


// start_connection
//	 Spawn read_th, write_th.
//
int start_connection(int sock_fd, int is_beacon)
{
	pthread_t r_thread, w_thread;

	// Create connection info
	Connection *con = new Connection(-1, is_beacon);
	if(con == NULL) {
		//printf("Memory Allocation Failure\n");
		exit(1);
	}

	// Lock mutex before spawning reading/writing thread.
	pthread_mutex_lock(&g_mutex_sd2conn); 
	{
		// Create read thread
		pthread_create(&r_thread, NULL, read_th, &sock_fd);
		con->rth = r_thread;

		// Create write thread
		pthread_create(&w_thread, NULL, write_th, &sock_fd);
		con->wth = w_thread;

		// Add it to the map
		g_sd2conn[sock_fd] = con;	
	}
	pthread_mutex_unlock(&g_mutex_sd2conn);

	return 0;
} // end of start_connection()


void* server_thread(void* arg)
{
	struct sockaddr_in my_addr;
	struct sockaddr_in client_addr;
	int yes = 1;
	int sockfd, new_fd;

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = ini.port;
	inet_aton((const char *)gethostbyname(ini.hostname), &(my_addr.sin_addr));
	memset(my_addr.sin_zero, '\0', sizeof(my_addr.sin_zero));
	
	// Create listening socket
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket()");
		printf("Please Try again\n");
		exit(1);
	}

	// Set socket
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		perror("setsockopt()");
		printf("Please Try again\n");
		exit(1);
	}

	// Bind
	if(bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr))) {
		perror("Bind()");
		printf("Please Try again\n");
		exit(1);
	}

	// Listen
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen()");
		printf("Please Try again\n");
		exit(1);
	}

	// Accept
	while(1) {
		socklen_t sin_size = sizeof(client_addr);
		new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);

		if (new_fd == -1) {
			//perror(" server_thread(): Can't/Doesn't accept()");
			return NULL; //@
		}
		else if(g_time2die > -1) { //@
			//close(new_fd); // Don't reply. Let the peer give up.
			continue;
		}
		
		if(DEBUG) printf("server_thread(): Client accepted.\n");

		if(DEBUG) printf("server_thread(): Calling start-connection().\n");
		// Spawn read/write thrread.
		start_connection(new_fd, -1);
		if(DEBUG) printf("server_thread(): Exiting start-connection().\n");

		pthread_mutex_lock(&g_mutex_curr_neighbor); //@
			g_curr_neighbor++;
		pthread_mutex_unlock(&g_mutex_curr_neighbor);
	}

	return NULL;
} // end of server_thread()


void beacon_connect()
{
	struct sockaddr_in my_addr;
	unsigned int i;
	int sockfd;
	hostent *h;
	my_addr.sin_family = AF_INET;

	bool double_conn_closed = false;
	
	// Set all beacon status to OFF
	pthread_mutex_lock(&g_mutex_beacon_status);
	{
		for(i = 0; i< ini.beacon_name.size(); i++) {
			g_beacon_status.push_back(0);
		}
	}
	pthread_mutex_unlock(&g_mutex_beacon_status);

	while(1) {
		for(i = 0; i< ini.beacon_name.size(); i++) {

			if(g_time2die > -1) {
				return;
			}			
			
			string peerid = "";
			peerid += ini.beacon_name[i]; 
			peerid += "_";
			stringstream out_stream;
			out_stream << ini.beacon_port[i];
			string s_temp = out_stream.str();
			peerid += s_temp;

			// if beacon is connected, it's OK.
			pthread_mutex_lock(&g_mutex_beacon_status);			
			if(g_beacon_status[i] == 1) {
				pthread_mutex_unlock(&g_mutex_beacon_status);
				continue;
			}		
			pthread_mutex_unlock(&g_mutex_beacon_status);
			
			// if I'm the beacon, set it to 'connected'
			if(strcmp(ini.hostname, ini.beacon_name[i].c_str()) == 0) {
				if(ini.port == ini.beacon_port[i]) {
	
					pthread_mutex_lock(&g_mutex_beacon_status);
						g_beacon_status[i] = 1;
					pthread_mutex_unlock(&g_mutex_beacon_status);
	
					continue;
				}
			}
			//pthread_mutex_unlock(&g_mutex_beacon_status); // ??????
	
			// It's a beacon not connected
			my_addr.sin_port = ini.beacon_port[i];
			const char *buffer = ini.beacon_name[i].c_str();
			
			if((h = gethostbyname(buffer)) == NULL) {
				//perror("Address Request");
				continue;
			}
			memcpy(&my_addr.sin_addr, h->h_addr, h->h_length);
			//inet_aton((const char *)gethostbyname(ini.beacon_name[i].c_str()), &(my_addr.sin_addr));
			memset(my_addr.sin_zero, '\0', sizeof(my_addr.sin_zero));

			// Create a socket
			if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
				perror("beacon-connect():socket()");
				exit(1);
			}

			// Connect to the beacon
			if(connect(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1) {
				//perror("beacon-connect():connect()");
				fflush(stdout);	
				pthread_mutex_lock(&g_mutex_beacon_status);
					g_beacon_status[i] = 0;
				pthread_mutex_unlock(&g_mutex_beacon_status);
				continue;
				//exit(1);
			}

			// Set it 'connected'
			pthread_mutex_lock(&g_mutex_beacon_status);
				g_beacon_status[i] = 1;
			pthread_mutex_unlock(&g_mutex_beacon_status);

			// Spawn read/write thrread.
			if(DEBUG) printf("beacon-connect(): Calling start-connection().\n");
			start_connection(sockfd, i);
			if(DEBUG) printf("beacon-connect(): Exiting start-connection().\n");

			// Set peer id
			pthread_mutex_lock(&g_mutex_sd2conn);
			{
				std::map<int, Connection* >::iterator itr = g_sd2conn.find(sockfd);
				if(itr == g_sd2conn.end()) { 
					pthread_mutex_unlock(&g_mutex_sd2conn);
					continue;
				}
			
				Connection *conn = g_sd2conn[sockfd];
				conn->peerid = peerid;
			}
			pthread_mutex_unlock(&g_mutex_sd2conn);
		 
			// Post a HELLO msg.
			post_hello(sockfd);

			pthread_mutex_lock(&g_mutex_hostid2sd);
				map<string, int>::iterator itr = g_hostid2sd.find(peerid);
			pthread_mutex_unlock(&g_mutex_hostid2sd);
			
			if(itr != g_hostid2sd.end()) {
				if(sockfd != itr->second) {
					//printf("Double Connection found !!\n");
					
					if(ini.port > ini.beacon_port[i]) {
						//printf("The connection initiated by me wins\n");
						close(itr->second);
						pthread_mutex_lock(&g_mutex_hostid2sd);
							itr->second = sockfd;
						pthread_mutex_unlock(&g_mutex_hostid2sd);
					}
					else if(ini.port < ini.beacon_port[i]) {
						//printf("The connection initiated by the peer wins\n");
						close(sockfd);
					}
					else {
						if((strcmp(ini.hostname, ini.beacon_name[i].c_str())) > 0) {
							//printf("The connection initiated by me wins ... though the port is same\n");
							close(itr->second);
							pthread_mutex_lock(&g_mutex_hostid2sd);
								itr->second = sockfd;
							pthread_mutex_unlock(&g_mutex_hostid2sd);
						}
						else {
							//printf("The connection initiated by the peer wins ... though the port is same\n");
							close(sockfd);
						}
					}
					double_conn_closed = true;
				}
/*
				// if double connection deleted,
				// Delete an entry in sd2conn map
				if(double_conn_closed) {
					pthread_mutex_lock(&g_mutex_sd2conn);
						g_sd2conn.erase(sockfd);
					pthread_mutex_unlock(&g_mutex_sd2conn);
					double_conn_closed = false;
				}
*/
			}
			else {
				pthread_mutex_lock(&(g_mutex_hostid2sd));
					g_hostid2sd[peerid] = sockfd;
				pthread_mutex_unlock(&(g_mutex_hostid2sd));
			}
				
		}
		
		//sleep(1);
		timeval tv;
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		select(0, NULL, NULL, NULL, &tv);
	}
	return;
} // end of beacon-connect()


int nonbeacon_connect()
{
	struct sockaddr_in my_addr;
	int sockfd;
	hostent *h;
	my_addr.sin_family = AF_INET;

	pthread_mutex_lock(&g_mutex_curr_neighbor);  
		g_curr_neighbor = 0;
	pthread_mutex_unlock(&g_mutex_curr_neighbor);

	list<Node_info>::iterator it;
	unsigned int i = 0;
	for(it = g_neighbor_list.nodes.begin(); it != g_neighbor_list.nodes.end(); it++, i++) {
			
		string peerid = "";
		peerid += it->host_name; 
		peerid += "_";
		stringstream out_stream;
		out_stream << it->port;
		string s_temp = out_stream.str();
		peerid += s_temp;
				
		my_addr.sin_port = it->port;					
		const char *buffer = it->host_name.c_str();
		
		//printf("Hostname trying to connect is %s\n", buffer);
		if((h = gethostbyname(buffer)) == NULL) {
			//perror("Address Request");
			continue;
		}
		
		memcpy(&my_addr.sin_addr, h->h_addr, h->h_length);
		memset(my_addr.sin_zero, '\0', sizeof(my_addr.sin_zero));

		// Create a socket
		if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
			perror("ERR nonbeacon-connect():socket()");
			exit(1);
		}
		// Connect to the neighbouring node
		if(connect(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1) {
			//perror("ERR nonbeacon-connect():connect()");
			fflush(stdout);	
			continue;
			//exit(1);
		}
			
		// Spawn read/write thrread.
		start_connection(sockfd, -1); //-1 for not beacon
		pthread_mutex_lock(&g_mutex_curr_neighbor);  
			g_curr_neighbor++;
		pthread_mutex_unlock(&g_mutex_curr_neighbor);


		pthread_mutex_lock(&g_mutex_hostid2sd);
		{
			g_hostid2sd[peerid] = sockfd;
		}
		pthread_mutex_unlock(&g_mutex_hostid2sd);

		pthread_mutex_lock(&g_mutex_sd2conn);
		{
			Connection *conn = g_sd2conn[sockfd];
			conn->peerid = peerid;
		}
		pthread_mutex_unlock(&g_mutex_sd2conn);
			
		// Post a HELLO msg.
		post_hello(sockfd);
	}
	
	pthread_mutex_lock(&g_mutex_curr_neighbor);  
		if(g_curr_neighbor < ini.minneighbor) {
			pthread_mutex_unlock(&g_mutex_curr_neighbor);
			return -1;
		}
	pthread_mutex_unlock(&g_mutex_curr_neighbor);
	
	return 0; //0 is for sucess and that  we have more than min neighbors connected to us.
} // end of non-beacon-connect()


void beacon_main()
{
	pthread_t se_th;
	pthread_t dis_thread;
	
	// Create dispatch thread
	pthread_create(&dis_thread, NULL, dispatch_th, NULL);
	if(DEBUG) printf("dispatch thread created.\n");	

	// Create server thread
	pthread_create(&se_th, NULL, server_thread, NULL);
	if(DEBUG) printf("server thread created ..................\n");	

	beacon_connect();
	return;
} // end of beacon_main()


void kill_this_node(char err_code) {
	g_time2die = err_code;

	
	if(DEBUG_KILL) {
		//printf("0. kill_this_node():err_code = %x\n", err_code);
		fflush(stdout);
	}
	
	pthread_mutex_lock(&g_mutex_sd2conn);
		
		void* thread_ret;
		std::map<int, Connection* >::iterator iter;

		if(DEBUG_KILL) printf("1. ");
		for(iter = g_sd2conn.begin(); iter != g_sd2conn.end(); iter++) {
				
			if(DEBUG_KILL) printf("1.1 ");
			pthread_cond_broadcast(&(iter->second->cv));
			pthread_mutex_unlock(&g_mutex_sd2conn);
			
			if(DEBUG_KILL) printf("1.1 ");
			pthread_join(iter->second->wth, &thread_ret);			
			if(DEBUG_KILL) printf("1.1 ");
			
			pthread_mutex_lock(&g_mutex_sd2conn);
		}

		if(DEBUG_KILL) printf("2. ");
	pthread_mutex_unlock(&g_mutex_sd2conn);
	
	g_indice.write_index_file();
	g_lru.write_lru_file();
	write_fileid_index_file(g_fileid2fileno, ini.homedir, g_fileno, g_temp_fileno);
	write_fileno2perm_index_file(g_fileno2perm, ini.homedir);

} // end of kill_this_node



void* sig_handler( void *)
{
	sigset_t signal_set;
	int sig;
	//printf("Is sig handler created\n");
	while(1) {
		sigfillset( &signal_set);
		sigwait( &signal_set, &sig);

		switch(sig) {
		case SIGALRM: 
			//traves the peerid2sockfd map
			//and send notify to all and shutdown the connection
			//printf("Auto shutdown time expired ... Sending NTFY msg to all peers\n");
			g_flag = 1;	
			//**
			//kill_this_node(1);
			//exit(1);
			break;
		case SIGQUIT:
			//exit_now = TRUE;
			//return NULL;
			////break;
		case SIGINT:
			//printf("Did I catch the sigint\n");
			short_circuit = 1;
			//break;
		case SIGTRAP:
			short_circuit = 1;
			pthread_cond_signal(&g_cond_cmd_que);		
		}
	}
	return NULL;
} // end of sig_handler()


void * auto_shutdown(void *)
{
	struct timeval tv;
	tv.tv_sec = ini.autoshutdown;
	tv.tv_usec = 0;
	select(0, NULL, NULL, NULL, &tv);
	kill_this_node(1);
	exit(1);
	return NULL;

}


void * cmd_status_neighbor(void *)
{
	Event e1;
	map<int, int> nodes_done;
	map<int, int>::iterator itr;
	Status_res *msg;
	
	nodes_done[ini.port] = 1;

	if(DEBUG_CMD_WR) printf("*** 0.		\n");
	while(1) {
		if(DEBUG_CMD_WR) printf("*** 1.		\n");
		pthread_mutex_lock(&(g_mutex_cmd_que));
			while(g_cmd_que.size() == 0) {
				if(short_circuit == 1) {
					if(DEBUG_CMD_WR) printf("*** 1.1	 \n");
					pthread_mutex_unlock(&(g_mutex_cmd_que));
					//Please remove the UOID from the map so that no other packets can on the queue
					pthread_mutex_lock(&g_mutex_suoid2sd);
						
					pthread_mutex_unlock(&g_mutex_suoid2sd);
					
					return NULL;
	
				}
				if(DEBUG_CMD_WR) printf("*** 1.2	 \n");
	
				pthread_cond_wait(&g_cond_cmd_que, &g_mutex_cmd_que);
			}
			if(DEBUG_CMD_WR) printf("*** 1.3	 \n");
			e1 = g_cmd_que.front();
			g_cmd_que.pop();
		pthread_mutex_unlock(&(g_mutex_cmd_que));
		
		if(e1.hdr->msg_type != STRS)
			continue;
		
		if(short_circuit == 1) {
			//Remove the entry from the map
			return NULL;
		}
		if(DEBUG_CMD_WR) printf("*pthread_mutex_lock(&g_mutex_suoid2sd);** 2.		\n");
		msg = (Status_res *)e1.vmsg;
		uint32_t temp,record_len, i = 0;
		uint16_t stemp, port;
		
		memcpy(&temp, msg->data, 4);
		record_len = ntohl(temp);
		//printf("The record length = %d\n", record_len);
		i=4;

		if(DEBUG_CMD_WR) printf("*** 3.		\n");
		while(record_len != 0) {
			memcpy(&stemp, msg->data + i, 2);
			//printf("Port in netowrk order %d\n" ,stemp);
						
			if(DEBUG_CMD_WR) printf("*** 3.1	 \n");
			port = ntohs(stemp);
			//printf("Neighbouring node = %d\n", port);
			itr = nodes_done.find(port);
			if(itr == nodes_done.end()) {
				nodes_done[port] = 1;
				if(DEBUG_CMD_WR) printf("*** 3.2	 \n");
				//Add the node to the nam file
				fprintf(fp_nam, "n -t * -s %d -c red -i black\n", port);

			}

			if(DEBUG_CMD_WR) printf("*** 3.3	 \n");
			//Add the link to the nam file
			fprintf(fp_nam, "l -t * -s %d -d %d -c blue\n", msg->host_port,port);
			i = i + record_len ;
			memcpy(&temp, msg->data + i, 4);
			if(DEBUG_CMD_WR) printf("*** 3.4	 \n");
			record_len = ntohl(temp);
			i = i + 4;
		}
		if(DEBUG_CMD_WR) printf("*** 4.		\n");
	}
	return NULL;
}

void * cmd_status_file(void *)
{
	Event e1;
	
	Status_res *msg;
	
	if(DEBUG_CMD_WR) printf("*** 0.		\n");
	while(1) {
		if(DEBUG_CMD_WR) printf("*** 1.		\n");
		pthread_mutex_lock(&(g_mutex_cmd_que));
			while(g_cmd_que.size() == 0) {
				if(short_circuit == 1) {
					if(DEBUG_CMD_WR) printf("*** 1.1	 \n");
					pthread_mutex_unlock(&(g_mutex_cmd_que));
					//Please remove the UOID from the map so that no other packets can on the queue
					pthread_mutex_lock(&g_mutex_suoid2sd);
						
					pthread_mutex_unlock(&g_mutex_suoid2sd);
					
					return NULL;
	
				}
				if(DEBUG_CMD_WR) printf("*** 1.2	 \n");
	
				pthread_cond_wait(&g_cond_cmd_que, &g_mutex_cmd_que);
			}
			if(DEBUG_CMD_WR) printf("*** 1.3	 \n");
			e1 = g_cmd_que.front();
			g_cmd_que.pop();
		pthread_mutex_unlock(&(g_mutex_cmd_que));
		
		if(e1.hdr->msg_type != STRS )
			continue;
		
		if(short_circuit == 1) {
			//Remove the entry from the map
			return NULL;
		}
		
		msg = (Status_res *)e1.vmsg;
		
		uint32_t temp,record_len, i = 0;
		
		memcpy(&temp, msg->data, 4);
		record_len = ntohl(temp);
		//printf("The record length = %d\n", record_len);
		i=4;
		
		if(DEBUG_CMD_WR) printf("*** 3.		\n");
		if(record_len == 0) {
			fprintf(fp_stat_file, "\n%s:%d has no files.\n", msg->host_name, msg->host_port);
			continue;
		}
		else {
			memcpy(&temp, msg->data + record_len + 4, 4);
			uint32_t temp2 = ntohl(temp);
			if(temp2 == 0) {
				fprintf(fp_stat_file, "\n%s:%d has the following file:\n", msg->host_name, msg->host_port);
			}
			else {
				fprintf(fp_stat_file, "\n%s:%d has the following files:\n", msg->host_name, msg->host_port);
			}
		}
		
		while(record_len != 0) {
			
			fwrite(msg->data + i, 1, record_len, fp_stat_file);			
			
			i = i + record_len ;
			
			memcpy(&temp, msg->data + i, 4);
			
			record_len = ntohl(temp);
			i = i + 4;
		}
	}
	return NULL;
}

void * cmd_search_res(void *)
{
	Event e1;
	Search_res *msg;
	uint32_t count = 1;
	
	g_ui2meta.clear();
	if(DEBUG_CMD_WR) printf("*** 0.		\n");
	while(1) {
		if(DEBUG_CMD_WR) printf("*** 1.		\n");
		pthread_mutex_lock(&(g_mutex_cmd_que));
			while(g_cmd_que.size() == 0) {
				if(short_circuit == 1) {
					if(DEBUG_CMD_WR) printf("*** 1.1	 \n");
					pthread_mutex_unlock(&(g_mutex_cmd_que));
					//Please remove the UOID from the map so that no other packets can on the queue
					pthread_mutex_lock(&g_mutex_suoid2sd);
						
					pthread_mutex_unlock(&g_mutex_suoid2sd);
					
					return NULL;
	
				}
				if(DEBUG_CMD_WR) printf("*** 1.2	 \n");
	
				pthread_cond_wait(&g_cond_cmd_que, &g_mutex_cmd_que);
			}
			if(DEBUG_CMD_WR) printf("*** 1.3	 \n");
			e1 = g_cmd_que.front();
			g_cmd_que.pop();
		pthread_mutex_unlock(&(g_mutex_cmd_que));
		
		if(e1.hdr->msg_type != SHRS)
			continue;
		
		if(short_circuit == 1) {
			//Remove the entry from the map
			return NULL;
		}
		if(DEBUG_CMD_WR) printf("*pthread_mutex_lock(&g_mutex_suoid2sd);** 2.		\n");
		msg = (Search_res *)e1.vmsg;
		uint32_t temp, record_len, i = 0;
		char file_uoid[20];
		memcpy(&temp, msg->search_data, 4);
		record_len = ntohl(temp);		
		
		i=4;
		//create a global map which stores the count and the Meta struct for get
		
		if(DEBUG_CMD_WR) printf("*** 3.		\n");
		
		while(record_len != 0) {
			char *meta_data;
			Meta meta_obj; 
			memcpy(file_uoid, msg->search_data + i, 20);
			i = i + 20;
			meta_data = (char *)calloc(record_len, sizeof(char));
			cstr2string(file_uoid, meta_obj.fileid, 20);
			memcpy(meta_data, msg->search_data + i, record_len);
			i = i + record_len;
			string meta_string;
			cstr2string(meta_data, meta_string, record_len);
			vector<string> meta_tokens;
			Tokenize(meta_string, meta_tokens, " =\r\n");
			if(meta_tokens.size() >= 10) {
				
				meta_obj.sha1 = meta_tokens[6];
				printf("[%d]", count);
				printf("\t\tFileID=");
				
				const char *temp_ch;
				temp_ch = meta_obj.fileid.c_str();
				for(int t_i = 0; t_i < 20; t_i++)
					printf("%02x", (unsigned char) temp_ch[t_i]);
				
				printf("\n");
				//meta_tokens[0] = "[metadata]"
				//meta_tokens[1] = "FileName"
				printf("\t\tFileName=%s\n", meta_tokens[2].c_str());
				printf("\t\tFileSize=%s\n", meta_tokens[4].c_str());
				printf("\t\tSHA1=%s\n", meta_tokens[6].c_str());
				printf("\t\tNonce=%s\n", meta_tokens[8].c_str());
				printf("\t\tKeywords=");
				for(uint32_t t_i = 10; t_i < meta_tokens.size(); t_i++) {
					if(meta_tokens[t_i] == "Bit-vector")
						break;
					printf("%s ", meta_tokens[t_i].c_str());
				}
				printf("\n\n");
				//Put the SHA1 on the map along with the file number
				free(meta_data);
			}
			else {
				printf("Cannot understand the metafile\n");
			}	
			memcpy(&temp, msg->search_data + i, 4);
			if(DEBUG_CMD_WR) printf("*** 3.4	 \n");
			record_len = ntohl(temp);
			i = i + 4;
			g_ui2meta[count] = meta_obj;			
			count++;
		}
		if(DEBUG_CMD_WR) printf("*** 4.		\n");
	}
	return NULL;
}


void * cmd_get_res(void *)
{
	Event e1;
	Get_res *msg;
	
	while(1) {
		pthread_mutex_lock(&(g_mutex_cmd_que));
			while(g_cmd_que.size() == 0) {
				if(short_circuit == 1) {
					if(DEBUG_CMD_WR) printf("*** 1.1	 \n");
					pthread_mutex_unlock(&(g_mutex_cmd_que));
					//Please remove the UOID from the map so that no other packets can on the queue
					
					return NULL;
	
				}
				
				pthread_cond_wait(&g_cond_cmd_que, &g_mutex_cmd_que);
			}
			e1 = g_cmd_que.front();
			g_cmd_que.pop();
		pthread_mutex_unlock(&(g_mutex_cmd_que));
		
		if(short_circuit == 1) {
			//Remove the entry from the map
			return NULL;
		}
		
		if(e1.hdr == NULL || e1.vmsg == NULL)
			continue;
			
		if(e1.hdr->msg_type != GTRS)
			continue;
		
		msg = (Get_res *)e1.vmsg;
		string meta_file, data_file;
		temp_file_name(msg->file_no, meta_file, data_file);
		char key[200], val[200];
		if(g_getextfile == "") {
			ssize_t read;	
			ifstream file;
		  	file.open(meta_file.c_str(), ios::in );
		  	
		  	file.getline(val, 200);
			if(strcmp(val, "[metadata]\r")) {
				continue;
			}
			
		 	file.getline(key, 200, '=');
			read = file.gcount();

		  	file.getline(val, 1000);
			read = file.gcount();
			val[read-2] = '\0';
	
			if (strcmp(key, "FileName") != 0) {
				continue;
			}
			g_getextfile = val;
		}
		int curr_size = get_filesize(g_getextfile);
		if(curr_size > -1) {
			printf("This will overwrite the existing file .. Do you want to conitnue ?\n");
			fflush(stdout);
			string response;
			cin>>response;
			if(response[0] == 'y' || response[0] == 'Y') {
			
			}
			else {
				return NULL;
			} 
				
		}
		int sta1 = copy_file(data_file.c_str(), g_getextfile.c_str());
		int sta2 = del_tempfile(msg->file_no);
		if(sta1!=-1 && sta2!=-1) {
			return NULL;
		}
		else {
			printf("Some error occured while getting the file \n");
			return NULL;
		}
	}
	return NULL;
}


/*
void Tokenize(const string& str, vector<string>& tokens, const string& delimiters = " =\"")
{
	// Skip delimiters at beginning.
	string::size_type lastPos = str.find_first_not_of(delimiters, 0);
	// Find first "non-delimiter".
	string::size_type pos	  = str.find_first_of(delimiters, lastPos);

	while (string::npos != pos || string::npos != lastPos)
	{
		// Found a token, add it to the vector.
		tokens.push_back(str.substr(lastPos, pos - lastPos));
		// Skip delimiters.  Note the "not_of"
		lastPos = str.find_first_not_of(delimiters, pos);
		// Find next "non-delimiter"
		pos = str.find_first_of(delimiters, lastPos);
	}
}
*/

void *cmd_time(void *arg)
{
	int *ttl = (int *)arg;
	struct timeval tv;
	tv.tv_sec = *ttl;
	tv.tv_usec = 0;
	select(0, NULL, NULL, NULL, &tv);
	short_circuit = 1;
	pthread_cond_signal(&g_cond_cmd_que);
	return NULL;
}


void* cmd_thread(void *)
{
	fd_set readfds;
	struct timeval tv;
	//printf("\n");
	fflush(stdout);
	int flag = 1;
	int get_allowed = 0;

	while (1) {
		if(flag == 1) {
			printf("servant:%d>", ini.port);
			fflush(stdout);
			flag = 0;
		}

		FD_ZERO(&readfds);
		FD_SET(STDIN_FILENO, &readfds);
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		int select_retval = select(STDIN_FILENO+1, &readfds, NULL, NULL, &tv);
		if (select_retval < 0) {
			//perror("select");
		} 
		else if (select_retval > 0) {
			if (FD_ISSET(STDIN_FILENO, &readfds)) {
				string s = "";

				getline(cin,s);
				//sleep(3);

				if(s.length() == 0)
					continue;				
				vector<string> words;
				Tokenize(s, words, " =\"");
				if(words.size() == 0)
					continue;

				flag = 1;
				if(words[0] == "status") {
					get_allowed = 0;
					if(words.size() == 1)
						continue;
					if(words[1] == "neighbors") {
						if(words.size() < 4)
							continue;
						fflush(stdout);
					
						const char *w = words[2].c_str();
						int t_ttl = atoi(w);
						//! t_ttl++;
						const char *file = words[3].c_str();

						//opening the nam file
						fp_nam = fopen(file, "w");
						fprintf(fp_nam, "V -t * -v 1.0a5\n");
						fprintf(fp_nam, "n -t * -s %d -c red -i black\n", ini.port);
						short_circuit = 0;
						pthread_t cmd_time_t;

						//creating a timer thread
						//pthread_create(&cmd_time_t, NULL, cmd_time, (void *)&ini.jointo);
						pthread_create(&cmd_time_t, NULL, cmd_time, (void *)&ini.msglifetime);
						
						Hdr *hdr;
						void *vmsg;
						status_req(&hdr, &vmsg, NEIGHBOR_INFO, t_ttl);
						Event ev(EV_CMD, SD_STRQ_N, hdr, vmsg); // -1 for cmd_th. -2 for keep_alive_th. -3 for CKRQ
						pthread_mutex_lock(&g_mutex1);
							g_que1.push(ev);
							pthread_cond_signal(&g_cv1);
						pthread_mutex_unlock(&g_mutex1);
						pthread_t cmd_status_neighbor_t;
						void* thread_ret;
						pthread_create(&cmd_status_neighbor_t, NULL, cmd_status_neighbor, NULL);
						pthread_join(cmd_status_neighbor_t, &thread_ret);		
						fclose(fp_nam);
					}
					if(words[1] == "files") {
						if(words.size() < 4)
							continue;
						
						const char *w = words[2].c_str();
						int t_ttl = atoi(w);
						//! t_ttl++;
						const char *file = words[3].c_str();

						//opening the nam file
						fp_stat_file = fopen(file, "w");
						short_circuit = 0;
						pthread_t cmd_time_t;

						//creating a timer thread
						//pthread_create(&cmd_time_t, NULL, cmd_time, (void *)&ini.jointo);
						pthread_create(&cmd_time_t, NULL, cmd_time, (void *)&ini.msglifetime);
						
						Hdr *hdr;
						void *vmsg;
						status_req(&hdr, &vmsg, FILE_INFO, t_ttl);
						Event ev(EV_CMD, SD_STRQ_N, hdr, vmsg); // -1 for cmd_th. -2 for keep_alive_th. -3 for CKRQ
						pthread_mutex_lock(&g_mutex1);
							g_que1.push(ev);
							pthread_cond_signal(&g_cv1);
						pthread_mutex_unlock(&g_mutex1);
						pthread_t cmd_status_file_t;
						void* thread_ret;
						pthread_create(&cmd_status_file_t, NULL, cmd_status_file, NULL);
						pthread_join(cmd_status_file_t, &thread_ret);		
						fclose(fp_stat_file);
					}
				get_allowed = 0;
					
				}
				else if(words[0] == "store"){
					if(words.size() < 3) {
						printf("Incorrect usage\n");
						continue;
					}
					string filename = words[1];
					const char *w = words[2].c_str();
					int t_ttl = atoi(w);
					if(t_ttl == 0)
						continue;
					int32_t t_file_size = get_filesize(filename);
					if(t_file_size == -1) {
						printf("File does not exist\n");
						continue;
					}
					uint32_t file_size = t_file_size;

					pthread_mutex_lock(&g_mutex_temp_fileno);
						int temp_fileno = g_temp_fileno++;
					pthread_mutex_unlock(&g_mutex_temp_fileno);

					//copy the file into the filedirecotry and calculate the sha
					char int_buf[12];
					
					snprintf(int_buf, 11, "%d", temp_fileno);
					string filename_buf = ini.homedir; 
					filename_buf += "files/"; 
					filename_buf += "temp_"; 
					filename_buf += int_buf; 
					filename_buf += ".data";
					
					printf("Storing in Progress .....\n");
					string pass_name = ini.homedir; 
					pass_name += "files/"; 
					pass_name += "temp_"; 
					pass_name += int_buf; 
					pass_name += ".pass";
										
					if(DEBUG_RC1) cout<<"src file: "<< filename << endl;
					if(DEBUG_RC1) cout<<"dst file: "<< filename_buf << endl;
					int rv = copy_file(filename.c_str(), filename_buf.c_str());
					if(rv == -1) {
						printf("Not enough space on the system !!!\n");
						continue;
					}
					//string filename_buf = "cp " + filename + " " + ini.homedir + "files/" + "temp_" + int_buf + ".data";
					//system(filename_buf.c_str());
					//calculate the sha value here ... should we use a string ?
					
					string meta_filename = ini.homedir; 
					meta_filename+= "files/"; 
					meta_filename+= "temp_"; 
					meta_filename+= int_buf; 
					meta_filename+= ".meta";
					if(DEBUG) cout<<"Meta file name = "<<meta_filename<<endl<<flush;	
					FILE *fp;
					fp = fopen(meta_filename.c_str(), "w");
					if(fp==NULL) {
						printf("Could not open the meta file \n");
						continue;
					}
					unsigned char sha1_buf[20], pass[20]; 
					sha1_file((char *)filename.c_str(), sha1_buf);
					fprintf(fp, "[metadata]\r\n");
					fprintf(fp, "FileName=%s\r\n", filename.c_str());
					fprintf(fp, "FileSize=%d\r\n", file_size);
					//Adds the sha value to the meta file
					fprintf(fp, "SHA1=");
					print_sha1(sha1_buf, fp);
					fprintf(fp, "\r\n");
					generate_password(pass);
					generate_nonce(sha1_buf, pass);
					fprintf(fp, "Nonce=");
					print_sha1(sha1_buf, fp);
					fprintf(fp, "\r\n");

					fprintf(fp, "Keywords=");
					unsigned char str_buf[20];
					unsigned char md5_out[16];
					bitvector obj;
					for(uint32_t t_i = 3; t_i < words.size(); t_i++) {
						string s = words[t_i];
						transform(s.begin(), s.end(), s.begin(), (int(*)(int)) std::tolower);
						char *p;
						p = (char *)calloc(s.length(), sizeof(char));
						strcpy(p, s.c_str());
						fprintf(fp,"%s ", p);	
						SHA1((unsigned char *)p, strlen(p), str_buf);
						unsigned int value;
						value = str_buf[18];
						value = value & 1;
						value = value << 8;
						value = value | str_buf[19];
						obj.set(512 + value);
						MD5((unsigned char *)p, strlen(p), md5_out);
						value = md5_out[14];
						value = value & 1;
						value = value << 8;
						value = value | md5_out[15];
						obj.set(value);
					}
					fprintf(fp, "\r\n");
					fprintf(fp, "Bit-vector=");
					obj.print_hex(fp);
					fprintf(fp, "\r\n");
					fclose(fp);
					
					
					fp = fopen(pass_name.c_str(), "w");
					print_sha1(pass, fp);
					fclose(fp);
					
					//creating an event and pushing it on level 3 queue
					
					uint32_t meta_file_size = get_filesize(meta_filename);
					if(DEBUG_RC1) cout<<"Meta data file size = "<<meta_file_size<<endl;
					Hdr *hdr;
					void *vmsg;
					store(&hdr, &vmsg, temp_fileno, t_ttl, meta_file_size, file_size);
					Event ev(EV_CMD, SD_STOR, hdr, vmsg); // -1 for cmd_th. -2 for keep_alive_th. -3 for CKRQ
					pthread_mutex_lock(&g_mutex1);
						g_que1.push(ev);
						pthread_cond_signal(&g_cv1);
					pthread_mutex_unlock(&g_mutex1);

					get_allowed = 0;
					
				}
				else if(words[0] == "search") {
					
					if(words.size() < 3) {
						printf("Incorrect command!!\n");
						continue;
					}
					vector<string> query;
					char type;
					if(words[1] == "filename") {
						//char *file_name;
						//file_name = (char *)calloc(words[2].length(), sizeof(char));
						type = 1; //1 is for the filename
						//strcpy(file_name, words[2].c_str());
						query.push_back(words[2]);
					}
					else if(words[1] == "sha1hash") {
						type = 2;
						unsigned char sha_hash[20];
						if(words[2].length() != 40) {
							printf("Incorrect Sha value\n");
							continue;
						}
						string sha_s = words[2];
						transform(sha_s.begin(), sha_s.end(), sha_s.begin(), (int(*)(int)) std::tolower);
						int t_j = 0, temp_ch;
						for(int t_i = 0; t_i < 40; t_i = t_i+2, t_j++) {
							if(sha_s[t_i] >= '0' && sha_s[t_i] <='9')
								temp_ch = sha_s[t_i] - '0';
							if(sha_s[t_i] >='a' && sha_s[t_i] <= 'f')
								temp_ch = 10 + sha_s[t_i] - 'a';
							sha_hash[t_j] = (unsigned char)temp_ch;
							sha_hash[t_j] = sha_hash[t_j] << 4;
							
							if(sha_s[t_i + 1] >= '0' && sha_s[t_i + 1] <='9')
								temp_ch = sha_s[t_i + 1] - '0';
							if(sha_s[t_i + 1] >='a' && sha_s[t_i + 1] <= 'f')
								temp_ch = 10 + sha_s[t_i + 1] - 'a';
							
							sha_hash[t_j] |= (unsigned char)temp_ch;
						}
					
						sha_s = "";
						if(DEBUG_RC1) cout<<"The converted sha value is 0x";
						for(int t_i = 0; t_i < 20; t_i++) {
							if(DEBUG_RC1) printf("%02x", sha_hash[t_i]);
							sha_s += sha_hash[t_i];
						}
						query.push_back(sha_s);


					}
					else if(words[1] == "keywords") {
						type = 3;
						if(words.size() == 2) {
							printf("Incorrect usage !! Specify the Keywords.\n");
							continue;
						}

						uint32_t t_i;
						for(t_i = 2; t_i < words.size(); t_i++) {
							query.push_back(words[t_i]);
						}

					}
					else {
						printf("Incorrect Usage !!\n");
						continue;
					}
						
					Hdr *hdr;
					void *vmsg;
					search_req(&hdr, &vmsg, type, query); 
					Event ev(EV_CMD, SD_SHRQ, hdr, vmsg); 
					pthread_mutex_lock(&g_mutex1);
						g_que1.push(ev);
						pthread_cond_signal(&g_cv1);
					pthread_mutex_unlock(&g_mutex1);
					short_circuit = 0;
					pthread_t cmd_time_t;
					//creating a timer thread
					//pthread_create(&cmd_time_t, NULL, cmd_time, (void *)&ini.jointo);
					pthread_create(&cmd_time_t, NULL, cmd_time, (void *)&ini.msglifetime);
					
					pthread_t cmd_search_t;
					void* thread_ret;
					pthread_create(&cmd_search_t, NULL, cmd_search_res, NULL);
					pthread_join(cmd_search_t, &thread_ret);		
					get_allowed = 1;
					// 
				}
				else if(words[0] == "get") {
					if(words.size() < 2) {
						printf("Incorrect Usage !!!\n");
						continue;
					}
					if(get_allowed == 0) {
						printf("Please use search before calling get\n");
						continue;
					}
					int ui_no = atoi(words[1].c_str());	
					map<int, struct Meta>::iterator it = g_ui2meta.find(ui_no);
					if(it == g_ui2meta.end()) {
						printf("Incorrect Number\n");
						continue;
					}
					//printf("File ID requested: ");
					//printarrayinhex(it->second.fileid.c_str(), 20);
					char file_id[20];
					memcpy(file_id, it->second.fileid.c_str(), 20);
					
					char sha_hash[20];
					string sha_s = it->second.sha1;
					transform(sha_s.begin(), sha_s.end(), sha_s.begin(), (int(*)(int)) std::tolower);
					int t_j = 0;
					char temp_ch;
					for(int t_i = 0; t_i < 40; t_i = t_i+2, t_j++) {
						if(sha_s[t_i] >= '0' && sha_s[t_i] <='9')
							temp_ch = sha_s[t_i] - '0';
						if(sha_s[t_i] >='a' && sha_s[t_i] <= 'f')
							temp_ch = 10 + sha_s[t_i] - 'a';
						sha_hash[t_j] = (unsigned char)temp_ch;
						sha_hash[t_j] = sha_hash[t_j] << 4;
						
						if(sha_s[t_i + 1] >= '0' && sha_s[t_i + 1] <='9')
							temp_ch = sha_s[t_i + 1] - '0';
						if(sha_s[t_i + 1] >='a' && sha_s[t_i + 1] <= 'f')
							temp_ch = 10 + sha_s[t_i + 1] - 'a';
						
						sha_hash[t_j] |= (unsigned char)temp_ch;
					}
					if(DEBUG_RC1) printf("Sha1 value of requested file is:");
					if(DEBUG_RC1) printarrayinhex(sha_hash, 20);
					
					if(words.size() == 2)
						g_getextfile = "";
					else
						g_getextfile = words[2];
					
					Hdr *hdr;
					void *vmsg;
					
					get_req(&hdr, &vmsg, file_id, sha_hash);
					Event ev(EV_CMD, SD_GTRQ, hdr, vmsg); // -1 for cmd_th. -2 for keep_alive_th. -3 for CKRQ
					pthread_mutex_lock(&g_mutex1);
						g_que1.push(ev);
						pthread_cond_signal(&g_cv1);
					pthread_mutex_unlock(&g_mutex1);
					short_circuit = 0;
					pthread_t cmd_time_t;

					//creating a timer thread
					pthread_create(&cmd_time_t, NULL, cmd_time, (void *)&ini.getmsglifetime);
					
					pthread_t cmd_get_res_t;
					void* thread_ret;
					pthread_create(&cmd_get_res_t, NULL, cmd_get_res, NULL);
					pthread_join(cmd_get_res_t, &thread_ret);	
					get_allowed = 1;
				}
				else if(words[0] == "delete") {
					if(words.size() < 7) {
						printf("Incorrect Usage !!!\n");
						continue;
					}
					
					//printf("File ID requested: ");
					//printarrayinhex(it->second.fileid.c_str(), 20);
					if(words[1] != "FileName" || words[3] != "SHA1" || words[5] != "Nonce") {
						printf("Incorrect usage!!!\n");
						continue;
					}
					string s_sha1 = words[4];
					string s_nonce = words[6];
					
					if(s_sha1.length() != 40 || s_nonce.length() != 40) {
						printf("Incorrect Usage !!!\n");
						continue;
					}
										
					char sha1[20];
					char nonce[20];
					transform(s_sha1.begin(), s_sha1.end(), s_sha1.begin(), (int(*)(int)) std::tolower);
					sha1hex2byte(s_sha1, sha1);
					transform(s_nonce.begin(), s_nonce.end(), s_nonce.begin(), (int(*)(int)) std::tolower);
					sha1hex2byte(s_nonce, nonce);
					string str_sha1, str_nonce;
					
					set<int> s;
					int set_size = g_indice.search(words[2], s_sha1, s_nonce, s);
					char pass_file_name[200];
					char c_passwrd[40];
					char pass[20];
					string s_passwrd;
					int pass_size = 0;
					if(set_size !=0) {
						set<int>::iterator it = s.begin();
						sprintf(pass_file_name, "%s%s%d%s", ini.homedir, "files/", *it, ".pass");
						
						pass_size = get_filesize(string(pass_file_name));
						if(DEBUG_RC1) printf("Size of the passsword file = %d\n", pass_size);
						if(pass_size > 0) {
							FILE *fp = fopen(pass_file_name, "r");
							fread(c_passwrd, 1, 40, fp);
							fclose(fp);
							cstr2string(c_passwrd, s_passwrd, 40);
							sha1hex2byte(s_passwrd, pass);
						}
					}
					if(set_size == 0 || pass_size == -1) {
						printf("No password found for this file do u want to use randon password ?\n");
						string question;
						cin>>question;
						if(question[0] == 'Y' || question[0] == 'y') {
							generate_password((unsigned char *)pass);
						}
						else {
							continue;	
						}
					}
					Hdr *hdr;
					void *vmsg;
					printf("Delete in Progress ....\n");
					del(&hdr, &vmsg, words[2], sha1, nonce, pass);
					Event ev(EV_CMD, SD_DEL, hdr, vmsg); // -1 for cmd_th. -2 for keep_alive_th. -3 for CKRQ
					pthread_mutex_lock(&g_mutex1);
						g_que1.push(ev);
						pthread_cond_signal(&g_cv1);
					pthread_mutex_unlock(&g_mutex1);
					get_allowed = 0;
				}
				
				else if(words[0] == "shutdown") {
						kill_this_node(1);
						exit(1);
				}
				
			}
		} 
		else {
			//printf("Time out :)");
			fflush(stdout);
		}			
	}

	return NULL;
} // end of cmd-thead()


void soft_restart() {
	map<int, Connection* >::iterator it;

	pthread_mutex_lock(&g_mutex_sd2conn);
		//printf("soft_restart():Entering loop....... \n");
		for(it = g_sd2conn.begin(); it != g_sd2conn.end(); it++) {
			pthread_mutex_unlock(&g_mutex_sd2conn);
				pthread_cond_signal(&(it->second->cv));
				pthread_join(it->second->rth, NULL);
				pthread_join(it->second->wth, NULL);		

			pthread_mutex_lock(&g_mutex_sd2conn);

			delete it->second;
			it->second = NULL;
		}
		g_sd2conn.clear();		
		//printf("soft_restart():Exiting loop....... \n");
	pthread_mutex_unlock(&g_mutex_sd2conn);

	pthread_mutex_lock(&g_mutex_hostid2sd);
		g_hostid2sd.clear();
	pthread_mutex_unlock(&g_mutex_hostid2sd);
}

int main(int argc, char *argv[]) {
	// Init global variables
	init_globals();

	// Process command line arguments
	read_command_line(argc, argv);

	// Initialize mutexes
	init_mutexes();

	// Read ini file. Determine if I'm a beacon
	g_beacon = init(argv[argc-1]);

	// 
	sigset_t signal_set;
	pthread_t thread_id;
	sigfillset( &signal_set);
	pthread_sigmask(SIG_BLOCK, &signal_set, NULL);
	pthread_create(&thread_id, NULL, sig_handler, NULL);

	//alarm(ini.autoshutdown);
	pthread_t shut_thread;
	pthread_create(&shut_thread, NULL, auto_shutdown, NULL);

	// Open log file
	string log_file = "";
	log_file = string(ini.homedir) + "/" + string(ini.logfile);

	// Initialize indice and LRU
	g_indice.init_home(ini.homedir);
	g_lru.init_home(ini.homedir);
	g_lru.init_quota(ini.cachesize);

	if(g_reset) {
		//Delete index file & LRU file in home_dir
		char rm_cmd[200];
		sprintf(rm_cmd, "/bin/rm -f %skwrd_index", ini.homedir);
		if(DEBUG_RM) printf("Removing %s\n", rm_cmd);
		system(rm_cmd);

		sprintf(rm_cmd, "/bin/rm -f %sname_index", ini.homedir);
		if(DEBUG_RM) printf("Removing %s\n", rm_cmd);
		system(rm_cmd);

		sprintf(rm_cmd, "/bin/rm -f %ssha1_index", ini.homedir);
		if(DEBUG_RM) printf("Removing %s\n", rm_cmd);
		system(rm_cmd);

		sprintf(rm_cmd, "/bin/rm -f %sfileid_index", ini.homedir);
		if(DEBUG_RM) printf("Removing %s\n", rm_cmd);
		system(rm_cmd);

		sprintf(rm_cmd, "/bin/rm -f %snonce_index", ini.homedir);
		if(DEBUG_RM) printf("Removing %s\n", rm_cmd);
		system(rm_cmd);

		sprintf(rm_cmd, "/bin/rm -f %sfileno2perm_index", ini.homedir);
		if(DEBUG_RM) printf("Removing %s\n", rm_cmd);
		system(rm_cmd);

		sprintf(rm_cmd, "/bin/rm -f %slru.txt", ini.homedir);
		if(DEBUG_RM) printf("Removing %s\n", rm_cmd);
		system(rm_cmd);

		//Delete all files in mini filesystem.
		sprintf(rm_cmd, "/bin/rm -f %sfiles/*", ini.homedir);
		if(DEBUG_RM) printf("Removing %s\n", rm_cmd);
		system(rm_cmd);

		g_log_file = fopen(log_file.c_str(), "w");
	}
	else {
		// Read index files.
		g_indice.read_index_file();

		// Read LRU status file.
		g_lru.read_lru_file();

		// Read fileid index file
		read_fileid_index_file(g_fileid2fileno, g_fileno2fileid, ini.homedir, g_fileno, g_temp_fileno);
		
		// Read fileno2perm index file
		read_fileno2perm_index_file(g_fileno2perm, ini.homedir);

		// Open log file
		g_log_file = fopen(log_file.c_str(), "a");
	}
	
	if(!g_log_file) {
		print_error_and_exit("Err. cannot open log file");
	}
	
	bool first_run = true;
	while(g_time2die == 3) { //@  3: self restart
		if(DEBUG) printf("Self restarting....\n");
		g_time2die = -1;	 //@ -1: normal
		g_curr_neighbor= 0;

		pthread_t keep_alive_thread_id;
		//pthread_create(&keep_alive_thread_id, NULL, keep_alive_th, NULL);

		// if: I'm a beacon node
		if(g_beacon) {

			pthread_t cmd_thread_id;
			pthread_create(&cmd_thread_id, NULL, cmd_thread, NULL);
			pthread_create(&keep_alive_thread_id, NULL, keep_alive_th, NULL);
			beacon_main();
		}
		// else: I'm a regular node
		else {
			// Read beacon list and store it in g_beacon_list 
			if(DEBUG_MAIN) printf("### nonbeacon 1.\n");
			for(unsigned int i = 0; i < ini.beacon_name.size(); i++) {
				Node_info node(ini.beacon_name[i], ini.beacon_port[i]);		
				g_beacon_list.add_node(node);
			}
		
			// Explore neighbors and store closest ones in g_neighbor_list
			if(DEBUG_MAIN) printf("### nonbeacon 2. Exploring neighbors...\n");
			explore_neighbors();
		
			if(DEBUG_MAIN) printf("### nonbeacon 3.\n");
			if (nonbeacon_connect() == -1) {
				if(g_reset == false) {
					g_time2die = 3;
					g_reset = true;				
					//pthread_join(keep_alive_thread_id, NULL);
					continue;
				}
				else
					break;
			}
			pthread_create(&keep_alive_thread_id, NULL, keep_alive_th, NULL);

			// Create dispatch thread	
			if(DEBUG_MAIN) printf("### nonbeacon 4.\n");
			if( first_run == true) {
				first_run = false;

				pthread_t se_th;
				pthread_t dis_thread;
				
				pthread_t cmd_thread_id; // Final pt1. line chage 2
				pthread_create(&cmd_thread_id, NULL, cmd_thread, NULL);

				pthread_create(&dis_thread, NULL, dispatch_th, NULL);
				if(DEBUG) printf("dispatch thread created.\n");	

				// Create server thread
				pthread_create(&se_th, NULL, server_thread, NULL);
				if(DEBUG) printf("server thread created..................\n");				
			}
			// pthread_join(dis_thread, NULL);
			// pthread_join(se_th, NULL);
		}

		pthread_join(keep_alive_thread_id, NULL);
		
		soft_restart();
	}

	// Clean up
	fclose(g_log_file);
	destroy_mutexes();

	return 0;
}

