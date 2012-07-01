// dispatch.cpp

#include "dispatch.h"
#include "globals.h"
#include "util.h"
#include "error.h"
#include "indice.h"
#include "lru.h"



void* ckrs_timer( void *)
{
	struct timeval tv;
	tv.tv_sec = ini.jointo;
	tv.tv_usec = 0;
	
	select(0, NULL, NULL, NULL, &tv);
	
	if(g_is_connected) {
		;
	}
	else {
		g_time2die = 3;
	}
	
	return NULL;
}

bool is_cache_available(int file_no) {
	bool store_it = false;
	
	string temp_fname_meta;
	string temp_fname_data;						
	temp_file_name(file_no, temp_fname_meta, temp_fname_data);
	
	unsigned long meta_size = get_filesize(temp_fname_meta);
	unsigned long data_size = get_filesize(temp_fname_data);

	// Try to push it into LRU queue
	vector<int> victims;
	store_it = g_lru.push(g_fileno, meta_size + data_size, victims);

	// Delete victim files
	vector<int>:: iterator itr;
	for(itr = victims.begin(); itr != victims.end(); itr++) {
		int fileno = *itr;
		int err = 0;
	
		string file_name;
		string file_sha1;
		string file_nonce;

		err = read_meta_file(fileno, file_name, file_sha1, file_nonce);
		if(!err) {
			err = del_file(fileno);							
		}
		if(!err) {
			g_fileno2perm.erase(fileno);
			g_indice.erase(file_name, file_sha1, file_nonce);
			g_fileid2fileno.erase(g_fileno2fileid[fileno]);
			g_fileno2fileid.erase(fileno);
		}
	}
				
	return store_it;
}


void save_in_filesystem(int file_no, bool is_permanent) {
	// Store the file to the mini file system.
	int err = copy_tempfile2mini(file_no);			
	if(!err) {
	
		char uoid[20];
		char obj_type[] = "file";
		getuoid(ini.node_inst_id, obj_type, uoid, 20);
		string fileid;
		cstr2string(uoid, fileid, 20);

		g_fileid2fileno[fileid] = g_fileno;
		g_fileno2fileid[g_fileno] = fileid;
		g_fileno2perm[g_fileno] = is_permanent; // Permanant or cache
		g_indice.read_meta_file(g_fileno);
		g_fileno++;
	}
}

