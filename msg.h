#ifndef MSG_H__
#define MSG_H__

#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <set>
#include "constants.h"

#define EV_MSG 0x10
#define JNRQ 0xFC
#define JNRS 0xFB
#define HLLO 0xFA
#define KPAV 0xF8
#define NTFY 0XF7
#define CKRQ 0XF6
#define CKRS 0XF5
#define SHRQ 0XEC
#define SHRS 0XEB
#define GTRQ 0XDC
#define GTRS 0XDB
#define STOR 0XCC
#define DELT 0XBC
#define STRQ 0XAC
#define STRS 0XAB

#define EV_CMD 0x20

#define NEIGHBOR_INFO 0x01
#define FILE_INFO 0x02

#define EV_TMR 0x30


using namespace std;

struct Cmd{
	char type;
	int ttl;
	string ext_file;	
};


struct Hdr {
	unsigned char msg_type;
	char uoid[20];
	unsigned char ttl;
	char reserved;
	uint32_t data_len;
	
	Hdr(char _type, char* _uoid, unsigned char _ttl = 0, char _reserved = 0, uint32_t _len = 0);
	//Hdr(char _type, char* _uoid, char _ttl = 0, char _reserved = 0, uint32_t _len = 0);
};


struct Join_req {
	uint32_t host_loc;
	uint16_t host_port;
	char* host_name; // NULL terminated string

	Join_req(uint32_t _loc = 0, uint16_t _port = 0, char* _name = NULL);
	~Join_req();
};


struct Join_res {
	char uoid[20];
	uint32_t distance;
	uint16_t host_port;
	char* host_name; // NULL terminated string

	Join_res(char* _uoid, uint32_t _dist = 0, uint16_t _port = 0, char* _name = NULL);
	~Join_res();
};


struct Hello {
	uint16_t host_port;
	char* host_name;

	Hello(uint16_t _port = 0, char* name = NULL);
	~Hello();
};

struct Notify {
	char error;
	
	Notify(char _error = 0);
};

struct Check_res {
	char uoid[20];
	
	Check_res(char *_uoid);
};

struct Status_req {
	char type;

	Status_req(char _type = 0);
};

struct Status_res {
	char uoid[20];
	uint16_t hostinfo; //length of the data for host information ... port && name
	uint16_t host_port;
	char *host_name;
	//uint32_t record_len;
	char *data;

	Status_res(char* _uoid, uint16_t _hostinfo = 0, uint16_t _host_port = 0, char *_host_name = NULL, uint32_t record_len = 0, char *_data = NULL);
	~Status_res();
};

struct Search_req {
	char search_type;	// 1: file name, 2: SHA1, 3: a list of keywords.
	//char* query;
	vector < string > query;

	Search_req(char _search_type, vector<string> &_query);
	~Search_req();
};

struct Search_res {
	char uoid[20];
	//uint32_t record_len;

	char* search_data;	
	
	Search_res(char* _uoid, char* _search_data = NULL);
	~Search_res();
};

struct Get_req {
	char file_id[20];
	char sha1[20];
	
	Get_req(char* _file_id, char* _sha1);
	~Get_req();
};

struct Get_res {
	char uoid[20];
	uint32_t metadata_len;
	uint32_t file_no;

	Get_res(char* _uoid, uint32_t _metadata_len, uint32_t _file_no);
	~Get_res();
};

struct Store {
    uint32_t metadata_len;
	uint32_t file_no;

    Store(uint32_t _metadata_len = 0, uint32_t _file_no = 0);
    ~Store();
};

/*
struct Store {
	uint32_t metadata_len;
	char* metadata;
	uint32_t file_no;

	Store(uint32_t _metadata_len, char* _metadata, uint32_t _file_no);
	~Store();
};
*/

struct Del {
	string filename;
	char sha1[20];
	char nonce[20];
	char password[20];

	Del(string _filename, char* _sha1, char *_nonce, char *_password);
	~Del();
};

struct Status {
	char status_type;	// 1: neighbors, 2: files

	Status(char _status_type);
	~Status();
};

struct Meta {
	//string filename;
	//stirng filesize;
	string fileid; //Is a 20 char string .. so the exact hash how it should be
	string sha1;   //Is a 40 char string .. in the hex format ... need to convert it to 20 char
	//string nonce;
	Meta(string _fileid = "", string _sha1 = "");
};


int getuoid(char *node_inst_id, char *obj_type, char *uoid_buf, unsigned int uoid_buf_size);

// Message creators
int join_req(Hdr** hdr_p, void** vmsg_p);
int join_res(Hdr** hdr_p, void** vmsg_p, Hdr* hdr1, void* vmsg1);
int hello(Hdr** hdr_p, void** vmsg_p);
int notify(Hdr** hdr_p, void** vmsg_p, char error);
int check_req(Hdr** hdr_p, void** vmsg_p);
int check_res(Hdr** hdr_p, void** vmsg_p, Hdr *chdr);
int status_req(Hdr** hdr_p, void** vmsg_p, char status, char ttl);
int status_res(Hdr** hdr_p, void** vmsg_p, Hdr* shdr, uint32_t record_len, char *data);
int create_status_res(Hdr** hdr_p, void** vmsg_p, Hdr *shdr);
int status_res_files(Hdr** hdr_p, void** vmsg_p, Hdr* shdr, vector<int> file_no);
int store(Hdr** hdr_p, void** vmsg_p, int file_no, unsigned char ttl, uint32_t metasize, uint32_t filesize );
int search_req(Hdr** hdr_p, void** vmsg_p, char type, vector<string> &data);
int search_res(Hdr** hdr_p, void** vmsg_p, Hdr *shdr, void *smsg, set<int> file_nos);
int get_req(Hdr** hdr_p, void** vmsg_p, char *file_id, char *sha_hash);
int get_res(Hdr** hdr_p, void** vmsg_p, Hdr *shdr,  void *smsg, int fileno);


// Fuctions that send/receive message
int log_msg(FILE* fp, Hdr* hdr, void* vmsg, char rfs, int sd = 0);
int log_err(FILE* fp, string str, bool err);


int send_msg(int sd, Hdr* hdr, void* vmsg);
int recv_msg(int sd, Hdr** hdr, void** vmsg);
int dup_msg(Hdr** hdr_p, void** vmsg_p, Hdr* hdr1, void* vmsg1);

// Functions that send a particular message directly
void post_hello(int sd);
void send_notify(char error, int sd);
void del(Hdr** hdr_p, void** vmsg_p, string filename, char *sha1, char *nonce, char *pass);
#endif
