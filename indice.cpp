#include <fstream>
#include "indice.h"
#include "util.h"

Indice g_indice;

Indice::Indice() {
	int state = pthread_mutex_init(&m_mutex, NULL);
	if(state) {
        printf("mutex_indice init failed.");
        exit(1);	
	}
}

Indice::~Indice() {
	pthread_mutex_destroy(&m_mutex);
}

void Indice::clear() {
	pthread_mutex_lock(&m_mutex);

	m_name2set.clear();
	m_sha12set.clear();
	m_kwrd2set.clear();

	pthread_mutex_unlock(&m_mutex);
}

	
int Indice::init_home(string homedir) {
	if(homedir.size() == 0) {
		return -1;
	}
	else {
		m_homedir = homedir;	
		return 0;
	}
}

int Indice::read_meta_file(int file_no) {
	int err = 0;
	char key[200] = "";
	char val[1000] = "";
	char file_name[200] = "";
	ssize_t read;
	
	sprintf(file_name, "%sfiles/%d.meta", m_homedir.c_str(), file_no);
	if(DEBUG_INDICE) printf("read-meta-file(): %sfiles/%d.meta \n", m_homedir.c_str(), file_no);

	
	ifstream file;
  	file.open(file_name, ios::in );
  	
  	file.getline(val, 200);
	if(strcmp(val, "[metadata]\r")) {
		err = -1;
		return err;
	}
	if(DEBUG_INDICE) cout << string(val) << endl;
  	
	while(!file.eof())
	{
	  	file.getline(key, 200, '=');
		read = file.gcount();

	  	file.getline(val, 1000);
		read = file.gcount();
		val[read-2] = '\0';
		
		// Need to lowercase the key?
		
		if (strcmp(key, "FileName") == 0) {
			m_name2set[string(val)].insert(file_no);
			if(DEBUG_INDICE) cout << string(key) << " = " << string(val) << endl;
		}
		else if (strcmp(key, "SHA1") == 0) {
			m_sha12set[string(val)].insert(file_no);
			if(DEBUG_INDICE) cout << string(key) << " = " << string(val) << endl;
		}
		else if (strcmp(key, "Bit-vector") == 0) {
			bitvector bits;
			bits.set(val); //** BUG BUG BUG
			if(DEBUG_RC1) printf("----- INDICE ------\n");
			if(DEBUG_RC1) bits.print_hex();
			if(DEBUG_RC1) printf("-------------------\n");
			m_kwrd2set[bits].insert(file_no);
			if(DEBUG_INDICE) cout << string(key) << " = " << string(val) << endl;
		}
		else if (strcmp(key, "Nonce") == 0) {
			m_nonce2set[string(val)].insert(file_no);
			if(DEBUG_INDICE) cout << string(key) << " = " << string(val) << endl;
		}
	}
	file.close();
	
	return err;
}

int Indice::read_name_index_file() {
	int err = 0;

	char file_name[200] = "";
	sprintf(file_name, "%sname_index", m_homedir.c_str());

	ifstream file (file_name);
	if (file.is_open()) {
		while(!file.eof())
		{
			string s;
			int num = 0;
			int file_no = 0;

		  	file >> s;
		  	file >> num;

			for(int i = 0; i < num; i++) {	  	
				file >> file_no;
				m_name2set[s].insert(file_no);
			}
		}
		file.close();
	}
	else {
		err = -1;
	}	
	
	return err;
}

int Indice::read_sha1_index_file() {
	int err = 0;

	char file_name[200] = "";
	sprintf(file_name, "%ssha1_index", m_homedir.c_str());

	ifstream file (file_name);
	if (file.is_open()) {
		while(!file.eof())
		{
			string s;
			int num = 0;
			int file_no = 0;

		  	file >> s;
		  	file >> num;

			for(int i = 0; i < num; i++) {	  	
				file >> file_no;
				m_sha12set[s].insert(file_no);
			}
		}
		file.close();
	}
	else {
		err = -1;
	}	
	
	return err;
}


int Indice::read_kwrd_index_file() {
	int err = 0;

	char file_name[200] = "";
	sprintf(file_name, "%skwrd_index", m_homedir.c_str());

	ifstream file (file_name);
	if (file.is_open()) {
		while(!file.eof())
		{
			string s;
			int num = 0;
			int file_no = 0;

		  	file >> s;
		  	file >> num;

			bitvector bits;
			bits.set((char*) s.c_str());
			for(int i = 0; i < num; i++) {	  	
				file >> file_no;
				m_kwrd2set[bits].insert(file_no);
			}
		}
		file.close();
	}
	else {
		err = -1;
	}	
	
	return err;
}

