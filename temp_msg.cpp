#include "includes.h"
#include "msg.h"
#include "error.h"
#include "globals.h"

// function send_msg
//	 Send a msg through a socket
// arguments
//	 sd:	 socket descriptor
//	 hdr:  ptr to the header
//	 msg:  ptr to the msg to be sent
//	 Make sure we are sending the packets ready ... this is just putting it on a pipe ...(do put hdr.data_len)
// return
//	 0 if success


void *keep_alive_th()
{
    struct timeval tv;
    tv.tv_sec = ini.keepaliveto / 2;
    tv.tv_usec = 0;
    Connection *conn;    
	while(1) {
		select(0, NULL, NULL, NULL, tv);
		pthread_mutex_lock(&g_mutex_hostid2sd);
		{
			if(DEBUG_PRO_EV) printf("//**  1\n");
			std::map<string, int>::iterator itr;

			// For every sd currently available,
			for(itr = g_hostid2sd.begin(); itr != g_hostid2sd.end(); itr++) {
				int sd = itr->second;
				pthread_mutex_lock(&g_mutex_sd2conn);
				{
				    conn = sd2conn[sd];
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
	}
	return NULL;
}

void nonbeacon_connect()
{
//global current number of neighbors ..curr_neighbors
	struct sockaddr_in my_addr;
	unsigned int i;
	int sockfd;
	hostent *h;
	my_addr.sin_family = AF_INET;
	curr_neighbors = 0;

	for(i = 0; i< g_neighbor_list.nodes.size(); i++) {
			
		string peerid = "";
		peerid += g_neighbor_list.nodes[i].hostname; 
		peerid += "_";
		stringstream out_stream;
		out_stream << g_neighbor_list.nodes[i].port;
		string s_temp = out_stream.str();
		peerid += s_temp;
		
		

		my_addr.sin_port = ini.beacon_port[i];
		const char *buffer = ini.beacon_name[i].c_str();
		
		if((h = gethostbyname(buffer)) == NULL) {
			perror("Address Request");
			continue;
		}
		memcpy(&my_addr.sin_addr, h->h_addr, h->h_length);
		memset(my_addr.sin_zero, '\0', sizeof(my_addr.sin_zero));

		// Create a socket
		if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
			perror("ERR nonbeacon_connect():socket()");
			exit(1);
		}
		// Connect to the neighbouring node
		if(connect(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1) {
			perror("ERR nonbeacon_connect():connect()");
			fflush(stdout);	
			continue;
			//exit(1);
		}
			
		// Spawn read/write thrread.
		start_connection(sockfd, -1); //-1 for not beacon
		curr_neighbor++;

		// Post a HELLO msg.
		
		pthread_mutex_lock(&g_mutex_hostid2sd);
			g_hostid2sd[peerid] = sockfd;
		pthread_mutex_unlock(&g_mutex_hostid2sd);
	
		pthread_mutex_lock(&g_mutex_sd2conn);
		{
			Connection *conn = g_sd2conn[sockfd];
			conn->peerid = peerid;
		}
		pthread_mutex_unlock(&g_mutex_sd2conn);
			
		send_hello(sockfd);
	
	}
	if(curr_neighbor < ini.minneighbor)
		return -1;
	return 0; //0 is for sucess and that  we have more than min neighbors connected to us.
} // end of non_beacon_connect()


}

void send_hello(int sd)
{
	pthread_mutex_lock(&g_mutex_sd2conn);
	{
		conn = g_sd2conn[sd];
	}
	pthread_mutex_unlock(&g_mutex_sd2conn);
	
	Hdr *hdr;
	void *vmsg;
	hello(hdr, vmsg);
	Event ev(EV_MSG, sd, hdr, vmsg);
	
	pthread_mutex_lock(&(conn.mutex));
	conn.que.push_back(ev);
	pthread_mutex_unlock(&(conn.mutex));
	
	Port_sd port_sd(ini.port, sd);
	string host(ini.hostname);

	pthread_mutex_lock(&(g_mutex_str2port_sd));	
	g_str2port_sd[host] = port_sd;	
	pthread_mutex_unlock(&(g_mutex_str2port_sd));	
	delete hdr;
	delete (Hello *)vmsg;
	return;
}


int send_msg(int sock_fd, Hdr *hdr, void* vmsg) {
	
	char *buf;
	char *message;
	uint32_t temp;
	uint16_t stemp;	
	int result = 0;
	fd_set wset;
	struct timeval time;
	FD_ZERO(&wset);
	FD_SET(sock_fd,&wset);
	time.tv_sec = 30;
	time.tv_usec = 0;
	result = select(sock_fd + 1, NULL, &wset, NULL, &time);
	if(!result) {
		shutdown(sock_fd, SHUT_RDWR);
		close(sock_fd);
		return -1;
	}
	buf = (char *)calloc(HEADER_LEN, sizeof(char));
	//obtained the header .. its now time to parse it
	buf = (char *)hdr;
	temp = hdr->data_len;
	hdr->data_len = ntohl(temp);
	message = (char *)calloc(hdr->data_len, sizeof(char));

//TODO: Not using any buffer ... writing all the data in one go ... will have to change it eventually :)		

	switch(hdr->msg_type) {
		case 0xFC: //Join Message
		{
			Join_req *msg = (Join_req *)vmsg;
			temp = htonl(msg->host_loc);
			memcpy(message, &temp, sizeof(temp));
			stemp = htons(msg->host_port);
			memcpy(message + 4, &stemp, sizeof(stemp));
			memcpy(message + 6, msg->host_name, hdr->data_len - 6);
		}		
		break;
		
		case 0xFB: //Join Response
		{
			Join_res *msg = (Join_res *)vmsg;
			memcpy(message, msg->UOID, 20);
			temp = htonl(msg->distance);
			memcpy(message + 20, &temp, sizeof(temp));
			stemp = htons(msg->host_port);
			memcpy(message + 24, &stemp, sizeof(stemp));
			memcpy(message + 26, msg->host_name, hdr->data_len - 26);
		}
		break;

		case 0xFA: //Hello Message
		{
			Hello *msg = (Hello *)vmsg;
			stemp = htons(msg->host_port);
			memcpy(message, &stemp, sizeof(stemp));
			memcpy(message + 2, msg->host_name, hdr->data_len - 2);
		}		
		break;

		case 0xF8: //Keep alive
			message = NULL;
		break;

		case 0xF7: //Notify
		{
			Notify *msg = (Notify *)vmsg;
			message[0] = msg->error;
		}
		break;

		case 0xF6: //Check request
			message = NULL;
		break;

		case 0xF5: //Check response
		{
			Check_res *msg = (Check_res *)vmsg;
			memcpy(message, msg->UOID, 20);
		} 
		break;

		case 0xEC: //search request
		break;

		case 0xEB: //search response
		break;

		case 0xDC: //get request
		break;

		case 0xDB: //get response
		break;

		case 0xCC: //store
		break;

		case 0xBC: //delete
		break;

		case 0xAC: //status request
		{		
			Status_req *msg = (Status_req *)vmsg;
			message[0] = msg->type; 
		}				
		break;

		case 0xAB: //status response
		{
			Status_res *msg = (Status_res *)vmsg;
			memcpy(message, msg->UOID, 20);
			stemp = htons(msg->hostinfo);
			memcpy(message + 20, &stemp, sizeof(stemp));
			stemp = htons(msg->host_port);
			memcpy(message + 22, &stemp, sizeof(stemp));
			memcpy(message + 24, msg->host_name, msg->hostinfo - 2);
			temp = htonl(msg->record_len);
			memcpy(message + 22 + msg->hostinfo, &temp, sizeof(temp));
			memcpy(message + 22 + msg->hostinfo + 4, msg->data, msg->record_len);
		}
		break;

		default :
			print_error("Message type not recognised");
			return -1;
	}

	if(send(sock_fd, buf, HEADER_LEN, 0) == -1) {
		perror("writing2soc:");
		return -1;
	}

	if(send(sock_fd, message, hdr->data_len, 0) == -1) {
		perror("writing2soc:");
		return -1;
	}

	return 0;  

}


// function recv_msg
//	 Receive a msg through a socket
//	 Allocate memory for both the objects in this function
//	 Deletion ??
// arguments
//	 sd:	 socket descriptor
//	 msg:  ptr to the msg to be received and filled
// return
//	 0 if success
//
int recv_msg(int sd, Hdr* hdr, void* vmsg) {
	char buf[HEADER_LEN];
	char *message;
	uint32_t temp;
	uint16_t stemp;	
	fd_set rset;
	timeval time;
	int result = ERR;

	FD_ZERO(&rset);
	FD_SET(sd, &rset);
	time.tv_sec = 30;
	time.tv_usec = 0;

	// Check if data is ready
	result = select(sd + 1, &rset, NULL, NULL, &time);
	if(!result) {
		shutdown(sd, SHUT_RDWR);
		close(sd);
		return -1;
	}

	// Receive Common Header
	if((result = recv(sd, buf, HEADER_LEN,0)) == -1) {
		perror("Reading Sock:");
		return -1;
	}

	// Obtained the header .. its now time to parse it
	hdr = (Hdr *)buf;
	temp = hdr->data_len;
	hdr->data_len = ntohl(temp);
	message = (char *)calloc(hdr->data_len, sizeof(char));

//TODO: Not using any buffer ... reading all the data in one go ... will have to change it eventually :)		

	// Receive message body
	if((result = recv(sd, message, hdr->data_len, 0)) == -1) {
		printf("Reading Sock:");
		return -1;
	}
		
	switch(hdr->msg_type) {
		case 0xFC: // Join Request Message
		{
			Join_req* msg = new Join_req;
			memcpy(&temp, message, sizeof(temp));
			msg->host_loc = ntohl(temp);
			memcpy(&stemp, message + 4, sizeof(stemp));
			msg->host_port = ntohs(stemp);

			// Store NULL terminated host name
			msg->host_name = (char *)calloc(hdr->data_len - 6 + 1, sizeof(char)); 
			memcpy(msg->host_name, message + 6, hdr->data_len - 6);
			msg->host_name[hdr->data_len - 6] = '\0'; 
			vmsg = (void*)msg;
		}
		break;
		
		case 0xFB: // Join Response
		{
			Join_res* msg = new Join_res;
			memcpy(msg->UOID, message, 20);
			memcpy(&temp, message + 20, sizeof(temp));
			msg->distance = ntohl(temp);
			memcpy(&stemp, message + 24, sizeof(stemp));
			msg->host_port = ntohs(stemp);
							
			// Store NULL terminated host name
			msg->host_name = (char *)calloc(hdr->data_len - 26 + 1, sizeof(char)); 
			memcpy(msg->host_name, message + 26, hdr->data_len - 26);
			msg->host_name[hdr->data_len - 26] = '\0'; 
			vmsg = (void*)msg;
		}
		break;
		case 0xFA: //Hello Message
		{								 
			Hello *msg = new Hello;
			memcpy(&stemp, message, sizeof(stemp));
			msg->host_port = ntohs(stemp);
			msg->host_name = (char *)calloc(hdr->data_len - 2, sizeof(char)); //adding one char for NULL
			memcpy(msg->host_name, message + 2, hdr->data_len - 2);
			msg->host_name[hdr->data_len - 2] = '\0'; //host_name can now be used as a string :)
			vmsg = (void*) msg;
		}		
		break;
	
		case 0xF8: //Keep alive
			vmsg = NULL;
		break;

		case 0xF7: //Notify
		{
			Notify *msg = new Notify;
			msg->error = message[0];
			vmsg = (void *)msg;
		} 
		break;

		case 0xF6: //Check request
			vmsg = NULL;
		break;

		case 0xF5: //Check response
		{
			Check_res *msg = new Check_res;
			memcpy(msg->UOID, message, 20);
			vmsg = (void *)msg;
		}
		break;

		case 0xEC: //search request
		break;

		case 0xEB: //search response
		break;

		case 0xDC: //get request
		break;

		case 0xDB: //get response
		break;

		case 0xCC: //store
		break;

		case 0xBC: //delete
		break;

		case 0xAC: //status request
		{
			Status_req *msg = new Status_req;
			msg->type = message[0];
			vmsg = (void *)msg; 
		}
		break;
		
		case 0xAB: //status response
		{
			Status_res *msg = new Status_res;
			memcpy(msg->UOID, message, 20);
			memcpy(&stemp, message + 20, sizeof(stemp));
			msg->hostinfo = ntohs(stemp);
			memcpy(&stemp, message + 22, sizeof(stemp));
			msg->host_port = ntohs(stemp);
			msg->host_name = (char *)calloc(msg->hostinfo - 2 + 1, sizeof(char));
			memcpy(msg->host_name, message + 24, msg->hostinfo - 2);
			msg->host_name[msg->hostinfo] = '\0';
			memcpy(&temp, message + 22 + msg->hostinfo, sizeof(temp));
			msg->record_len = ntohl(temp);
			msg->data = (char *)calloc(msg->record_len + 1, sizeof(char));
			memcpy(msg->data, message + 22 + msg->hostinfo + 4, msg->record_len);
			msg->data[msg->record_len] = '\0';
			vmsg = (void *)msg;
		}		
		break;

		default : 
			if(DEBUG) printf("Message type not recognised");
			return -1;
	}

	return 0;
}

 
// function join_req
//	 Build a join msg
//
int join_req(Hdr* hdr, void* msg)
{	
	return 0;
}