bool process_event(Event& ev1, std::queue<Event>& tmp_que){
	Hdr* hdr2 = NULL;
	void* vmsg2 = NULL;
	bool double_conn_closed = false;
	bool flood_it = false;
	bool store_it = true;

	switch(ev1.hdr->msg_type) {
		case HLLO:
		{
			Hello* msg1 = (Hello*) ev1.vmsg;

			// Retrieve sender's host_id from Hello msg.
			string hostid = msg1->host_name; 
			hostid += "_";
			stringstream out_stream;
			out_stream << msg1->host_port;
			string s_temp = out_stream.str();
			hostid += s_temp;
			
			// set the peer id
			pthread_mutex_lock(&g_mutex_sd2conn);
			{
				std::map<int, Connection* >::iterator sd2conn_iter;
				sd2conn_iter = g_sd2conn.find(ev1.sd);
				if(sd2conn_iter == g_sd2conn.end()) { 
					pthread_mutex_unlock(&g_mutex_sd2conn);
					return false;
				}

				Connection *conn = g_sd2conn[ev1.sd];
				conn->peerid = hostid;
			}
			pthread_mutex_unlock(&g_mutex_sd2conn);

			// Check if it's a reply to HELLO I sent.
			pthread_mutex_lock(&g_mutex_hostid2sd);
				map<string, int>::iterator itr = g_hostid2sd.find(hostid);

				// if match, drop it.
				if(itr != g_hostid2sd.end()) {
					if(ev1.sd != itr->second) {
						//printf("Double Connection found !!\n");
						if(ini.port > msg1->host_port) {
							//printf("The connection initiated be me wins\n");
							close(ev1.sd);
						}
						else if(ini.port < msg1->host_port) {
							//printf("The connection initiated by the peer wins\n");
							close(itr->second);
							itr->second = ev1.sd;

						}
						else {
							if((strcmp(ini.hostname, msg1->host_name)) > 0) {
								//printf("The connection initiated by me wins ... tough the port is same\n");
							close(ev1.sd);
							}
							else {
								//printf("The connection initiated by the peer wins ... tough the port is same\n");
								close(itr->second);
								itr->second = ev1.sd;
							}
						}
						double_conn_closed = true;
					}
/*
					// if double connection deleted,
					// Delete an entry in sd2conn map
					if(double_conn_closed) {
						pthread_mutex_lock(&g_mutex_sd2conn);
							g_sd2conn.erase(ev1.sd);
						pthread_mutex_unlock(&g_mutex_sd2conn);
						double_conn_closed = false;
					}
*/
				}
				// else, post Hello as a reply
				else {
					hello(&hdr2, &vmsg2);		
					Event ev2(EV_MSG, ev1.sd, hdr2, vmsg2, 's');
					tmp_que.push(ev2);
					if(hostid != "_0") {
						g_hostid2sd[hostid] = ev1.sd;
					}
				}
			pthread_mutex_unlock(&g_mutex_hostid2sd);

		}
		break;
		
		case KPAV: 
		{
			if(DEBUG_PR_EV) printf("***** 0.3	 \n");				
			if(DEBUG) printf("//**	process_event(): case KPAV\n");

			//! if(ev1.hdr->ttl == (unsigned char) 2) { // Keep alive should be generated with TTL = 2.
			//! if(ev1.hdr->ttl == (unsigned char) 1) { // Keep alive should be generated with TTL = 1.
			if(ev1.sd < 0) {
				flood_it = true;
				if(DEBUG_PRO_EV)  printf("//** Will flood KPAV.\n");
			}
			else {
				flood_it = false;
			}
		}
		break;
			
		case STOR: // Store
		{ 		
			// Take UOID from the header
			string uoid;
			cstr2string(ev1.hdr->uoid, uoid, UOID_LEN);
			Store* msg = (Store*)ev1.vmsg;
			
			pthread_mutex_lock(&g_mutex_suoid2sd);
			{
				// look up UOID
				map<string, int>::iterator itr1 = g_suoid2sd.find(uoid);

				// if you never received this msg before
				if(itr1 == g_suoid2sd.end()) {
					// Associate uoid with socket
					g_suoid2sd[uoid] = ev1.sd;

					// Will flood msg.
					//if(ev1.hdr->ttl > 1) { // 
					if(ev1.hdr->ttl > 1 || (ev1.hdr->ttl > 0 && ev1.sd < 0)) { // 
						flood_it = true;
					}

					int file_no = msg->file_no;

					// Detect for same files.
					string filename;
					string sha1;
					string nonce;
					set<int> same_files;
					
					read_temp_meta_file(file_no, filename, sha1, nonce);
					g_indice.search(filename, sha1, nonce, same_files);
					if(same_files.empty()) {
						store_it = true;
						
						// if STOR is a flooded msg, check cache availability
						if(ev1.sd > 0) {
							if(store_it = flip_coin(ini.storeprob)) {	// Flip coin to store in cache
								store_it = is_cache_available(file_no);
							}
						}
					
						// Store the file in the mini filesystem.
						if(store_it) {
							bool is_permanent = (ev1.sd < 0);	// permanent storage or cache?
							save_in_filesystem(file_no, is_permanent);
						}
					}
				}
				// if you have received this msg before
				else {
					flood_it = false; 			// No flood any more.
				}
			}
			pthread_mutex_unlock(&g_mutex_suoid2sd);	
		}
		break;			
		
		case DELT: // Delete
		{
			// Take UOID from the header
			string uoid;
			cstr2string(ev1.hdr->uoid, uoid, UOID_LEN);
			Del* msg = (Del*)ev1.vmsg;
			
			pthread_mutex_lock(&g_mutex_suoid2sd);
			{
					// look up UOID
				map<string, int>::iterator itr1 = g_suoid2sd.find(uoid);

				if(DEBUG_RC1) cout << "In dispatcher............." << endl;
				// if you never received this msg before
				if(itr1 == g_suoid2sd.end()) {
					// Associate uoid with socket
					g_suoid2sd[uoid] = ev1.sd;

					// Will flood msg.
					if(ev1.hdr->ttl > 1) { // 
						flood_it = true;
					}
					
					unsigned char nonce[20];
					generate_nonce(nonce, (unsigned char*) msg->password);
					
					if(DEBUG_RC1) printf("Nonce generated ...\n");
					if(DEBUG_RC1) printarrayinhex((char *)msg->password, 20);
					if(DEBUG_RC1) printarrayinhex((char *)nonce, 20);
					
					if(strncmp((char*)nonce, msg->nonce, 20) == 0) {					
						set<int> files;
						string file_sha1;
						string file_nonce;
						sha1byte2hex(msg->sha1, file_sha1); // byte20 -> hex40
						sha1byte2hex(msg->nonce, file_nonce); // byte20 -> hex40
						
						if(DEBUG_RC1) cout << "In dispatcher............." << endl;
						if(DEBUG_RC1) cout << msg->filename << endl;
						if(DEBUG_RC1) cout << file_sha1 << endl;
						if(DEBUG_RC1) cout << file_nonce << endl;
						
						g_indice.search(msg->filename, file_sha1, file_nonce, files);

						set<int>::iterator itr;
						for(itr = files.begin(); itr != files.end(); itr++) {
							int fileno = *itr;
							int err = del_file(fileno);
							if (!err) {
								g_fileno2perm.erase(fileno);
								g_indice.erase(msg->filename, file_sha1, file_nonce);
								g_lru.erase(fileno);
								g_fileid2fileno.erase(g_fileno2fileid[fileno]);
								g_fileno2fileid.erase(fileno);
							}
						}
					}
					else { // password mismatch.
						flood_it = true; 			// No flood any more.
					}
				}
				else { // if you have received this msg before
					flood_it = false; 			// No flood any more.
				}	
			}
			pthread_mutex_unlock(&g_mutex_suoid2sd);	
		}
		break;	
		
		case JNRQ: // Join Request
		case STRQ: // Status Request
		case CKRQ:
		case SHRQ:
		case GTRQ:
		{
			if(ev1.hdr->msg_type == JNRQ) {
				if(DEBUG_PR_EV) printf("***** 0.4	 \n");				
				Join_req* msg1 = (Join_req*) ev1.vmsg; 

				// Retrieve sender's host_id from Join msg. 
				string hostid = msg1->host_name;  
				hostid += "_"; 
				stringstream out_stream; 
				out_stream << msg1->host_port; 
				string s_temp = out_stream.str(); 
				hostid += s_temp; 
				 
				// set the peer id 
				pthread_mutex_lock(&g_mutex_sd2conn); 
				{ 
					std::map<int, Connection* >::iterator sd2conn_iter; 
					sd2conn_iter = g_sd2conn.find(ev1.sd); 

					// Associate sd to connection if new request has been received. 
					if(sd2conn_iter != g_sd2conn.end()) {  
						Connection *conn = g_sd2conn[ev1.sd]; 
						if(conn->peerid.empty()) {
							conn->peerid = hostid; 
						}
					} 
					else {
						pthread_mutex_unlock(&g_mutex_sd2conn); 
						return false; 
					}
				} 
				pthread_mutex_unlock(&g_mutex_sd2conn);
			}
		
			if(DEBUG_PR_EV) printf("***** 0.5	 \n");				
			// Take UOID from the header
			string uoid;
			cstr2string(ev1.hdr->uoid, uoid, UOID_LEN);
			
			pthread_mutex_lock(&g_mutex_suoid2sd);
			{
				if(DEBUG_RC1) printf("GTRQ uoid is:");
				if(DEBUG_RC1) printarrayinhex(uoid.c_str(), 20);

				// look up UOID
				map<string, int>::iterator itr1 = g_suoid2sd.find(uoid);

				// if never received this msg before, process it.
				if(itr1 == g_suoid2sd.end()) {
					// Associate uoid with socket
					g_suoid2sd[uoid] = ev1.sd;

					// Will flood Request msg.
					//if(ev1.hdr->ttl > 1) { // 
					if(ev1.hdr->ttl > 1 || (ev1.hdr->ttl > 0 && ev1.sd < 0)) { // 
						flood_it = true;
					}

					// Create Response msg.
					switch(ev1.hdr->msg_type) 
					{
						case JNRQ:
							join_res(&hdr2, &vmsg2, ev1.hdr, ev1.vmsg);
							//printf("JNRS message created.\n");
							break;
							
						case STRQ: 
						{
							Status_req* msg1 = (Status_req*)ev1.vmsg;
							if(DEBUG_RC1) printf("STRQ received.\n");

							if (msg1->type == 0x02) { // for STRQ-FILE
								if(DEBUG_RC1) printf("STRQ received inside iff.\n");
								
								vector<int> fileno_vec;
								map<int, bool>::iterator itr;
								for (itr = g_fileno2perm.begin(); itr != g_fileno2perm.end(); itr++){
									fileno_vec.push_back(itr->first);
									if(DEBUG_RC1) printf("Adding element to a vector\n");
								}
								status_res_files(&hdr2, &vmsg2, ev1.hdr, fileno_vec);
							}
							else if (msg1->type == 0x01) {	// for STRQ-NEIGHBOR
								pthread_mutex_lock(&g_mutex_hostid2sd);
									create_status_res(&hdr2, &vmsg2, ev1.hdr);
								pthread_mutex_unlock(&g_mutex_hostid2sd);
							}

							break;
						}
						case CKRQ:
							if(g_beacon) {
								check_res(&hdr2, &vmsg2, ev1.hdr);
								//printf("CKRS message created.\n");
								flood_it = false;
							}
							break;
							
						case SHRQ:
							{
								set<int> file_no_set;
								Search_req* msg1 = (Search_req*) ev1.vmsg;
								//Search_res* msg2 = (Search_res*)vmsg2;
								
								g_indice.search(msg1->search_type, msg1->query, file_no_set);
								search_res(&hdr2, &vmsg2, ev1.hdr, ev1.vmsg, file_no_set);
								
								if(file_no_set.size()) { // if search succeed, stop flooding.
									flood_it = true;
								}
							}						
							break;
						case GTRQ:
							{
								Get_req* msg1 = (Get_req*) ev1.vmsg;
								string fileid;
								cstr2string(msg1->file_id, fileid, 20);
								
								if(DEBUG_RC1) printf("requested fileid is:");
								if(DEBUG_RC1) printarrayinhex(fileid.c_str(), 20);

								// if file exists.								
								map<string, int>::iterator itr;
								itr = g_fileid2fileno.begin();								

								itr = g_fileid2fileno.find(fileid);								
								if(itr != g_fileid2fileno.end()) {
									// stop flooding
									flood_it = false;
									int err = copy_mini2tempfile(itr->second);
									if(err == 0) {
										get_res(&hdr2, &vmsg2, ev1.hdr, ev1.vmsg, itr->second);
									}
								}
								else {
									// flooding
									flood_it = true;								
								}		
							}
							break;
					}
					
					// If response msg has been created,
					// Wrap it with L2 event. Push it into L2 queue
					if(hdr2 && vmsg2) {
						Event ev2(ev1.type, ev1.sd, hdr2, vmsg2, 's');			
						//g_suoid2sd[uoid] = ev1.sd; // redundant
						tmp_que.push(ev2);
						if(DEBUG_PRO_EV) printf("//** Response %02x pushed\n", (unsigned char) ev2.hdr->msg_type);
					}
				}
				// if you have received this msg already, drop it.
				else {
					flood_it = false; // Drop it. No flood any more.
				}
			}
			pthread_mutex_unlock(&g_mutex_suoid2sd);	
		}
		break;

		case JNRS: // Join Response
		case STRS: // Status Response
		case CKRS: // Check Response
		case SHRS: // Search Response
		case GTRS: // Get Response
		{
			string uoid;
			// Get "request" msg's UOID stored in response msg.
			Status_res *msg = (Status_res *)ev1.vmsg;
			cstr2string(msg->uoid, uoid, UOID_LEN);
			if(DEBUG_PR_EV) printf("***** 0.6	 \n");				

			pthread_mutex_lock(&g_mutex_suoid2sd);
			{
				map<string, int>::iterator itr = g_suoid2sd.find(uoid);

				//! if have received "request" msg before, forward it back to the requester.
				if(itr != g_suoid2sd.end() && ev1.hdr->ttl > 1) {
					// Copy Response msg to be fowarded. TTL-- inside.
					dup_msg(&hdr2, &vmsg2, ev1.hdr, ev1.vmsg);

					// For ease of debugging. :-)
					//Search_res* msg1 = (Search_res*) ev1.vmsg;
					//Search_res* msg2 = (Search_res*) vmsg2;

					hdr2->ttl--; //
					Event ev2(ev1.type, itr->second, hdr2, vmsg2, 'f'); // Set sd to the one I received request. 

					// Push it to L2 queue
					tmp_que.push(ev2);
					//g_suoid2sd.erase(itr); // Delete sd in the map.

					// for GTRS only
					if(ev1.hdr->msg_type == GTRS) {
						if(DEBUG_RC1) printf("GTRS received.\n");
						Get_res* msg = (Get_res*)ev1.vmsg;
						int file_no = msg->file_no;
						
						// Detect for same files.
						string filename;
						string sha1;
						string nonce;
						set<int> same_files;
					
						read_temp_meta_file(file_no, filename, sha1, nonce);
						g_indice.search(filename, sha1, nonce, same_files);
						if(same_files.empty()) {
							store_it = true;	// default behavior is "store."
							// if GTRS is a flooded msg, check cache availability
							if(itr->second > 0) {
								if(store_it = flip_coin(ini.cacheprob)) {	// Flip coin
									store_it = is_cache_available(file_no);
								}
							}
				
							// Store the file in the mini filesystem.
							if(store_it) {
								//bool is_permanent = (ev1.sd < 0);	// permanent storage or cache?
								bool is_permanent = (itr->second < 0);	// permanent storage or cache?
								save_in_filesystem(file_no, is_permanent);
							}
						}
					}
				}
				// else, drop it.
				else {
					//printf("Response not found in map !! Drop it.\n");
					; // Drop it.
				}
			}
			pthread_mutex_unlock(&g_mutex_suoid2sd);
		}
		break;

		case NTFY: // Notify
		{
			g_is_connected = false;
			
			if(DEBUG_PR_EV) printf("***** 0.7	 \n");				
/*
			string uoid;
			cstr2string(ev1.hdr->uoid, uoid, UOID_LEN);
			pthread_mutex_lock(&g_mutex_hostid2sd);			
				sd = g_hostid2sd[uoid];
				g_hostid2sd.erase();
			pthread_mutex_unlock(&g_mutex_hostid2sd);			
*/

			Connection* conn = NULL;
			string peerid = "";
			pthread_mutex_lock(&g_mutex_sd2conn);
				conn = g_sd2conn[ev1.sd];
				peerid = conn->peerid;
			pthread_mutex_unlock(&g_mutex_sd2conn);

			pthread_mutex_lock(&g_mutex_hostid2sd);			
				g_hostid2sd.erase(peerid);				
			pthread_mutex_unlock(&g_mutex_hostid2sd);			
	
			if(!g_beacon && !ini.nocheck) {
				flood_it = true;	// Will flood Check_req message.
				if(DEBUG_PRO_EV)  printf("//** Will flood %02x.\n", (unsigned char) ev1.hdr->msg_type);

				g_is_connected = false;
				alarm(ini.jointo); // Alarm for CHRS timeout.
				pthread_t ckrs_t;
				pthread_create(&ckrs_t, NULL, ckrs_timer, NULL);
			}
		}
		break;
	
		default:
			flood_it = false;
		break;
	}
		
	return flood_it;
}

