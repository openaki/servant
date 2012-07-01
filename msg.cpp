#include "includes.h"
#include "msg.h"
#include "error.h"
#include "constants.h"
#include "startup.h"
#include "event.h"
#include "util.h"
#include<set>

extern Startup ini;
extern FILE* g_log_file;

extern map<int, Connection* > g_sd2conn;
extern pthread_mutex_t g_mutex_sd2conn;

extern map<string, Port_sd> g_hostname2port_sd;
extern pthread_mutex_t g_mutex_hostname2port_sd; 

extern map<string, int> g_hostid2sd;
extern pthread_mutex_t	g_mutex_hostid2sd;

extern map<int, int> g_fileno2ref;
extern pthread_mutex_t g_mutex_fileno2ref;


extern pthread_mutex_t	g_mutex_log_msg;

extern int g_temp_fileno;
extern pthread_mutex_t g_mutex_temp_fileno;

extern map<int, string> g_fileno2fileid;

int getuoid(char *node_inst_id, char *obj_type, char *uoid_buf, unsigned int uoid_buf_size)
{
	static unsigned long seq_no = (unsigned long)1;
	char sha1_buf[SHA_DIGEST_LEN] , str_buf[104];
	snprintf(str_buf, sizeof(str_buf), "%s_%s_%1ld", node_inst_id, obj_type, (long)seq_no++);
	SHA1((unsigned char *)str_buf, strlen(str_buf), (unsigned char *)sha1_buf);
	memset(uoid_buf, 0, uoid_buf_size);
	memcpy(uoid_buf, sha1_buf, min(uoid_buf_size, sizeof(sha1_buf)));
	return 1;
}


//Hdr::Hdr(char _type, char* _uoid, char _ttl, char _reserved, uint32_t _len) 
Hdr::Hdr(char _type, char* _uoid, unsigned char _ttl, char _reserved, uint32_t _len) 
{
	msg_type = _type;
	memcpy(uoid, _uoid, 20);
	ttl = _ttl;
	reserved = _reserved;
	data_len = _len;
}


Join_req::Join_req(uint32_t _loc, uint16_t _port, char* _name) 
{
	host_loc = _loc; 
	host_port = _port; 
	host_name = _name;
}

Join_req::~Join_req() 
{
	if(DEBUG) printf(" ~Join_req()\n");
	//delete host_name;
	free(host_name);
	host_name = NULL;
}


Join_res::Join_res(char* _uoid, uint32_t _dist, uint16_t _port, char* _name) 
{
	memcpy(uoid, _uoid, 20); 
	distance = _dist;
	host_port = _port; 
	host_name = _name;
}

Join_res::~Join_res() 
{
	if(DEBUG) printf(" ~Join_res()\n");
//#	free(host_name);					  // Why commented ???
//#	host_name = NULL;
}


Hello::Hello(uint16_t _port, char* name) {
	host_port = _port;
	host_name = name;
}

Hello::~Hello() {
	//delete host_name;
	free(host_name);
	host_name = NULL;
}


Notify::Notify(char _error) {
	error = _error;
}


Check_res::Check_res(char *_uoid) 
{
	memcpy(uoid, _uoid, 20);
}


Status_req::Status_req(char _type)
{
	type = _type;
}


Status_res::Status_res(char* _uoid, uint16_t _hostinfo, uint16_t _host_port, char *_host_name, uint32_t _record_len, char *_data) 
{
	//if(!_uoid) printf("Status-res(): _uoid is NULL!\n");
	memcpy(uoid, _uoid, 20);
	//if(!_uoid) printf("Status-res(): copied NULL pointer!\n");
	hostinfo = _hostinfo;
	host_port = _host_port;
	host_name = _host_name;
	//record_len = _record_len;
	data = _data;
}

Status_res::~Status_res() 
{
	if(DEBUG) printf(" ~Status-res()\n");	
	//delete host_name;
	free(host_name);
	host_name = NULL;
	//delete data;
	free(data);
	data = NULL;
}

Search_req::Search_req(char _search_type, vector<string> &_query) {
	search_type = _search_type;
	query = _query;
}

Search_req::~Search_req() {
	//delete query;
	//free(query);
	//query = NULL;
	//Vector calls its own distructor :)
}

Search_res::Search_res(char* _uoid, char* _search_data) {
	if(_uoid) {
		memcpy(uoid, _uoid, 20);
	}
	else {
		memset(uoid, '0', 20);	
	}
	//record_len = _record_len; // Deleted by Akshat.
	
	search_data = _search_data;
}

Search_res::~Search_res() {
	//delete metadata;
	free(search_data);
	search_data = NULL;
}

Get_req::Get_req(char* _file_id, char* _sha1) {
	if(_file_id) {
		memcpy(file_id, _file_id, 20);
	}
	else {
		memset(file_id, '0', 20);
	}
	
	if(_sha1) {
		memcpy(sha1, _sha1, 20);
	}
	else {
		memset(sha1, '0', 20);	
	}
}

Get_req::~Get_req() {
}

Get_res::Get_res(char* _uoid, uint32_t _metadata_len, uint32_t _file_no) {
	if(_uoid) {
		memcpy(uoid, _uoid, 20);
	}
	else {
		memset(uoid, '0', 20);
	}
	metadata_len = _metadata_len;
	file_no = _file_no;
}

Get_res::~Get_res() {
	//delete metadata;
	//free(metadata);
	//metadata = NULL;
}

/*
Store::Store(uint32_t _metadata_len, char* _metadata, uint32_t _file_no) {
	metadata_len = _metadata_len;
	metadata = _metadata;
	file_no = _file_no;
}
*/

Store::Store(uint32_t _metadata_len, uint32_t _file_no) {
	metadata_len = _metadata_len;
	file_no = _file_no;
	if(DEBUG_RC1) cout<<"File no. in the constructor"<<file_no<<endl;
}

Store::~Store() {
	//delete metadata;
	//free(metadata);
	//metadata = NULL;
}


Del::Del(string _filename, char* _sha1, char *_nonce, char *_password) {
	filename = _filename;
	if(_sha1) {
		memcpy(sha1, _sha1, 20);
	}
	else {
		memset(sha1, '0', 20);
	}
	if(_nonce) {
		memcpy(nonce, _nonce, 20);
	}
	else {
		memset(nonce, '0', 20);
	}
	if(_password) {
		memcpy(password, _password, 20);
	}
	else {
		memset(password, '0', 20);
	}	
}

Del::~Del() {
	//delete file_spec;
}

Status::Status(char _status_type) {
	status_type = _status_type;
}

Status::~Status() {
}

Meta::Meta(string _fileid, string _sha1) {
	fileid = _fileid;
	sha1 = _sha1;
}

// function log_err
//	 prints error / warning on the log file
// argument
//	 fp: file pointer
//	 str: string to print
//	 err: true:  error.   "**" will be printed at the beginning of the message.
//		  false: warning. "//" will be printed at the beginning of the message.
//
int log_err(FILE* fp, string str, bool err) {
	if(!fp) // skip internally generated msg. i.e., cmd_th. keep_alive_th
		return -1;

	if(err) {	// for Error messages
		fprintf(fp, "** ");
	}
	else {		// for Debugging messages
		fprintf(fp, "// ");	
	}

	fprintf(fp, "%s \n", str.c_str());
	fflush(fp); 
	return 0;
}


