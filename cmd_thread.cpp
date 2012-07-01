#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <vector>
#include "servant.h"


//Please call this function under the lock of g_peerid2sd
//Will create a status req packet
int create_status_res(Hdr** hdr_p, void** vmsg_p, Hdr *shdr)
{	
	printf("Entering create_status_res() \n");
	Hdr *hdr;
	void *vmsg;
	string hostid = "", hostname = "", hostport;
	size_t pos;
	uint16_t port;
	uint32_t len;
	char c_port[3] = "AA";
	char c_len[5] = "AAAA";
	string s_len;
	vector< string > v_hostname;
	vector< uint16_t > v_hostport;
	vector< int > v_length;
	int sum_len = 0;
	
	printf("Initially c_port is %s", c_port);
	map<string, int>::iterator itr;
	for(itr = g_hostid2sd.begin(); itr != g_hostid2sd.end(); ++itr) {
		cout<<"I have a connection establised\n";
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
	printf("Vectors initialized\n");
	printf("\nSum_len = %d\n", sum_len);
	char *reply = (char *)calloc(sum_len, sizeof(char));
	int j=0;
	for(int i = 0; i<v_hostport.size(); i++) {
		int l_temp = htonl(v_length[i]);
		
		memcpy(reply + j, &l_temp, 4);
		int s_temp = v_hostport[i];
		memcpy(reply + j + 4, &s_temp, 2);
		printf("Hope !!\n");	
		const char *c_hostname = v_hostname[i].c_str();
		memcpy(reply + j + 6, c_hostname, v_length[i] - 2);
		j = j + v_length[i] + 4;
	}
	
	
	for(unsigned int i =0; i< sum_len; i++)
		printf("%02x ", (unsigned char)reply[i]);
	printf("\n");
	
	status_res(&hdr, &vmsg, shdr, sum_len, reply);
	*hdr_p = hdr;
	*vmsg_p = vmsg;
	printf("Leaving the create_status_reply\n");
	
	return 1;
}

using namespace std;


void * cmd_write(void *)
{
	// queue<Event> g_cmd_que;
	// pthread_mutex_t g_mutex_cmd_que;
	// pthread_cond_t g_cond_cmd_que;
	Event e1;
	map<int><int> nodes_done;
	map<int, int>::iterator itr;
	Status_res *msg;
	while(1) {
		pthread_mutex_lock(&(g_mutex_cmd_que));
			while(g_cmd_que.size() == 0) {
				if(short_circuit == 1) {
					pthread_mutex_unlock(&(g_mutex_cmd_que));
					return NULL;
	
				}
	
				pthread_cond_wait(&g_cond_cmd_que, &g_mutex_cmd_que);
			}
			e1 = g_cmd_que.front();
			g_cmd_que.pop();
		pthread_mutex_unlock(&(g_mutex_cmd_que));
		
		msg = (Status_res *)e1->vmsg;
		uint32_t temp,record_len, i = 0;
		uint16_t stemp, port;
		memcpy(&temp, msg->data, 4);
		record_len = ntohl(temp);
		i=0;
		while(record_len != 0) {
			memcpy(&stemp, msg->data + 4 + i, 2);
			port = ntohs(stemp);
			itr = nodes_done.find(port);
			if(itr == nodes_done.end()) {
				nodes_done[port] = 1;
				//Add the node to the nam file
			}

			//Add the link to the name file
			i = i + record_len;
		}
	}
	return NULL;
}


		
				
}

void Tokenize(const string& str, vector<string>& tokens, const string& delimiters = " ")
{
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    string::size_type pos     = str.find_first_of(delimiters, lastPos);

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

void* cmd_thread(void *)
{
    fd_set readfds;
    struct timeval tv;
	printf("\n");
	fflush(stdout);
	char *line;
	int line_len;
	int flag = 1;
	char ch;
	Cmd* cmd = new cmd;

	while (1) {
		if(flag == 1) {
			printf("servant:>" );//ini.port);
			fflush(stdout);
			flag = 0;
		}

		FD_ZERO(&readfds);
		FD_SET(STDIN_FILENO, &readfds);
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		int select_retval = select(STDIN_FILENO+1, &readfds, NULL, NULL, &tv);
		if (select_retval < 0) {
			perror("select");
		} else if (select_retval > 0) {
			if (FD_ISSET(STDIN_FILENO, &readfds)) {
				string s = "";

				getline(cin,s);
				vector<string> words;
				Tokenize(s, words);
				cout<<words[0];	
				flag = 1;
				if(words[0] == "status") {
					if(words[1] == "neighbors") {
						cout<<"I was here :)";
						fflush(stdout);
						//cmd->type = CMD_STATUS_N;
						const char *w = words[2].c_str();
						cout<<"Is it printed"<<atoi(w);	
						//cmd->ttl = atoi(w);
						//cmd->ext_file = words[3];
						//Please add cmd to the event queue ....

						Event ev(EV_CMD, cmd);
				
					}
				} else if(words[0] == "shutdown") {
						kill_this_node(1);
						exit(1);
				}
				
			}
		} else {
			//printf("Time out :)");
			fflush(stdout);
		}
			
	}

    return NULL;
}