void flood_msg(Event& ev1, std::queue<Event>& tmp_que) {
	Hdr* hdr2 = NULL;
	void* vmsg2 = NULL;

	// Flood msg.
	if(DEBUG_PRO_EV) printf("//** before flooding\n");
	int num_forwarded = 0;

	bool forward_it = true;	
	
	// STOR: Reset file ref counter.
	if(ev1.hdr->msg_type == STOR) {
		Store* s = (Store*) ev1.vmsg;
		
		pthread_mutex_lock(&g_mutex_fileno2ref);
			g_fileno2ref[s->file_no] = 0;
		pthread_mutex_unlock(&g_mutex_fileno2ref);
	}

	pthread_mutex_lock(&g_mutex_hostid2sd);
	{
		if(DEBUG_PRO_EV) printf("//**  1\n");
		std::map<string, int>::iterator itr;								
		
		// For every sd currently available,
		for(itr = g_hostid2sd.begin(); itr != g_hostid2sd.end(); itr++) {
			int sd = itr->second;
			
			// Should not forward back to the sender... 
			if(sd == ev1.sd)
				continue;
				
			// For some messages, we need to flip a coin to decide forwarding.
			if(ev1.hdr->msg_type == STOR) {
				forward_it = flip_coin(ini.nstoreprob);
			}

			if(forward_it) {
				
				//pthread_mutex_lock(&g_mutex_sd2conn); //RC3. No need?
				{
					// Pre-process: Dupliate msg.
					if(ev1.hdr->msg_type == NTFY) {
						char uoid[20];
						char obj_type[] = "msg";
						getuoid(ini.node_inst_id, obj_type, uoid, 20);					
			
						hdr2 = new Hdr(CKRQ, uoid, 255, 0, 0);   // For Notify msg, flood Check_req
						vmsg2 = NULL;
					}
					else {
						dup_msg(&hdr2, &vmsg2, ev1.hdr, ev1.vmsg); // For other msgs, duplicate msg.

						if(ev1.sd > 0) //! L1 event was received from socket
							hdr2->ttl--;
					}
			
					// Create L2 event
					Event ev2(EV_MSG, sd, hdr2, vmsg2);

					if(ev1.hdr->msg_type == NTFY) {
						g_suoid2sd[ev2.hdr->uoid] = SD_CKRQ;	// -1 for cmd_th. -2 for keep_alive_th. -3 for CKRQ
					}

					// Post process: Set send/forward flag.
					if(ev1.sd < 0) {
						ev2.rfs = 's';											
					}
					else {
						ev2.rfs = 'f';						
					}

					// Push it into temporary queue
					tmp_que.push(ev2);
					num_forwarded++;
					
					// STOR: increase file ref counter
					if(ev1.hdr->msg_type == STOR) {
						Store* s = (Store*) ev1.vmsg;

						pthread_mutex_lock(&g_mutex_fileno2ref);
							g_fileno2ref[s->file_no] += 1;
						pthread_mutex_unlock(&g_mutex_fileno2ref);
					}
				}
				//pthread_mutex_unlock(&g_mutex_sd2conn); //RC3
			}
		}
	}
	pthread_mutex_unlock(&g_mutex_hostid2sd);

	if( (ev1.hdr->msg_type == STOR) && (num_forwarded == 0) ) {		
		Store* msg = (Store*)ev1.vmsg;
		del_tempfile(msg->file_no);	// Delete temp files that read_th() has created
	}

	if(DEBUG_PR_EV) printf("***** 3.	 \n");	
	return;
} // end of flood_msg()