// function log_msg
//	 print message log on file
// argument
//	 fp:		file to print
//	 hdr:		common header
//	 vmsg:	message body
//	 rfs:		'r' message received
//					'f' message forwarded
//					's' message sent
//
int log_msg(FILE* fp, Hdr* hdr, void* vmsg, char rfs, int sd) 
{
	if(sd < 0 || !fp) // skip internally generated msg. i.e., cmd_th. keep_alive_th
		return 0;

	char buff1[40] = ""; // for r/f/s, time	, from/to
	char buff2[5] = "";  // for msg_type
	char buff3[40] = ""; // for size, ttl, msgid
	char buff4[1000] = ""; // for data
	bool valid_msg = true;
 
	timeval time;
	long sec = 0;
	short msec = 0;	
	unsigned long size = 0;
	short ttl = 0;
	string peerid = "";

	if(DEBUG_LOGMSG) printf("**LOG MSG** 0.		\n");				
	gettimeofday(&time, NULL);	
	sec = time.tv_sec;
	msec = time.tv_usec / 1000;
	size = sizeof(Hdr) + hdr->data_len;
	ttl = (short)hdr->ttl;

	if(DEBUG_LOGMSG) printf("**LOG MSG** 2.		\n");				

	// Retrieve peerid
	if(sd == 0){		// if socket is not given, get peerid directly from the hdr & msg.
		get_peerid_from_msg(hdr, vmsg, peerid);	
	}
	else				// else, consult sd2conn map.
	{
		pthread_mutex_lock(&g_mutex_sd2conn);	
			map<int, Connection* >::iterator itr = g_sd2conn.find(sd);
			if(itr != g_sd2conn.end()) {		
				Connection* conn = g_sd2conn[sd];
				peerid = conn->peerid;
			}
		pthread_mutex_unlock(&g_mutex_sd2conn);
	}
	
	if(DEBUG_LOGMSG) printf("**LOG MSG** peerid = %s\n", peerid.c_str());				
	if(peerid == "") {		
		peerid = " unknown hostid ";
		//return 0;		
	}

	if(DEBUG_LOGMSG) printf("**LOG MSG** 3.		\n");				
	
	
	sprintf(buff1, "%c %10ld.%03d %s", rfs, sec, msec, peerid.c_str());
	sprintf(buff3, "%lu %3d %02x%02x%02x%02x", size, ttl,
			(unsigned char) hdr->uoid[16], (unsigned char) hdr->uoid[17],
			(unsigned char) hdr->uoid[18], (unsigned char) hdr->uoid[19] );
	
	if(DEBUG_LOGMSG) printf("**LOG MSG** 4.		\n");				
	switch(hdr->msg_type) {
		case JNRQ: // Join Req
		{
			Join_req* msg = (Join_req*) vmsg;
			strcpy(buff2, "JNRQ");
			sprintf(buff4, "%d %s", msg->host_port, msg->host_name);
		}
		break;
		case JNRS: // Join Res
		{
			Join_res* msg = (Join_res*) vmsg;
			strcpy(buff2, "JNRS");
			sprintf(buff4, "%02x%02x%02x%02x %d %d %s", 
				(unsigned char) msg->uoid[16], (unsigned char) msg->uoid[17], 
				(unsigned char) msg->uoid[18], (unsigned char) msg->uoid[19], 
				msg->distance, msg->host_port, msg->host_name);
		}
		break;

		case HLLO: // Hello
		{
			Hello* msg = (Hello*) vmsg;
			strcpy(buff2, "HLLO");
			sprintf(buff4, "%d %s", msg->host_port, msg->host_name);		
		}
		break;
		
		case KPAV:
		{
			strcpy(buff2, "KPAV");
		}
		break;

		case NTFY: // Notify
		{
			Notify* msg = (Notify*) vmsg;
			strcpy(buff2, "NTFY");
			sprintf(buff4, "%x", msg->error);		
		}	
		break;
		
		case CKRQ: // Check_req
		{
			strcpy(buff2, "CKRQ");		
		}
		break;
		
		case CKRS: // Check_res
		{
			Check_res* msg = (Check_res*) vmsg;
			strcpy(buff2, "CKRS");
			sprintf(buff4, "%02x%02x%02x%02x", 
				(unsigned char) msg->uoid[16], (unsigned char) msg->uoid[17], 
				(unsigned char) msg->uoid[18], (unsigned char) msg->uoid[19]);			
		}
		break;

		case STRQ: // Status Request
		{
			Status_req* msg = (Status_req*) vmsg;
			strcpy(buff2, "STRQ");
			sprintf(buff4, "%x", msg->type);
		}
		break;

		case STRS: // Status Response
		{
			Status_res* msg = (Status_res*) vmsg;
			strcpy(buff2, "STRS");
			sprintf(buff4, "%02x%02x%02x%02x", 
				(unsigned char) msg->uoid[16], (unsigned char) msg->uoid[17], 
				(unsigned char) msg->uoid[18], (unsigned char) msg->uoid[19]); 
		}
		break;
		
		case STOR: // Store Response
		{
			strcpy(buff2, "STOR");
		}
		break;
		
		case SHRQ: // Search request
		{
			Search_req* msg = (Search_req*) vmsg;
			strcpy(buff2, "SHRQ");

			string queries;
			vector<string>::iterator itr;
			for(itr = msg->query.begin(); itr != msg->query.end(); itr++) {
				queries += *itr;
				queries += string(" ");
			}
			sprintf(buff4, "%d %s", msg->search_type, queries.c_str());
		}
		break;

		case SHRS: // Search response
		{
			Search_res* msg = (Search_res*) vmsg;
			strcpy(buff2, "SHRS");
			
			sprintf(buff4, "%02x%02x%02x%02x", 
				(unsigned char) msg->uoid[16], (unsigned char) msg->uoid[17], 
				(unsigned char) msg->uoid[18], (unsigned char) msg->uoid[19]); 
		}
		break;

		case GTRQ: // Get request
		{
			Get_req* msg = (Get_req*) vmsg;
			strcpy(buff2, "GTRQ");
			
			sprintf(buff4, "%02x%02x%02x%02x", 
				(unsigned char) msg->file_id[16], (unsigned char) msg->file_id[17], 
				(unsigned char) msg->file_id[18], (unsigned char) msg->file_id[19]); 
		}
		break;

		case GTRS: // Get response
		{
			Get_res* msg = (Get_res*) vmsg;
			strcpy(buff2, "GTRS");
			
			sprintf(buff4, "%02x%02x%02x%02x", 
				(unsigned char) msg->uoid[16], (unsigned char) msg->uoid[17], 
				(unsigned char) msg->uoid[18], (unsigned char) msg->uoid[19]); 
		}
		break;

		case DELT: // Delete Response
		{
			strcpy(buff2, "DELT");
		}
		break;

		default:
			valid_msg = false;
		break;
	}
	
	if(DEBUG_LOGMSG) printf("**LOG MSG** 5.		\n");				

	if(valid_msg) {	
		if(DEBUG_LOGMSG) printf("log-msg(): msg 0x%02x logged.\n", (unsigned char) hdr->msg_type);

		if(LOGFILE_PRINTS_PORT) {
			fprintf(fp, "%s %s %s %s soc=%d\n", buff1, buff2, buff3, buff4, sd);
		}
		else {
			fprintf(fp, "%s %s %s %s\n", buff1, buff2, buff3, buff4);		
		}
	}
	else {
		fprintf(fp, "// log-msg(): Unkonwn message type: 0x%02x **\n", (unsigned char) hdr->msg_type); 
	}
	fflush(fp);

	return 0;
} // end of log-msg()