int Indice::read_nonce_index_file() {
	int err = 0;

	char file_name[200] = "";
	sprintf(file_name, "%snonce_index", m_homedir.c_str());

	ifstream file (file_name);
	if (file.is_open()) {
		while(!file.eof())
		{
			string s;
			int num = 0;
			int file_no = 0;

		  	file >> s;
		  	file >> num;

			for(int i = 0; i < num; i++) {	  	
				file >> file_no;
				m_nonce2set[s].insert(file_no);
			}
		}
		file.close();
	}
	else {
		err = -1;
	}	
	
	return err;
}

int Indice::read_index_file() {
	int err = 0;

	read_name_index_file();
	read_sha1_index_file();
	read_kwrd_index_file();
	read_nonce_index_file();
	
	return err;
}

int Indice::write_name_index_file() {
	int err = 0;
	FILE* fp = NULL;

	char file_name[200] = "";
	sprintf(file_name, "%sname_index", m_homedir.c_str());
	fp = fopen(file_name, "w");
	if(!fp) {
		return -1;
	}
	
	map<string, set<int> >::iterator itr;
	for(itr = m_name2set.begin(); itr != m_name2set.end(); itr++) {
		set<int>& myset = itr->second;
		fprintf(fp, "%s %d", itr->first.c_str(), myset.size());		
		
		set<int>::iterator itr2;
		for(itr2 = myset.begin(); itr2 != myset.end(); itr2++) {
			fprintf(fp, " %d", *itr2);
		}
		fprintf(fp, "\n");
	}	
		
	fclose(fp);	
	return err;
}

int Indice::write_sha1_index_file() {
	int err = 0;
	FILE* fp = NULL;

	char file_name[200] = "";
	sprintf(file_name, "%ssha1_index", m_homedir.c_str());
	fp = fopen(file_name, "w");
	if(!fp) {
		return -1;
	}

	map<string, set<int> >::iterator itr;
	for(itr = m_sha12set.begin(); itr != m_sha12set.end(); itr++) {
		set<int>& myset = itr->second;
		fprintf(fp, "%s %d", itr->first.c_str(), myset.size());

		set<int>::iterator itr2;
		for(itr2 = myset.begin(); itr2 != myset.end(); itr2++) {
			fprintf(fp, " %d", *itr2);
		}
		fprintf(fp, "\n");
	}

	fclose(fp);	
	return err;
}

int Indice::write_kwrd_index_file() {
	int err = 0;
	FILE* fp = NULL;

	char file_name[200] = "";
	sprintf(file_name, "%skwrd_index", m_homedir.c_str());
	fp = fopen(file_name, "w");
	if(!fp) {
		return -1;
	}

	map<bitvector, set<int> >::iterator itr;
	for(itr = m_kwrd2set.begin(); itr != m_kwrd2set.end(); itr++) {
		//itr->first.print_hex(fp);
		bitvector bits = itr->first;
		bits.print_hex(fp);

		set<int>& myset = itr->second;
		fprintf(fp, " %d", myset.size());

		set<int>::iterator itr2;
		for(itr2 = myset.begin(); itr2 != myset.end(); itr2++) {
			fprintf(fp, " %d", *itr2);
		}
		fprintf(fp, "\n");
	}

	fclose(fp);	
	return err;
}

int Indice::write_nonce_index_file() {
	int err = 0;
	FILE* fp = NULL;

	char file_name[200] = "";
	sprintf(file_name, "%snonce_index", m_homedir.c_str());
	fp = fopen(file_name, "w");
	if(!fp) {
		return -1;
	}

	map<string, set<int> >::iterator itr;
	for(itr = m_nonce2set.begin(); itr != m_nonce2set.end(); itr++) {
		set<int>& myset = itr->second;
		fprintf(fp, "%s %d", itr->first.c_str(), myset.size());

		set<int>::iterator itr2;
		for(itr2 = myset.begin(); itr2 != myset.end(); itr2++) {
			fprintf(fp, " %d", *itr2);
		}
		fprintf(fp, "\n");
	}

	fclose(fp);	
	return err;
}

int Indice::write_index_file() {
	int err = 0;
	
	err = write_name_index_file();
	err = write_sha1_index_file();
	err = write_kwrd_index_file();
	err = write_nonce_index_file();
	
	return err;
}

int Indice::search_name(string& name, set<int> &s) {
	int num_item = 0;
	
	if(DEBUG_INDICE) cout << "name search: " << name << endl;
	map<string, set<int> >::iterator itr = m_name2set.find(name);
	if(itr != m_name2set.end()) {
		s = itr->second;
		num_item = 	s.size();
	}
	
	return num_item;
}


int Indice::search_sha1(string& sha1, set<int> &s) {
	int num_item = 0;
	
	map<string, set<int> >::iterator itr = m_sha12set.find(sha1);
	if(itr != m_sha12set.end()) {
		s = itr->second;
		num_item = 	s.size();
	}

	return num_item;
}

