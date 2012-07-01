#ifndef INDICE_H__
#define INDICE_H__

#include <string>
#include <set>
#include <map>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include "bitvector.h"
#include "constants.h"

using namespace std;

class Indice
{
	string m_homedir;
	map<string, set<int> >    m_name2set; // file_name to set
	map<string, set<int> >    m_sha12set; // sha1 to set
	map<bitvector, set<int> > m_kwrd2set; // bitvector to set
	map<string, set<int> >    m_nonce2set; // nonce to set
	pthread_mutex_t m_mutex;
	
	int read_name_index_file();
	int read_sha1_index_file();
	int read_kwrd_index_file();
	int read_nonce_index_file();

	int write_name_index_file();
	int write_sha1_index_file();
	int write_kwrd_index_file();
	int write_nonce_index_file();
	
	int search_name(string& name, set<int> &s);
	int search_sha1(string& sha1, set<int> &s);
	int search_sha1(const char *sha1, set<int> &s);	
	//int search_kwrd(string& kwrd, set<int> &s);
	int search_kwrds(vector<string>& kwrds, set<int> &s);	// AND based search for multiple keywords
	int search_nonce(string& nonce, set<int> &s);
	

public:
	Indice();
	~Indice();
	void clear();
	
	int init_home(string homedir);		// home directory should be initialized before using.
	int read_meta_file(int file_no);
	int read_index_file();
	int write_index_file();

	// search_type: 1=name, 2=sha1, 3=keywords, 4=nonce
	int search(char search_type, vector<string>& kwrds, set<int> &s);	
	int search_20(char search_type, vector<string>& kwrds, set<int> &s);
	int search(string& name, string& sha1, string& nonce, set<int>& s);
	int search_20(string& _name, char* _sha1, char* _nonce, set<int>& _s);
	void erase(string& name, string& sha1, string& nonce);
	void erase_20(string& _name, char* _sha1, char* _nonce);
};

extern Indice g_indice;

//int read_fileid_index_file(map<string, int>& fileid2fileno, map<int, string>& fileno2fileid, char* homedir, int& global_fileno);
//int write_fileid_index_file(map<string, int>& fileid2fileno, char* homedir, int global_fileno);

int read_fileid_index_file(map<string, int>& fileid2fileno, map<int, string>& fileno2fileid, char* homedir, int& global_fileno, int& global_temp_fileno);
int write_fileid_index_file(map<string, int>& fileid2fileno, char* homedir, int global_fileno, int global_temp_fileno);

int read_fileno2perm_index_file(map<int, bool>& fileno2perm, char* homedir);
int write_fileno2perm_index_file(map<int, bool>& fileno2perm, char* homedir);



#endif