// function send_msg
//	 Send a msg through a socket
// arguments	
//	 sd:	 socket descriptor
//	 hdr:  ptr to the header
//	 msg:  ptr to the msg to be sent
//	 Make sure we are sending the packets ready ... this is just putting it on a pipe ...(do put hdr.data_len)
// return
//	 0 if success
int send_msg(int sock_fd, Hdr *hdr, void* vmsg)
{
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

    // Fill the buffer for msg
    temp = htonl(hdr->data_len);
    memcpy(buf, &hdr->msg_type, sizeof(hdr->msg_type));
    memcpy(buf + 1, hdr->uoid, 20);
    memcpy(buf + 21, &hdr->ttl, sizeof(hdr->ttl));
    memcpy(buf + 22, &hdr->reserved, sizeof(hdr->reserved));
    memcpy(buf + 23, &temp, sizeof(hdr->data_len));

	if(hdr->msg_type == STOR) {
		Store *msg = (Store *)vmsg;
		message = (char *)calloc(4, sizeof(char));
		temp = htonl(msg->metadata_len);
		memcpy(message, &temp, sizeof(temp));
		if((result = send(sock_fd, buf, HEADER_LEN, 0)) == -1)
			return -1;
		
		if((result = send(sock_fd, message, 4, 0)) == -1)
			return -1;
	        
	    uint32_t temp_fileno = msg->file_no;
		//cout<<"Temp file no "<<temp_fileno<<endl;
		//cout<<"ini filepath"<<ini.file_path<<endl;
		char int_buf[12];
		snprintf(int_buf, 11, "%d", temp_fileno);
		string temp_fname = ini.file_path + "temp_" + int_buf;
		string meta_filename = temp_fname + ".meta";
		string file_name = temp_fname + ".data";
		//cout << "meta_filename:" <<meta_filename << endl;
		FILE *fp;
		fp = fopen(meta_filename.c_str(), "r");
		if(!fp) {
			printf("Open failed.\n");
			return -1;
		}
		int sta;
		sta = send_file(fp, sock_fd);
		fclose(fp);
		if(sta != 0)
			return -1;
		
		fp = fopen(file_name.c_str(), "r");
		sta = send_file(fp, sock_fd);	
		fclose(fp);
		if(sta!= 0)
			return -1;	
		pthread_mutex_lock(&g_mutex_fileno2ref);
        	g_fileno2ref[temp_fileno]--;
			if(g_fileno2ref[temp_fileno] == 0) {
				del_tempfile(temp_fileno);
				g_fileno2ref.erase(temp_fileno);
			}
		pthread_mutex_unlock(&g_mutex_fileno2ref);

		
		return 0;
	}
	if(hdr->msg_type == GTRS) {
	
		Get_res *msg = (Get_res *)vmsg;
		message = (char *)calloc(24, sizeof(char));
		memcpy(message, msg->uoid, 20);
		temp = htonl(msg->metadata_len);
		memcpy(message + 20, &temp, sizeof(temp));
		if(DEBUG_RC1) printf("Host order%d --- Network order%u\n", msg->metadata_len, temp);
		if((result = send(sock_fd, buf, HEADER_LEN, 0)) == -1)
			return -1;
		
		if((result = send(sock_fd, message, 24, 0)) == -1)
			return -1;
	        
	    string meta_file, data_file;
	    temp_file_name(msg->file_no, meta_file, data_file);
	    
	    FILE *fp;
	    fp = fopen(meta_file.c_str(), "r");
		if(DEBUG_RC1) printf("Meta file has been opened \n");
		if(!fp) {
			printf("Open failed.\n");
			return -1;
		}
		int sta;
		sta = send_file(fp, sock_fd);
		fclose(fp);
		if(sta != 0)
			return -1;
		
		if(DEBUG_RC1) printf("Data file is opened \n");
		fp = fopen(data_file.c_str(), "r");
		sta = send_file(fp, sock_fd);	
		fclose(fp);
		if(sta!= 0)
			return -1;	
		
		//There can be only one path through which a get_res packet can travel
		//So only one thread can send it at a time ... No need to store it in a map
		del_tempfile(msg->file_no);
		if(DEBUG_RC1) printf("Hope fully the message has been sent \n");
		return 0;
	}	
		
	//obtained the header .. its now time to parse it
	message = (char *)calloc(hdr->data_len, sizeof(char));

	//TODO: Not using any buffer ... writing all the data in one go 
	//... will have to change it eventually :)		

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
			memcpy(message, msg->uoid, 20);
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
			memcpy(message, msg->uoid, 20);
		} 
		break;

		case 0xEC: //search request
		{
			Search_req *msg = (Search_req *)vmsg;
			message[0] = msg->search_type;
			int j = 1;
			uint32_t i = 0;
			for(i = 0; i< msg->query.size() - 1; i++) {
			
				memcpy(message + j, msg->query[i].c_str(), msg->query[i].length());
				j+=msg->query[i].length();
				memcpy(message + j, " ", 1);
				j++;
			}
				memcpy(message + j, msg->query[i].c_str(), msg->query[i].length());
				j+=msg->query[i].length();
		}
		break;

		case 0xEB: //search response
		{
			Search_res *msg = (Search_res *)vmsg;
			memcpy(message, msg->uoid, 20);
			memcpy(message + 20, msg->search_data, hdr->data_len - 20);			
		
		}
		break;

		case 0xDC: //get request
		{
			Get_req *msg = (Get_req *)vmsg;
			memcpy(message, msg->file_id, 20);
			memcpy(message + 20, msg->sha1, 20);
		}
		break;

		case 0xDB: //get response
		break;

		case 0xCC: //store
		break;

		case 0xBC: //delete
		{
			Del *msg = (Del *)vmsg;
			int filename_len = hdr->data_len - 97;
			//filename = (char *)calloc(filename_len + 1, sizeof(char));
			memcpy(message, "FileName=", 9);
			memcpy(message + 9, msg->filename.c_str(), filename_len);
			memcpy(message + 9 + filename_len, "\r\n", 2);
			memcpy(message + 9 + filename_len + 2, "SHA1=", 5);
			memcpy(message + 9 + filename_len + 2 + 5, msg->sha1, 20);
			memcpy(message + 9 + filename_len + 2 + 5 + 20, "\r\n", 2);
			memcpy(message + 9 + filename_len + 2 + 5 + 22, "Nonce=", 6);
			
			memcpy(message + 9 + filename_len + 2 + 5 + 22 + 6, msg->nonce, 20);
			memcpy(message + 9 + filename_len + 2 + 5 + 22 + 6 + 20, "\r\n", 2);
			memcpy(message + 9 + filename_len + 2 + 5 + 22 + 6 + 22, "Password=", 9);
			memcpy(message + 9 + filename_len + 2 + 5 + 22 + 6 + 22 + 9, msg->password, 20);
			memcpy(message + 9 + filename_len + 2 + 5 + 22 + 6 + 22 + 9 + 20, "\r\n", 2);
			//Total number comes out to be 97 + filename_len
		}
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
			memcpy(message, msg->uoid, 20);
			stemp = htons(msg->hostinfo);
			memcpy(message + 20, &stemp, sizeof(stemp));
			stemp = htons(msg->host_port);
			memcpy(message + 22, &stemp, sizeof(stemp));
			memcpy(message + 24, msg->host_name, msg->hostinfo - 2);
			int record_len = hdr->data_len - 22 - msg->hostinfo;
			memcpy(message + 22 + msg->hostinfo, msg->data, record_len);
		}
		break;

		default :
			//printf("send_msg(): Unknown Message type 0x%02x \n", (unsigned char) hdr->msg_type);
			return -1;
	}