/*
int Indice::search_sha1(string& sha1, set<int> &s) {
	int num_item = 0;
	
	map<string, set<int> >::iterator itr = m_sha12set.find(sha1);
	if(itr != m_sha12set.end()) {
		s = itr->second;
		num_item = 	s.size();
	}

	return num_item;
}
*/

int Indice::search_sha1(const char *sha1, set<int> &s) {
	int num_item = 0;
	string s_sha;
	sha1byte2hex(sha1, s_sha);
	if(DEBUG_RC1) printf("-------------------\n");
	for(int i=0; i<40; i++) {
		if(DEBUG_RC1) printf("%c", s_sha[i]);
	}
	
	num_item = search_sha1(s_sha, s);
	return num_item;
}


int Indice::search_kwrds(vector<string>& kwrds, set<int> &s) {
	bitvector bits;

	vector<string>::iterator vitr;
	for(vitr = kwrds.begin(); vitr != kwrds.end(); vitr++) {
		if(DEBUG_INDICE) cout << *vitr << endl;
		bits.set(*vitr);	// Accumulate bit vectors for all keywords.
	}
	
	map<bitvector, set<int> >::iterator itr1;
	set<int>::iterator itr2;
	
	// Do linear search
	for(itr1 = m_kwrd2set.begin(); itr1 != m_kwrd2set.end(); itr1++) {
		// if bloom filter hits
		bitvector filter = itr1->first; // copy it before use it. since itr->first returns const.
		if( filter.is_hit(bits) ) {
			//TODO: This search should be extended to perform deep search.
			//      i.e., search for actual keywords in meta file.
			for(itr2 = itr1->second.begin(); itr2 != itr1->second.end(); itr2++) {
				s.insert(*itr2);
			}
		}
	}

	return s.size();
}

int Indice::search_nonce(string& nonce, set<int> &s) {
	int num_item = 0;
	
	map<string, set<int> >::iterator itr = m_nonce2set.find(nonce);
	if(itr != m_nonce2set.end()) {
		s = itr->second;
		num_item = 	s.size();
	}

	return num_item;
}

/*
int Indice::search(char search_type, vector<string>& kwrds, set<int> &s) {

	if (kwrds.empty()) {
		return -1;
	}
		
	switch(search_type) {
		case 1:
			search_name(kwrds[0], s);
			break;
		case 2:
			search_sha1(kwrds[0], s);
			break;
		case 3:
			search_kwrds(kwrds, s);
			break;
		case 4:
			search_nonce(kwrds[0], s);
			break;
		default:
			break;
	}

	return s.size();	
}
*/
int Indice::search(char search_type, vector<string>& kwrds, set<int> &s) {

	if (kwrds.empty()) {
		return -1;
	}
		
	switch(search_type) {
		case 1:
			search_name(kwrds[0], s);
			break;
		case 2:
			search_sha1((const char *)kwrds[0].c_str(), s);
			break;
		case 3:
			search_kwrds(kwrds, s);
			break;
		case 4:
			search_nonce(kwrds[0], s);
			break;
		default:
			break;
	}

	return s.size();	
}


int Indice::search_20(char search_type, vector<string>& kwrds, set<int> &s) {

	string hex40;
	sha1byte2hex(kwrds[0].c_str(), hex40);
	
	if (kwrds.empty()) {
		return -1;
	}
		
	switch(search_type) {
		case 1:
			search_name(hex40, s);
			break;
		case 2:
			search_sha1(hex40, s);
			break;
		case 4:
			search_nonce(hex40, s);
			break;
		default:
			break;
	}

	return s.size();	
}

int Indice::search(string& name, string& sha1, string& nonce, set<int>& s) {
	set<int> set1;
	set<int> set2;
	set<int> set3;
	set<int> set4;

	if(DEBUG_RC1) cout << name << endl;
	if(DEBUG_RC1) cout << sha1 << endl;
	if(DEBUG_RC1) cout << nonce << endl;
	
	search_name(name, set1);
	search_nonce(nonce, set2);
	search_sha1(sha1, set3);
	
	set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(), inserter(set4, set4.begin()) );
	set_intersection(set3.begin(), set3.end(), set4.begin(), set4.end(), inserter(s, s.begin()) );

	return s.size();
}

int Indice::search_20(string& _name, char* _sha1, char* _nonce, set<int>& _s) {
	string sha1;
	string nonce;
	sha1byte2hex(_sha1, sha1); // byte20 -> hex40
	sha1byte2hex(_nonce, nonce); // byte20 -> hex40
	
	return search(_name, sha1, nonce, _s);
}

