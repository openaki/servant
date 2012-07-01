#include "includes.h"
#include "startup.h"
#include "init.h"
#include "error.h"
#include "constants.h"
extern Startup ini;

int readline(FILE *fp, char **ln, int *len)
{
	vector<char> line;
	char *buf;
	char ch;
	int i = 0;
	while((ch = fgetc(fp))!=EOF) {
		if(ch == '\n')
			break;
		//cout<<ch<<"  "<<i++<<" ";
		fflush(0);
		line.push_back(ch);
	}
	if(ch == EOF)
		return EOF;
	*len = line.size();
	buf = (char *)calloc(*len, sizeof(char) + 1);
	for(i = 0; i< *len; i++)
		buf[i] = line[i];
	buf[*len] = '\0';
	*ln = buf;
    if(ch == EOF)
	        return EOF;
	return 1;

}


char * rem_space(char *word)
{
	char *ans;
	int flag;
	char quote;
	unsigned int i,x;
	if(word == NULL)
		return NULL;
	if(strlen(word) == 0)
		return NULL;
	ans = (char *)calloc(strlen(word) + 1, sizeof(char));
	if(ans == NULL) {
		printf("Malloc failed");
		exit(1);
	}
	x = 0;
	i=0;
	while(word[i] == ' ' || word[i] == '\t')
		i++;
	//i--;
	flag = 0;
	quote = '\'';
	if(word[i] == '\"') {
		quote = '\"';
		flag = 1;
		i++;
	}
	if(word[i] == '\'') {
		quote = '\'';
		flag = 1;
		i++;
	}

	for (; i< strlen(word); i++) {
		if(word[i] == ' ' && flag == 0)
			break;

		if(word[i] == quote && flag == 1) {
			break;
		}
		ans[x++] = tolower(word[i]);
	}
	ans[x] = '\0';
	return ans;
}

