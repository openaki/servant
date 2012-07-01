// util.h

#ifndef UTIL_H__
#define UTIL_H__

#include <string>
#include "msg.h"

using namespace std;

// GENERAL
void Tokenize(const string& str, vector<string>& tokens, const string& delimiters = " =\"");
int cstr2string(const char* cstr, string& s, int len);
int get_peerid_from_msg(Hdr* hdr, void* vmsg, string& peerid);
string make_hostid(string name, unsigned long port);
bool flip_coin(double prob);
void print_hdr(Hdr* hdr);
void printarrayinhex(const char *data, int len);

// UOID
int sha1_file(char* file_name, unsigned char* sha1_sum);
int sha1_file(FILE* fp, unsigned char* sha1_sum);
void print_sha1(unsigned char* _sha1);
void print_sha1(unsigned char* _sha1, FILE *fp);
void generate_nonce(unsigned char *ans, unsigned char *pass);
void generate_password(unsigned char *ans);
void sha1hex2byte(string sha_s, char *sha_hash);        // hex40 -> byte40
void sha1byte2hex(const char *sha_hash, string &sha_s); // byte20 -> hex40
//void sha1byte2hex(char *byte20, string& hex40);       // byte20 -> hex40 by Bob.


// FILE INFO
void temp_file_name(int file_no, string& meta_file, string& data_file);
void file_name(int file_no, string& meta_file, string& data_file);
long long int get_filesize(string filename);
int read_temp_meta_file(int file_no, string& filename, string& sha1, string& nonce);
int read_meta_file(int file_no, string& filename, string& sha1, string& nonce);

// FILE TRANSFER
int send_file(FILE* fp, int sd);
int recv_file(FILE* fp, int sd, uint32_t size);

// FILE CREATION & DELETION
int copy_file(const char *fnsource, const char *fntarget);
int copy_mini2tempfile(int fileno);
int copy_tempfile2mini(int fileno);
int del_file(vector<int>& files);
int del_file(int file_no);
int del_tempfile(int file_no);


#endif