/*
	// Fill the buffer for msg
	temp = htonl(hdr->data_len);
	memcpy(buf, &hdr->msg_type, sizeof(hdr->msg_type));
	memcpy(buf + 1, hdr->uoid, 20);
	memcpy(buf + 21, &hdr->ttl, sizeof(hdr->ttl));
	memcpy(buf + 22, &hdr->reserved, sizeof(hdr->reserved));
	memcpy(buf + 23, &temp, sizeof(hdr->data_len));
*/
	// Send header
	if((result = send(sock_fd, buf, HEADER_LEN, 0)) == -1) {
		//printf("ERR send_msg():writing %02x to soc %d\n", (unsigned char) hdr->msg_type, sock_fd);
		fflush(stdout);
		//perror("ERR send_msg():writing to soc");
		return -1;
	}

	// Send message body
	if((result = send(sock_fd, message, hdr->data_len, 0)) == -1) {
		//printf("ERR send_msg():writing %02x to soc %d\n", (unsigned char) hdr->msg_type, sock_fd);
		fflush(stdout);
		//perror("ERR send_msg():writing to soc");
		return -1;
	}

	if(DEBUG) printf("send_msg(): msg 0x%02x sent.\n", (unsigned char) hdr->msg_type);

	return 0;  
} // end of send_msg()


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
int recv_msg(int sd, Hdr** hdr_p, void** vmsg_p) 
{
	Hdr *hdr;
	void *vmsg;
	char *buf;
	char *message;
	uint32_t temp;
	uint16_t stemp;	
	fd_set rset;
	timeval time;
	int result = 0;

	FD_ZERO(&rset);
	FD_SET(sd, &rset);
	time.tv_sec = ini.keepaliveto;
	time.tv_usec = 0;
	
	// Allocate a buffer for common header
	buf = (char*) calloc(HEADER_LEN, sizeof(char));
	
	// Check if data is ready
	//while(result != 1) {
		result = select(sd + 1, &rset, NULL, NULL, &time);
	//}
	if(result == 0) {
		return -1;
	}

	if(result == -1) {
		shutdown(sd, SHUT_WR);
		close(sd);
		return -1;
	}

	// Receive Common Header into the buffer
	if((result = recv(sd, buf, HEADER_LEN, MSG_WAITALL)) == -1) {
		//perror("ERR recv_msg():recv()");
		return -1;
	}
	
	if(result < HEADER_LEN) {
		//printf("Less data recieved \n");
		return -1;
	}

	if(DEBUG) printf("recv_msg(): Hdr received. type = 0x%02x\n", (unsigned char) buf[0]);

	char msg_type;
	char uoid[20];
	char ttl;
	char res;
	uint32_t data_len;

	memcpy(&msg_type, buf, sizeof(hdr->msg_type));
	memcpy(uoid, buf + 1, 20);
	memcpy(&ttl, buf + 21, sizeof(hdr->ttl));
	memcpy(&res, buf + 22, sizeof(hdr->reserved));
	memcpy(&data_len, buf + 23, sizeof(hdr->data_len));	

	// buf => Hdr structure
	hdr = new Hdr(msg_type, uoid, ttl, res, data_len);
	temp = hdr->data_len;
	hdr->data_len = ntohl(temp);
	//printf("data_len: %d\n", hdr->data_len);

//The special message Store has been read
	if(hdr->msg_type == STOR) {
		message = (char *)calloc(4, sizeof(char));
		
		if(!message) printf("calloc FAILED!\n");
		
		if((result = recv(sd, message, 4, 0)) == -1) {
			if(DEBUG_RC1) printf("ERR Receive_msg(): recv()");
			return -1;
		}
		Store* msg = new Store;
		memcpy(&temp, message, sizeof(temp));
		msg->metadata_len = ntohl(temp);
		//printf("metadata_len: %d\n", msg->metadata_len);
		
		pthread_mutex_lock(&g_mutex_temp_fileno);
			int temp_fileno = g_temp_fileno++;
		pthread_mutex_unlock(&g_mutex_temp_fileno);
		
		
        msg->file_no = temp_fileno;

		char int_buf[12];
		snprintf(int_buf, 11, "%d", temp_fileno);
		string temp_fname = ini.file_path + "temp_" + int_buf;
		string meta_filename = temp_fname + ".meta";
		string file_name = temp_fname + ".data";
		//printf("meta_filename: %s\n", meta_filename.c_str());
		//printf("data_filename: %s\n", file_name.c_str());


		FILE *fp;
		fp = fopen(meta_filename.c_str(), "w");
		int sta;
		sta = recv_file(fp, sd, msg->metadata_len);
		fclose(fp);
		if(sta != 0)
			return -2;
		
		fp = fopen(file_name.c_str(), "w");
		sta = recv_file(fp, sd, hdr->data_len - 4 - msg->metadata_len);	
		fclose(fp);
		if(sta!= 0)
			return -2;
		vmsg = (void *)msg;
	    *hdr_p = hdr;
	    *vmsg_p = vmsg;

	    return 0;
	}

	if(hdr->msg_type == GTRS) {
		if(DEBUG_RC1) printf("Starting to recieve the get response\n");
		
		message = (char *)calloc(24, sizeof(char));
		
		if(!message) printf("calloc FAILED!\n");
		
		if((result = recv(sd, message, 24, 0)) == -1) {
			if(DEBUG_RC1) printf("ERR Receive_msg(): recv()");
			return -1;
		}
		
		char getrs_uoid[20];
		memcpy(getrs_uoid, message, 20);
		memcpy(&temp, message + 20, sizeof(temp));
		int meta_size = ntohl(temp);
		//printf("metadata_len: %d\n", msg->metadata_len);
		
		pthread_mutex_lock(&g_mutex_temp_fileno);
			int temp_fileno = g_temp_fileno++;
		pthread_mutex_unlock(&g_mutex_temp_fileno);
		
		string meta_file, data_file;
		temp_file_name(temp_fileno, meta_file, data_file);
		if(DEBUG_RC1) printf("About to open %s\n", meta_file.c_str());
		if(DEBUG_RC1) printf("Size of Meta file %d\n", meta_size);
    	
    	FILE *fp;
		fp = fopen(meta_file.c_str(), "w");
		int sta;
		sta = recv_file(fp, sd, meta_size);
		fclose(fp);
		if(sta != 0)
			return -2;
		if(DEBUG_RC1) printf("About to open %s\n", data_file.c_str());
		fp = fopen(data_file.c_str(), "w");
		sta = recv_file(fp, sd, hdr->data_len - 24 - meta_size);	
		fclose(fp);
		if(sta!= 0)
			return -2;
		
		Get_res* msg = new Get_res(getrs_uoid, meta_size, temp_fileno);
		vmsg = (void *)msg;
	    *hdr_p = hdr;
	    *vmsg_p = vmsg;
		if(DEBUG_RC1) printf("What the fuck is happening\n");
	    return 0;
	}

	// Allocate a buffer for msg body
	message = (char *)calloc(hdr->data_len, sizeof(char));
	if(!message) { 
		printf("calloc FAILED!\n");
		exit(1);
	}
	//TODO: Not using any buffer ... reading all the data in one go ... 
	//will have to change it eventually :)		
	
	// Receive message body
	if((result = recv(sd, message, hdr->data_len, 0)) == -1) {
		//printf("ERR Receive_msg(): recv()");
		return -1;
	}
	
	

	switch(hdr->msg_type) {
		case 0xFC: // Join Request Message
		{
			//printf("*** JNRQ\n");
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
			//printf("*** JNRS\n");
			Join_res* msg = new Join_res(message);
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
			//printf("*** HLLO\n");
			Hello *msg = new Hello;
			memcpy(&stemp, message, sizeof(stemp));
			msg->host_port = ntohs(stemp);
			msg->host_name = (char *)calloc(hdr->data_len - 2, sizeof(char)); //add one char for NULL
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
			//printf("*** NTFY\n");
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
			Check_res *msg = new Check_res(message);
			vmsg = (void *)msg;
		}
		break;

		case 0xEC: //search request
		{
			char search_type = message[0];
			string search_str;
			for(uint32_t i = 1; i < hdr->data_len; i++) {
				search_str += message[i];
			}
			vector<string> search_tokens;
			
			Tokenize(search_str, search_tokens, " ");
			Search_req *msg = new Search_req(search_type, search_tokens);
			vmsg = (void *)msg;
		}
		break;

		case 0xEB: //search response
		{
			char temp_uoid[20];
			memcpy(temp_uoid, message, 20);
			Search_res *msg = new Search_res(temp_uoid);
			msg->search_data = (char *)calloc(hdr->data_len - 20, sizeof(char));
			memcpy(msg->search_data, message + 20, hdr->data_len - 20);
			vmsg = (void *)msg;
		}
		break;

		case 0xDC: //get request
		{
			char file_id[20];
			char sha1[20];
			memcpy(file_id, message, 20);
			memcpy(sha1, message+20, 20);
			Get_req *msg = new Get_req(file_id, sha1);
			vmsg = (void *)msg;
		
		}
		break;

		case 0xDB: //get response -- special case , handled above
		break;

		case 0xCC: //store -- special case , handled above
		break;

		case 0xBC: //delete
		{
			char *filename;
			char sha1[20];
			char nonce[20];
			char password[20];
			int filename_len = hdr->data_len - 97; //Constants bytes = 97
			filename = (char *)calloc(filename_len + 1, sizeof(char));
			memcpy(filename, message + 9, filename_len);
			filename[filename_len] = '\0';
			memcpy(sha1, message + 9 + filename_len + 2 + 5, 20);
			memcpy(nonce, message + 9 + filename_len + 2 + 5 + 22 + 6, 20);
			memcpy(password, message + 9 + filename_len + 2 + 5 + 22 + 6 + 22 + 9, 20);
			Del *msg = new Del(string(filename), sha1, nonce, password);
			vmsg = (void *)msg;
		}
		break;

		case 0xAC: //status request
		{
			//printf("*** STRQ\n");
			Status_req *msg = new Status_req;
			msg->type = message[0];
			vmsg = (void *)msg; 
		}
		break;
		
		case 0xAB: //status response
		{
			//printf("*** STRS\n");
			Status_res *msg = new Status_res(message);
			
			memcpy(&stemp, message + 20, sizeof(stemp));
			msg->hostinfo = ntohs(stemp);
			memcpy(&stemp, message + 22, sizeof(stemp));
			msg->host_port = ntohs(stemp);
			msg->host_name = (char *)calloc(msg->hostinfo - 2 + 1, sizeof(char));
			memcpy(msg->host_name, message + 24, msg->hostinfo - 2);
			msg->host_name[msg->hostinfo] = '\0';
			int record_len = hdr->data_len - 22 - msg->hostinfo;
			msg->data = (char *)calloc(record_len + 1, sizeof(char));
			memcpy(msg->data, message + 22 + msg->hostinfo, record_len);
			msg->data[record_len] = '\0';
			vmsg = (void *)msg;
		}		
		break;

		default : 
			//printf("recv_msg(): Unknown Message type 0x%02x \n", (unsigned char) hdr->msg_type);
			return -1;
	}
	*hdr_p = hdr;
	*vmsg_p = vmsg;
	
	if(DEBUG) printf("recv_msg(): msg 0x%02x received.\n", (unsigned char) hdr->msg_type);

	return 0;
} // end of recv_msg()

 
// function join_req
//	 Build a join msg
// where should we alloc the hdr structs ... before the function call ? or inside the function call ?
//	Bob: I think inside the function.