int init(char *filename)
{
	FILE *fp;
	
	int read, length;
	char *line;
	char *temp;
	char *word1, *word2;
	char *att, *att_value;
	int count1 = 0;
	int count2 = 0;
	int len;
	int count;
	line = NULL;
	len = 0;
	fp = fopen(filename, "r");
	if(!fp) {
	  print_error_and_exit("File not exist.");
	}

	count = 0;
	while ((read = readline(fp, &line, &len)) != -1) {
		length = strlen(line);
		if(length == 0)
			continue;
		//line[length - 1] = '\0';
		//printf("%s\n", line);
		if(line[0] == ';')
			continue;
		word1 = line;
		temp = line;
		word2 = NULL;
		while(*temp != '\0') {
			if(*temp == '=') {
				*temp = '\0';
				word2 = temp + 1;
				break;
			}
			temp++;
		}
					
		if((strcmp(word1, "[init]")) == 0) {
			count1++;
			if(count2 == 1)
				count2++;
			continue;
		}
		
		if((strcmp(word1, "[beacons]")) == 0 ) {
			count2++;
			if(count1 == 1)
				count1++;
			continue;
		}
	
		att = rem_space(word1);
		att_value = rem_space(word2);
		
		///printf("%sTTT%s\n", att, att_value);
		if(strlen(att) == 0)
			continue;

		if(count1 == 1) {
		
			if(( strcmp(att, "port")) == 0) {
				ini.port = atol(att_value);
				count++;
	
			} else if(( strcmp(att, "location")) == 0) {
				ini.location = atoll(att_value);
				count++;
			
			} else if(( strcmp(att, "homedir")) == 0) {
				ini.homedir = (char *)calloc(strlen(att_value), sizeof(char));
				strcpy(ini.homedir, att_value);
				count++;
	
			} else if(( strcmp(att, "logfilename")) == 0) {
				strcpy(ini.logfile, att_value);
					
			} else if(( strcmp(att, "autoshutdown")) == 0) {
				ini.autoshutdown = atoi(att_value);
	
			} else if(( strcmp(att, "ttl")) == 0) {
				ini.ttl = atoi(att_value);
	
			} else if(( strcmp(att, "msglifetime")) == 0) {
				ini.msglifetime = atoi(att_value);
			
			} else if(( strcmp(att, "getmsglifetime")) == 0) {
				ini.getmsglifetime = atoi(att_value);
	
			} else if(( strcmp(att, "initneighbors")) == 0) {
				ini.initneighbor = atoi(att_value);
			
			} else if(( strcmp(att, "jointimeout")) == 0) {
				ini.jointo = atoi(att_value);
	
			} else if(( strcmp(att, "keepalivetimeout")) == 0) {
				ini.keepaliveto = atoi(att_value);
			
			} else if(( strcmp(att, "minneighbors")) == 0) {
				ini.minneighbor = atoi(att_value);
			
			} else if(( strcmp(att, "nocheck")) == 0) {
				ini.nocheck = atoi(att_value);
	
			} else if(( strcmp(att, "cacheprob")) == 0) {
				ini.cacheprob = strtod(att_value, NULL);
			
			} else if(( strcmp(att, "storeprob")) == 0) {
				ini.storeprob = strtod(att_value, NULL);
			
			} else if(( strcmp(att, "neighborstoreprob")) == 0) {
				ini.nstoreprob = strtod(att_value, NULL);
			
			} else if(( strcmp(att, "cachesize")) == 0) {
				ini.cachesize = atol(att_value);
	
			} else if(( strcmp(att, "permsize")) == 0) {
				ini.permsize = atoi(att_value);
	
			} else {
				//printf("bad attribute in the ini file  =%s... ignoring it\n", att);
			}
		}
		if(count2 == 1) {
			if(( strcmp(att,"retry")) == 0) {
				ini.retry = atoi(att_value);
				continue;
			}
			
			temp = att;

			while(*temp != '\0') {
				if(*temp == ':') {
					*temp = '\0';
					att_value = temp + 1;
				}
				temp++;
			}
			
			string str = att;
			ini.beacon_name.push_back(str);	
			ini.beacon_port.push_back(atoi(att_value));
		
		}
		free(line);
	//	free(att);
	}
	unsigned int i;
	char *ip, *myip, hostname[NAME_LEN];
	
	int len_hostname = strlen(ini.homedir);
	if(ini.homedir[len_hostname - 1] != '/') {
		char *new_home_dir;
		new_home_dir = (char *)calloc(len_hostname + 2, sizeof(char));
		fflush(stdout);
		if(new_home_dir == NULL) {
			printf("Memory allocation Failed !!!");
			exit(1);
		}
		strcpy(new_home_dir, ini.homedir);
		if(DEBUG_RC1) printf("old: %s\n", new_home_dir);
		new_home_dir[len_hostname] = '/';
		new_home_dir[len_hostname + 1] = '\0';
		if(DEBUG_RC1) printf("new: %s\n", new_home_dir);
		free(ini.hostname);
		ini.homedir = new_home_dir;
	}
	const char *buffer;
	struct hostent *h;
	gethostname(hostname, NAME_LEN);
	//cout<<hostname;
	ini.hostname = (char *)calloc(sizeof(hostname) + 1, sizeof(char));
	ini.file_path = ini.homedir;
	ini.file_path +="files/";
	strcpy(ini.hostname, hostname);

	ini.node_id = (char *)calloc(sizeof(hostname) + 6, sizeof(char));
	snprintf(ini.node_id, sizeof(hostname) + 6, "%s_%d", ini.hostname, ini.port);
	ini.node_inst_id = (char *)calloc(sizeof(hostname) + 26, sizeof(char));
	snprintf(ini.node_inst_id, sizeof(hostname) + 26, "%s_%ld", ini.node_id, time(NULL));
	
	if((h = gethostbyname(hostname)) == NULL) {
		//perror("Address Request in init()");
		return 1;
	}
	myip = inet_ntoa(*((struct in_addr *)h->h_addr));
	//printf("%s\n", myip);	
	for(i=0; i< ini.beacon_name.size(); i++) {
		buffer = ini.beacon_name[i].c_str();
		if((h = gethostbyname(buffer)) == NULL) {
			//perror("Address Request in init()");
			return 1;
		}
		ip = inet_ntoa(*((struct in_addr *)h->h_addr));
		//printf("%s\n", ip);
		if((strcmp(myip, ip) == 0) && (ini.port == ini.beacon_port[i]))
				return 1; //True for a beacon node
	}


	return 0;
}