void* dispatch_th(void* arg) {
	Event ev1; // event for level-1 queue
	Event ev2; // event for level-2 queue

	std::queue<Event> tmp_que;	// temporary internal queue for outgoing event
	Connection *conn = NULL;;

	while(1) {		
		// Read incoming event from Level-1 queue
		if(DEBUG_DIS_TH) printf("** 0.	   \n");
		pthread_mutex_lock(&g_mutex1);
		{
			while(g_que1.size() == 0) {
				if(DEBUG_DIS_TH) printf("** 0.1		\n");
				pthread_cond_wait(&g_cv1, &g_mutex1);
			}
		
			if(DEBUG_DIS_TH) printf("** 0.2		\n");
			// Pop L1 Queue
			ev1 = g_que1.front();
			g_que1.pop();
			if(DEBUG_DIS_TH) printf("** 0.3		\n");			
		}
		pthread_mutex_unlock(&g_mutex1);

		if(g_time2die > -1) {  
			continue;
		}

		// Process incoming-event, build outgoing-events, 
		// and store them in dispatcher's temporary queue.
		if(DEBUG_DIS_TH) printf("** 0.4		\n");			
		bool flood_it = process_event(ev1, tmp_que);

		// Log incoming-event msg
		pthread_mutex_lock(&g_mutex_log_msg);
			log_msg(g_log_file, ev1.hdr, ev1.vmsg, 'r', ev1.sd);
		pthread_mutex_unlock(&g_mutex_log_msg);

		if (flood_it) {
			flood_msg(ev1, tmp_que);
		}
		else {
			if(ev1.hdr->msg_type == STOR) {
				Store *msg = (Store *)ev1.vmsg;
				del_tempfile(msg->file_no);
			}
			continue;
		}


		if(DEBUG_DIS_TH) printf("** 0.5		\n");			
		
		// For all response messages generated by process_event()
		int num_ev = tmp_que.size();
		for(int i = 0; i < num_ev; i++) {
			// Pop an event from temp queue
			ev2 = tmp_que.front();
			tmp_que.pop();

			if(g_time2die > -1) {  
				continue;
			}
			
			// For Response messages coming back to the user command line.
			if(ev2.sd == SD_STRQ_N || ev2.sd == SD_SHRQ || ev2.sd == SD_GTRQ) {
				if(DEBUG_GOOD) printf(" ---------------------------------------------\n");
				ev2.type = EV_CMD;
			}
			
			if(ev2.sd == SD_CKRQ) {
				g_is_connected = true;
				continue;
			}
			
			if(DEBUG_DIS_TH) printf("** 1.	   \n");
			
			if(ev2.type == EV_CMD) {
				pthread_mutex_lock(&g_mutex_cmd_que);
				{
					// Push the CMD event into the level-2 CMD queue
					if(DEBUG) printf("Pushing CMD event into CMD queue\n");
					if(DEBUG_DIS_TH) printf("** 1.1		\n");
					g_cmd_que.push(ev2);
					if(DEBUG_DIS_TH) printf("** 1.2		\n");
					pthread_cond_signal(&g_cond_cmd_que);
					if(DEBUG_DIS_TH) printf("** 1.3		\n");
				}
				pthread_mutex_unlock(&g_mutex_cmd_que);
			}
			else if(ev2.type == EV_MSG) {
				pthread_mutex_lock(&g_mutex_sd2conn);
				{
					// Given sock, find cooresponding Level-2 MSG queue
					if(DEBUG_DIS_TH) printf("** 2.	   \n");
					std::map<int, Connection* >::iterator sd2conn_iter;
					sd2conn_iter = g_sd2conn.find(ev2.sd);
					
					// if Level-2 MSG queue is not found, destroy ev2
					if(sd2conn_iter == g_sd2conn.end()) {
						if(DEBUG_DIS_TH) printf("dispatch(): Dropping ev2.\n");
						ev2.cleanup();
						pthread_mutex_unlock(&g_mutex_sd2conn);
						continue;
					}
				
					conn = g_sd2conn[ev2.sd];
				}
				pthread_mutex_unlock(&g_mutex_sd2conn);				

				// Push the MSG event into the Level-2 MSG queue
				if(DEBUG_DIS_TH) printf("** 3.	   \n");
				pthread_mutex_lock(&(conn->mutex));
				{
					conn->que.push(ev2);
					pthread_cond_signal(&(conn->cv));
				}
				pthread_mutex_unlock(&(conn->mutex));
			}
		}

		// Destroy level-1 event
		if(DEBUG_DIS_TH) printf("** 4.	   \n");
		ev1.cleanup();
		if(DEBUG_DIS_TH) printf("** 5.	   \n");
	} // while

	return NULL;
} // end of dispatch_th