int join_req(Hdr** hdr_p, void** vmsg_p)
{	
	
	Hdr *hdr;
	void *vmsg;
	// Create host name
	char *hostname = (char *)calloc(NAME_LEN, sizeof(char));
	gethostname(hostname, NAME_LEN); // localhost???

	// Create common header
	char uoid[20];
	char obj_type[] = "msg";
	getuoid(ini.node_inst_id, obj_type, uoid, 20);

	hdr = new Hdr(0xFC, uoid, ini.ttl, 0, 6 + strlen(hostname));

	// Create msg body
	Join_req* msg = new Join_req(ini.location, ini.port, hostname);

	// Convert to void* pointer
	vmsg = (void *)msg;
	*hdr_p = hdr;
	*vmsg_p = vmsg;
	return 0;
}


//join_res
//	Accept a hdr and a vmsg pointer (this will have the return value) 
//	... also a Hdr (for Join req) & Join_req object pointer
//	Join_req should be an object ...

int join_res(Hdr** hdr_p, void** vmsg_p, Hdr *hdr1, void *vmsg1)
{	
	uint32_t distance;
	int32_t temp;
	Join_req* msg1 = (Join_req*) vmsg1;

	// Create common header
	char uoid[20];
	char obj_type[] = "msg";
	getuoid(ini.node_inst_id, obj_type, uoid, 20);
	char *hostname;
	Hdr* hdr2 = new Hdr(0xFB, uoid, ini.ttl, 0, 26 + strlen(ini.hostname));
	hostname = (char *)calloc(strlen(ini.hostname) + 1, sizeof(char));
	memcpy(hostname, ini.hostname, strlen(ini.hostname) + 1);
	
	// Create msg body
	temp = ini.location - msg1->host_loc;
	distance = abs(temp);	
	Join_res* msg2 = new Join_res(hdr1->uoid, distance, ini.port, hostname);
	
	*hdr_p = hdr2;
	*vmsg_p = (void*) msg2;

	return 0;
}


//hello
//	creates a hello packet

int hello(Hdr** hdr, void** vmsg)
{	
	// Create host name
	char *hostname = (char *)calloc(NAME_LEN, sizeof(char));
	gethostname(hostname, NAME_LEN);

	// Create common header
	char uoid[20];
	char obj_type[] = "msg";
	getuoid(ini.node_inst_id, obj_type, uoid, 20);
	
	// Bob: Hello should have TTL = 1
	*hdr = new Hdr(HLLO, uoid, 1, 0, 2 + strlen(hostname));

	// Create msg body
	Hello* msg = new Hello(ini.port, hostname);

	// Convert to void* pointer
	void *t_vmsg = (void *)msg;
	*vmsg = t_vmsg;

	return 0;
}

//notify
//Please pass error code along with the return pointers for hdr and msg

int notify(Hdr** hdr_p, void** vmsg_p, char error)
{

	// Create common header
	Hdr *hdr;
	void *vmsg;    
	char uoid[20];
	char obj_type[] = "msg";
	getuoid(ini.node_inst_id, obj_type, uoid, 20);
	
	hdr = new Hdr(0xF7, uoid, 1, 0, 1);

	// Create msg body
	Notify* msg = new Notify(error);

	// Convert to void* pointer
	vmsg = (void *)msg;
	*hdr_p = hdr;
	*vmsg_p = vmsg;
	return 0;
}