void Indice::erase(string& name, string& sha1, string& nonce) {

	set<int> files;
	search(name, sha1, nonce, files);
	
	for(set<int>::iterator itr = files.begin(); itr != files.end(); itr++) {
		int fileno = *itr;
		
		map<string, set<int> >::iterator itr1 = m_sha12set.find(sha1);
		itr1->second.erase(fileno);
		
		map<string, set<int> >::iterator itr2 = m_name2set.find(name);
		itr2->second.erase(fileno);

		map<string, set<int> >::iterator itr3 = m_nonce2set.find(nonce);
		itr3->second.erase(fileno);
		
		map<bitvector, set<int> >::iterator itr4;
		for(itr4 = m_kwrd2set.begin(); itr4 != m_kwrd2set.end(); itr4++) {
			itr4->second.erase(fileno);	
		}
	}
}

void Indice::erase_20(string& _name, char* _sha1, char* _nonce) {
	string sha1;
	string nonce;
	sha1byte2hex(_sha1, sha1); // byte20 -> hex40
	sha1byte2hex(_nonce, nonce); // byte20 -> hex40

	erase(_name, sha1, nonce);
}


int read_fileid_index_file(map<string, int>& fileid2fileno, map<int, string>& fileno2fileid, char* homedir, int& global_fileno, int& global_temp_fileno) {
	int err = 0;

	char file_name[200] = "";
	char sha_hash[20] = "";
	sprintf(file_name, "%sfileid_index", homedir);

	ifstream file (file_name, ios::in | ios::binary);
	if (file.is_open()) {
		file >> global_fileno;
		file >> global_temp_fileno;

		while(!file.eof())
		{
			string fileid_hex;
			int fileno;

		  	file >> fileid_hex;
			if(file.eof()) 
				break;
		  	file >> fileno;

			//bitvector bits;
			//bits.set((char*) fileid_hex.c_str());
			
			sha1hex2byte(fileid_hex, sha_hash);
			string fileid;
			cstr2string(sha_hash, fileid, 20);
			
			fileid2fileno[fileid] = fileno;
			fileno2fileid[ fileno ] = fileid;
			
			if(DEBUG_RC1) printf("-----------------------------------\n");
			if(DEBUG_RC1) printarrayinhex(fileid.c_str(), 20);
			if(DEBUG_RC1) printf("-------------------------------------\n");
		}
		file.close();
	}
	else {
		err = -1;
	}	
	
	return err;
}

int write_fileid_index_file(map<string, int>& fileid2fileno, char* homedir, int global_fileno, int global_temp_fileno) {
	FILE* fp = NULL;

	char file_name[200] = "";
	sprintf(file_name, "%sfileid_index", homedir);
	fp = fopen(file_name, "w");
	if(!fp) {
		return -1;
	}
	
	fprintf(fp, "%d\n", global_fileno);
	fprintf(fp, "%d\n", global_temp_fileno);

	map<string, int >::iterator itr;
	for(itr = fileid2fileno.begin(); itr != fileid2fileno.end(); itr++) {
		string fileid = itr->first;
		print_sha1((unsigned char*)fileid.c_str(), fp);
		fprintf(fp, " %d\n", itr->second);
	}	
		
	fclose(fp);
	return 0;
}

int write_fileno2perm_index_file(map<int, bool>& fileno2perm, char* homedir) {
	FILE* fp = NULL;

	char file_name[200] = "";
	sprintf(file_name, "%sfileno2perm_index", homedir);
	fp = fopen(file_name, "w");
	if(!fp) {
		return -1;
	}
	
	int num_files = fileno2perm.size();
	fprintf(fp, "%d\n", num_files);

	map<int, bool>::iterator itr;
	for(itr = fileno2perm.begin(); itr != fileno2perm.end(); itr++) {
		if(itr->second == true)
			fprintf(fp, "%d %d\n", itr->first, 1);
		else
			fprintf(fp, "%d %d\n", itr->first, 0);		
	}
	fprintf(fp, "\n");
		
	fclose(fp);
	return 0;
}

int read_fileno2perm_index_file(map<int, bool>& fileno2perm, char* homedir) {
	int err = 0;

	char file_name[200] = "";
	sprintf(file_name, "%sfileno2perm_index", homedir);

	ifstream file (file_name, ios::in);
	if (file.is_open()) {
		int num_files = 0;
		file >> num_files;

		for(int i = 0; i < num_files; i++) {
			int file_no;		
			int is_perm;
		
		  	file >> file_no;
			if(file.eof()) 
				break;
		  	file >> is_perm;
			
			if(is_perm == 1)	
				fileno2perm[file_no] = true;
			else			
				fileno2perm[file_no] = false;
		}
		file.close();
	}
	else {
		err = -1;
	}	
	return err;
}


/*
int main() {
	Indice i;	
	
	i.init_home(string("./15900/"));
	i.read_meta_file(0);
	i.write_index_file();

	i.clear();
	i.read_index_file();

	i.init_home(string("./15901/"));
	i.write_index_file();
	
	return 0;
}
*/