int check_req(Hdr** hdr_p, void** vmsg_p)
{
	// Create common header
	Hdr *hdr;
	void *vmsg;    
	char uoid[20];
	char obj_type[] = "msg";
	getuoid(ini.node_inst_id, obj_type, uoid, 20);
	
	hdr = new Hdr(0xF6, uoid, ini.ttl, 0, 0);

	// Create msg body
	
	// Convert to void* pointer
	vmsg = NULL;
	*hdr_p = hdr;
	*vmsg_p = vmsg;
	return 0;
}

// check_res
//	 need to pass the header for the check_req for uoid in check response

int check_res(Hdr** hdr_p, void** vmsg_p, Hdr *chdr)
{
	Hdr *hdr;
	void *vmsg;
	// Create common header
	char uoid[20];
	char obj_type[] = "msg";
	getuoid(ini.node_inst_id, obj_type, uoid, 20);
	
	hdr = new Hdr(0xF5, uoid, ini.ttl, 0, 20);

	// Create msg body
	Check_res* msg = new Check_res(chdr->uoid);

	// Convert to void* pointer
	vmsg = (void *)msg;
	*hdr_p = hdr;
	*vmsg_p = vmsg;
	return 0;
}

int status_req(Hdr** hdr_p, void** vmsg_p, char status, char ttl)
{
	Hdr *hdr;
	void *vmsg;
	// Create common header
	char uoid[20];
	char obj_type[] = "msg";
	getuoid(ini.node_inst_id, obj_type, uoid, 20);
	
	hdr = new Hdr(0xAC, uoid, ttl, 0, 1);

	// Create msg body
	Status_req* msg = new Status_req(status);

	// Convert to void* pointer
	vmsg = (void *)msg;
	*hdr_p = hdr;
	*vmsg_p = vmsg; 
	return 0;
}


int dup_msg(Hdr** hdr_p, void** vmsg_p, Hdr* hdr1, void* vmsg1)
{
	Hdr* hdr2 = NULL;
	void* vmsg2 = NULL;

	if(DEBUG_DUPMSG) printf("* 0.  \n");	
	// Duplicate common header
	//hdr2 = new Hdr(hdr1->msg_type, hdr1->uoid, hdr1->ttl - 1, 0, hdr1->data_len);
	hdr2 = new Hdr(hdr1->msg_type, hdr1->uoid, hdr1->ttl, 0, hdr1->data_len); //!
	

	// Duplicate msg body
	switch(hdr1->msg_type) 
	{
		case KPAV: // Keep_alive
			{
				; // No msg body
			}
			break;
		case JNRQ:
			{
				if(DEBUG_DUPMSG) printf("* 1.  \n");	
				Join_req* msg1 = (Join_req*) vmsg1;
				char* host_name2 = (char*) calloc(hdr1->data_len - 6 + 1, sizeof(char));
				strcpy(host_name2, msg1->host_name);

				Join_req* msg2 = new Join_req( msg1->host_loc, msg1->host_port, host_name2);
				vmsg2 = (void*)msg2;
			}
			break;
		case JNRS:
			{
				if(DEBUG_DUPMSG) printf("* 2.  \n");	
				Join_res* msg1 = (Join_res*) vmsg1;
				char* host_name2 = (char*) calloc(hdr1->data_len - 26 + 1, sizeof(char));
				strcpy(host_name2, msg1->host_name);
				
				Join_res* msg2 = new Join_res(msg1->uoid, msg1->distance, msg1->host_port, host_name2);		
				vmsg2 = (void*) msg2;	
			}
			break;
		case CKRQ:
			{
				; // No msg body		
			}
			break;
		case CKRS:
			{
				Check_res* msg1 = (Check_res*) vmsg1;
				Check_res* msg2 = new Check_res(msg1->uoid);
				vmsg2 = (void*)msg2;
			}
			break;
		case STRQ: // Status_req
			{
				if(DEBUG_DUPMSG) printf("* 3.  \n");	
				Status_req* msg1 = (Status_req*) vmsg1;
				Status_req* msg2 = new Status_req( msg1->type );
				vmsg2 = (void*)msg2;
			}
			break;
		case STRS: // Status_res
			{	
				if(DEBUG_DUPMSG) printf("* 4.  \n");	
				Status_res* msg1 = (Status_res*) vmsg1;
				Status_res* msg2 = new Status_res( msg1->uoid, msg1->hostinfo, msg1->host_port);
			
				int host_info_len = msg1->hostinfo - 2 + 1;					// +1 for NULL
				int record_len = hdr1->data_len - 22 - msg1->hostinfo + 1;	// +1 for NULL
				//printf("host_info_len=%d\n", host_info_len);
				msg2->host_name = (char*) malloc(host_info_len);
				msg2->data = (char*) malloc(record_len);
			
				memcpy(msg2->host_name, msg1->host_name, host_info_len); 
				memcpy(msg2->data, msg1->data, record_len);
				vmsg2 = (void*)msg2;
			}
			break;
		case STOR:
			{
				if(DEBUG_DUPMSG) printf("* STOR \n");
				Store* msg1 = (Store*) vmsg1;
				Store* msg2 = new Store( msg1->metadata_len, msg1->file_no);
				vmsg2 = (void*)msg2;
			}
			break;
		case SHRQ:
			{
				if(DEBUG_DUPMSG) printf("* SHRQ \n");
				Search_req* msg1 = (Search_req*) vmsg1;
				Search_req* msg2 = new Search_req(msg1->search_type, msg1->query);
				vmsg2 = (void*)msg2;
			}
			break;
		case SHRS:
			{
				if(DEBUG_DUPMSG) printf("* SHRS \n");
				
				Search_res* msg1 = (Search_res*) vmsg1;
				uint32_t data_len = hdr1->data_len - 20;
				char* data = (char*) calloc(data_len, sizeof(char));
				memcpy(data, msg1->search_data, data_len);
				
				Search_res* msg2 = new Search_res(msg1->uoid, data);
				vmsg2 = (void*)msg2;
			}
			break;
		case GTRQ:
			{
				if(DEBUG_DUPMSG) printf("* GTRQ \n");

				Get_req* msg1 = (Get_req*) vmsg1;
				Get_req* msg2 = new Get_req(msg1->file_id, msg1->sha1);
				
				vmsg2 = (void*)msg2;			
			}
			break;
		case GTRS:
			{
				if(DEBUG_DUPMSG) printf("* GTRS \n");
				
				Get_res* msg1 = (Get_res*) vmsg1;
				Get_res* msg2 = new Get_res(msg1->uoid, msg1->metadata_len, msg1->file_no);

				vmsg2 = (void*)msg2;			
			}
			break;
		case DELT:
			{
				if(DEBUG_DUPMSG) printf("* DELT \n");
				Del* msg1 = (Del*) vmsg1;
				Del* msg2 = new Del(msg1->filename, msg1->sha1, msg1->nonce, msg1->password);

				vmsg2 = (void*)msg2;				
			}
			break;
		default:
			break;
	}
	
	if(DEBUG_DUPMSG) printf("* 4.  \n");	

	*hdr_p = hdr2;
	*vmsg_p = vmsg2; 
	
	return 0;
} // end of dup_msg()


//Please call this function under the lock of g_peerid2sd
//Will create a status req packet
int create_status_res(Hdr** hdr_p, void** vmsg_p, Hdr *shdr)
{	
	//printf("Entering create_status_res() \n");
	Hdr *hdr;
	void *vmsg;
	string hostid = "", hostname = "", hostport;
	size_t pos;
	uint16_t port;
	string s_len;
	vector< string > v_hostname;
	vector< uint16_t > v_hostport;
	vector< int > v_length;
	int sum_len = 0;
	
	//printf("Initially c_port is %s", c_port);
	map<string, int>::iterator itr;
	for(itr = g_hostid2sd.begin(); itr != g_hostid2sd.end(); ++itr) {
		//cout<<"I have a connection establised\n";
		hostid = itr->first;
		pos = hostid.find("_");
		hostname = hostid.substr(0, pos);
		hostport = hostid.substr(pos + 1);
		istringstream buffer(hostport);
		//converting a string to int
		uint16_t temp;
		int some_int;
		buffer >> some_int;
		//coneverting into byte form
		temp = some_int;
		port = htons(temp);
		v_hostname.push_back(hostname);
		v_hostport.push_back(port);
		some_int = hostname.length() + 2;
		//temp = htonl(some_int);
		v_length.push_back(some_int);
		sum_len += hostname.length() + 2 + 4;
	}
	sum_len = sum_len + 4;
	//printf("Vectors initialized\n");
	//printf("\nSum_len = %d\n", sum_len);
	char *reply = (char *)calloc(sum_len, sizeof(char));
	int j=0;
	for(unsigned int i = 0; i<v_hostport.size(); i++) {
		int l_temp = htonl(v_length[i]);
		
		memcpy(reply + j, &l_temp, 4);
		uint16_t s_temp = v_hostport[i]; // Final pt1. line change 1
		memcpy(reply + j + 4, &s_temp, 2);
		//printf("Hope !!\n");	
		const char *c_hostname = v_hostname[i].c_str();
		memcpy(reply + j + 6, c_hostname, v_length[i] - 2);
		j = j + v_length[i] + 4;
	}
	
	status_res(&hdr, &vmsg, shdr, sum_len, reply);
	*hdr_p = hdr;
	*vmsg_p = vmsg;
	//printf("Leaving the create_status_reply\n");
	
	return 1;
}

//Please pass status_req header, record_len and the data that has been calculated by some other fn
//
int status_res(Hdr** hdr_p, void** vmsg_p, Hdr* shdr, uint32_t record_len, char *data)
{
	Hdr *hdr;
	void *vmsg;

	// Create common header
	char uoid[20];
	char obj_type[] = "msg";
	getuoid(ini.node_inst_id, obj_type, uoid, 20);
 
	hdr = new Hdr(0xAB, uoid, ini.ttl, 0, 24 + strlen(ini.hostname) + record_len);

	// Create msg body
	// Bob. 11/15/08. critical bug fix.
	char* hostname = (char *)calloc(strlen(ini.hostname) + 1, sizeof(char));
	memcpy(hostname, ini.hostname, strlen(ini.hostname) + 1);
	Status_res* msg = new Status_res(shdr->uoid, 2 + strlen(ini.hostname), ini.port, ini.hostname, record_len, data);

	// Convert to void* pointer
	vmsg = (void *)msg;
	*hdr_p = hdr;
	*vmsg_p = vmsg;
	return 0;
}


// post_hello
//	 Push a HLLO msg into a L-2 queue.
void post_hello(int sd)
{
	Connection *conn = NULL;

	pthread_mutex_lock(&g_mutex_sd2conn);
	{
		std::map<int, Connection* >::iterator sd2conn_iter;
		sd2conn_iter = g_sd2conn.find(sd);
		if(sd2conn_iter == g_sd2conn.end()) { 
			pthread_mutex_unlock(&g_mutex_sd2conn);
			return;	
		}			
		conn = g_sd2conn[sd];
	}
	pthread_mutex_unlock(&g_mutex_sd2conn);


	Hdr *hdr = NULL;
	void *vmsg = NULL;

	//*** Create an HELLO msg.
	hello(&hdr, &vmsg);
	//*** Here hdr, vmsg reset to NULL!

	// Create an event and push it to L-2 queue.
	Event ev2(EV_MSG, sd, hdr, vmsg, 's');
	pthread_mutex_lock(&(conn->mutex));
	{
		if(DEBUG) printf("post-hello(): posting HELLO msg. \n");
		conn->que.push(ev2);
		//printf("Signalling write thread\n");
		//pthread_cond_broadcast(&(conn->cv));
		pthread_cond_signal(&(conn->cv));
	}
	pthread_mutex_unlock(&(conn->mutex));
	
	// Associate host_name with sock and port.
	Port_sd port_sd(ini.port, sd);
	string host(ini.hostname);
	pthread_mutex_lock(&(g_mutex_hostname2port_sd));	
	{
		g_hostname2port_sd[host] = port_sd;	
	}
	pthread_mutex_unlock(&(g_mutex_hostname2port_sd));	

	// Bob: MUST NOT delete here. write_th will delete them after sending msg.
	//delete hdr;
	//delete (Hello *)vmsg;
	return;
} // end of post-hello()


void send_notify(char error, int sd)
{
	Hdr *hdr;
	void *vmsg;

	// Create & send notify msg.
	notify(&hdr, &vmsg, error);
	int send_err = send_msg(sd, hdr, vmsg);

	// Log sent msg
	if(send_err == 0) {
		pthread_mutex_lock(&g_mutex_log_msg);
			log_msg(g_log_file, hdr, vmsg, 's', sd);
		pthread_mutex_unlock(&g_mutex_log_msg);
	}
	
	delete hdr;
	delete (Notify*) vmsg;

	// Delete connection info.
	pthread_mutex_lock(&g_mutex_sd2conn);
	{
		std::map<int, Connection* >::iterator sd2conn_iter;
		sd2conn_iter = g_sd2conn.find(sd);
		if(sd2conn_iter != g_sd2conn.end()) {
//#			It delete the entry, kill_this_node seems to try to use it, causing seg fault. 
//#			g_sd2conn.erase(sd2conn_iter);	// Causes Seg fault.
											
		}				
	}
	pthread_mutex_unlock(&g_mutex_sd2conn);


}

int store(Hdr** hdr_p, void** vmsg_p, int file_no, unsigned char ttl, uint32_t metasize, uint32_t filesize )
{

	// Create common header
	Hdr *hdr;
	void *vmsg;
	char uoid[20];
	char obj_type[] = "msg";
	getuoid(ini.node_inst_id, obj_type, uoid, 20);

	hdr = new Hdr(STOR, uoid, ttl, 0, metasize + filesize + 4);
	
	// Create msg body
	if(DEBUG_RC1) cout<<"FIle no. sotred in the event"<<file_no<<endl;
	Store* msg = new Store(metasize, file_no);

	// Convert to void* pointer
	vmsg = (void *)msg;
	*hdr_p = hdr;
	*vmsg_p = vmsg;
	return 0;
}

int search_req(Hdr** hdr_p, void** vmsg_p, char type, vector<string> &data)
{

	// Create common header
	Hdr *hdr;
	void *vmsg;
	char uoid[20];
	char obj_type[] = "msg";
	getuoid(ini.node_inst_id, obj_type, uoid, 20);
	uint32_t len = 0;

	for(uint32_t i = 0; i<data.size(); i++) {
		len += data[i].length();
		len++; //This is for the space that seprates eachkeyword
	}
	len--; //This is for the removal of the extra space at the end

	hdr = new Hdr(SHRQ, uoid, ini.ttl, 0, len + 1); // The extra one is for char field

	// Create msg body
	Search_req* msg = new Search_req(type, data);

	// Convert to void* pointer
	vmsg = (void *)msg;
	*hdr_p = hdr;
	*vmsg_p = vmsg;
	return 0;
}

int search_res(Hdr** hdr_p, void** vmsg_p, Hdr *shdr, void *smsg, set<int> file_nos)
{

	Hdr *hdr;
	void *vmsg;
	int record_len = 0;
	
	// Create common header
	char uoid[20];
	char obj_type[] = "msg";
	char file_name[20] = "";
	char num_c[4];
	char *message;
	string s_message = "";
		
	
	set<int>::iterator it;
	for(it = file_nos.begin(); it != file_nos.end(); it++) {
		//sprintf(file_name, "%d.meta", *it);	
		sprintf(file_name, "%sfiles/%d.meta", ini.homedir, *it);
		
		string file(file_name);
		if(DEBUG_RC1) cout << file << endl;
		int temp = get_filesize(file);
		if(temp == -1)
			continue;

		uint32_t temp1 = htonl(temp);
		memcpy(num_c, &temp1, 4);
		//string temp_s = "";
		//cstr2string(num_c, temp_s, 4);
		cstr2string(num_c, s_message, 4);
		if(DEBUG_RC1) cout << "search_res(): " << s_message << endl;
		//s_message += temp_s;

		
		//getuoid(ini.node_inst_id, obj_type, uoid, 20);
		string fileid = g_fileno2fileid[*it];
		if(DEBUG_RC1) cout<<"The FileID is "<<fileid<<endl;
		//this is the file id that need to be stored in the map
		//cstr2string(uoid, s_message, 20);
		s_message += fileid;		
		
		FILE *fp;
		fp = fopen(file.c_str(), "r");
		if(!fp) {
			continue;
		}
		if(DEBUG_RC1) printf("Found a match\n");
		message = (char *)calloc(temp, sizeof(char));
		fread(message, 1, temp, fp);
		cstr2string(message, s_message, temp);
		if(DEBUG_RC1) cout << "search_res(): " << s_message << endl;
		free(message);
		fclose(fp);
	}
	uint32_t temp1 = 0;
	memcpy(num_c, &temp1, 4);
	cstr2string(num_c, s_message, 4);
	if(DEBUG_RC1) cout << "search_res(): " << s_message << endl;

	record_len = s_message.length() + 20;
	int s_message_len = s_message.length()+1;
	message = (char *)calloc(s_message_len, sizeof(char));
	
	//Bug: strcpy stops when 0x00 met.  
	//strcpy(message, s_message.c_str());		
	memcpy(message, s_message.c_str(), s_message_len); 

	message[s_message.length()] = '\0';
	getuoid(ini.node_inst_id, obj_type, uoid, 20);
 
 
 
	hdr = new Hdr(SHRS, uoid, ini.ttl, 0, record_len);
	Search_res* msg = new Search_res(shdr->uoid, message);

	// Convert to void* pointer
	vmsg = (void *)msg;
	*hdr_p = hdr;
	*vmsg_p = vmsg;
	return 0;
}

int get_req(Hdr** hdr_p, void** vmsg_p, char *file_id, char *sha_hash)
{
	// Create common header
	Hdr *hdr;
	void *vmsg;
	char uoid[20];
	char obj_type[] = "msg";
	getuoid(ini.node_inst_id, obj_type, uoid, 20);

	hdr = new Hdr(GTRQ, uoid, ini.ttl, 0, 40);
	
	// Create msg body
	Get_req* msg = new Get_req(file_id, sha_hash);

	// Convert to void* pointer
	vmsg = (void *)msg;
	*hdr_p = hdr;
	*vmsg_p = vmsg;
	return 0;	

}

int get_res(Hdr** hdr_p, void** vmsg_p, Hdr *shdr,  void *smsg, int fileno) 
{
	Hdr *hdr;
	void *vmsg;

	// Create common header
	char uoid[20];
	char obj_type[] = "msg";
	getuoid(ini.node_inst_id, obj_type, uoid, 20);
	string meta_file, data_file;
	int meta_size, data_size;
 	temp_file_name(fileno, meta_file, data_file);
 	
 	meta_size = get_filesize(meta_file);
 	if(meta_size == -1)
 		return -1; //File not found
 	
 	data_size = get_filesize(data_file);
 	if(data_size == -1)
 		return -1; //File not found
 		
	hdr = new Hdr(GTRS, uoid, ini.ttl, 0, 20 + 4 + meta_size + data_size);

	// Create msg body
	Get_res* msg = new Get_res(shdr->uoid, meta_size, fileno);

	// Convert to void* pointer
	vmsg = (void *)msg;
	*hdr_p = hdr;
	*vmsg_p = vmsg;
	return 0;
}

void del(Hdr** hdr_p, void** vmsg_p, string filename, char *sha1, char *nonce, char *pass) 
{
	Hdr *hdr;
	void *vmsg;

	// Create common header
	char uoid[20];
	char obj_type[] = "msg";
	getuoid(ini.node_inst_id, obj_type, uoid, 20);
	
	hdr = new Hdr(DELT, uoid, ini.ttl, 0, filename.length() + 97);

	// Create msg body
	Del* msg = new Del(filename, sha1, nonce, pass);

	// Convert to void* pointer
	vmsg = (void *)msg;
	*hdr_p = hdr;
	*vmsg_p = vmsg;
	return ;
}

int status_res_files(Hdr** hdr_p, void** vmsg_p, Hdr* shdr, vector<int> file_no)
{
	Hdr *hdr;
	void *vmsg;
	
	int total_file_size = 4;
	
	for(uint32_t i=0; i< file_no.size(); i++) {
		string meta_file, data_file;
		file_name(file_no[i], meta_file, data_file);
		int flsize = get_filesize(meta_file);
		if(flsize == -1)
			continue;
		total_file_size += flsize;
		total_file_size +=4;
	}
	
	char *data;
	data = (char *)calloc(total_file_size, sizeof(char));
	

	
	// Create common header
	char uoid[20];
	char obj_type[] = "msg";
	getuoid(ini.node_inst_id, obj_type, uoid, 20);
 
	hdr = new Hdr(0xAB, uoid, ini.ttl, 0, 24 + strlen(ini.hostname) + total_file_size);
	int j = 0;
	FILE *fp;
	for(uint32_t i=0; i< file_no.size(); i++) {
		string meta_file, data_file;
		file_name(file_no[i], meta_file, data_file);
		int flsize = get_filesize(meta_file);
		if(flsize == -1)
			continue;
		uint32_t temp = htonl(flsize);
		memcpy(data + j, &temp, sizeof(temp));
		j+=4;
		fp = fopen(meta_file.c_str(), "r");
		fread(data + j, 1, flsize, fp);
		j += flsize;
		
	}
	
	uint32_t temp = 0;
	memcpy(data + j, &temp, sizeof(temp));

	char* hostname = (char *)calloc(strlen(ini.hostname) + 1, sizeof(char));
	memcpy(hostname, ini.hostname, strlen(ini.hostname) + 1);
	Status_res* msg = new Status_res(shdr->uoid, 2 + strlen(ini.hostname), ini.port, ini.hostname,total_file_size, data);

	// Convert to void* pointer
	vmsg = (void *)msg;
	*hdr_p = hdr;
	*vmsg_p = vmsg;
	return 0;
}
