#include <stdio.h>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <iostream>
#include <openssl/sha.h>
#include "constants.h"
#include "error.h"

using namespace std;

//bool DEBUG = true;

class Event 
{
public:
	char type;
	int sd;

	Event();
	~Event();
};

Event::Event() {
	type = 0x00;
}


Event::~Event() {
	if(DEBUG) printf(" Event::~Event()\n");
}

void create_s() {
	printf("In create_s() \n");
	Event ev;
	return;
}

void create_d() {
	printf("In create_d() \n");
	Event* ev = new Event;
	return;
}


int cstr2string(const char* cstr, string& s, int len) {
	int i = 0;
	
	if(cstr) {
		for(i = 0; i < len; i++) { 
			s += *cstr++;
		}
	}
	
	return i;
}

/*
int sha1_file(FILE* fp, unsigned char* sha1_sum) {

	SHA_CTX ctx;
	char file_buff[FILE_BUFF_SIZE];
	size_t len = 0;

	if(!fp || !sha1_sum) {
		return ARG_NULL;
	}

	if(!SHA1_Init(&ctx) ) {
		return ERR;
	}        
	
	while( (len = fread(file_buff, sizeof(char), FILE_BUFF_SIZE, fp)) ) {
		if(!SHA1_Update(&ctx, file_buff, len))
			return ERR;
	}

	if(!SHA1_Final(sha1_sum, &ctx))
		return ERR;
	else
		return 0;
}

int sha1_file(char* file_name, unsigned char* sha1_sum) {
	int err = 0;

	if(!file_name || !sha1_sum) {
		return ARG_NULL;
	}

	FILE* fp = fopen(file_name, "r");
	if(fp) {
		err = sha1_file(fp, sha1_sum);
		fclose(fp);
	}		

	return err;
}

void print_sha1(unsigned char* _sha1) {
	for(int i = 0; i < 20; i++) {
		printf("%02x", _sha1[i]);
	}
	printf("\n");
}
*/

void sha1hex2byte(string sha_s, char *sha_hash)
{
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
	return ;
}

// 20-byte to 40-hex conversion
void byte2sha1hex(char *byte20, string& hex40) 
{
	char lo_mask = 0x0f;
	 
	char byte40[41] = "";
	char* p = byte40;

	for(int i = 0; i < 20; i ++) {
		char ch1 = (byte20[i] >> 4) & lo_mask;
		char ch2 = byte20[i] & lo_mask;

		if(ch1 >= 10)
			*p++ = 'a' + ch1 - 10;
		else
			*p++ = '0' + ch1;
			
		if(ch2 >= 10)
			*p++ = 'a' + ch2 - 10;
		else
			*p++ = '0' + ch2;
	}
	
	cstr2string(byte40, hex40, 40);

	return;
}

void sha1byte2hex(const char *sha_hash, string &sha_s)
{
	char buf[100] = "";
	for(int i = 0; i< 20; i++) {
		sprintf(buf, "%s%02x", buf, (unsigned char)sha_hash[i]);
	}
	sha_s = buf;
	transform(sha_s.begin(), sha_s.end(), sha_s.begin(), (int(*)(int)) std::tolower);
	
	return ;
}


int main() {
	map<string, long> mymap;
	mymap[string("aaa")] = 10;
	mymap[string("bbb")] = 20;
	
	map<string, long>::iterator itr;
	for(itr = mymap.begin(); itr != mymap.end(); itr++) {
		itr->second = itr->second + 1;
	}

	return 0;
}

/*
int main() {
	char byte20[20] = {0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 
						0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c };
	

	string s1 = string("01234567890abcdef0123456789abcdef0123456");
	string s2;
	
	sha1hex2byte(s1, byte20);
	//byte2sha1hex(byte20, s2);
	sha1byte2hex(byte20, s2);
	
	cout << s1 << endl;
	cout << s2 << endl;
	return 0;
}
*/
/*
int main() {
	set<int> set1;
	set<int> set2;
	set<int> set3;
	insert_iterator<set<int> > ii(set3, set3.begin()); 
	
	set1.insert(5);
	set1.insert(1);
	set1.insert(3);

	set2.insert(2);
	set2.insert(6);
	set2.insert(1);
	set2.insert(4);

	set<int>::iterator itr1;
	for(itr1 = set1.begin(); itr1 != set1.end(); itr1++) {
		cout << *itr1 << " " ;	
	}
	cout << endl;

	set<int>::iterator itr2;
	for(itr2 = set2.begin(); itr2 != set2.end(); itr2++) {
		cout << *itr2 << " " ;	
	}
	cout << endl;


	//set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(), std::back_inserter(set3));
	//set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(), std::back_inserter(set3));
	//set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(), ii);
	set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(), inserter(set3, set3.begin()) );

	
	set<int>::iterator itr3;
	for(itr3 = set3.begin(); itr3 != set3.end(); itr3++) {
		cout << *itr3 << " " ;	
	}
	cout << endl;
	
}
*/

/*
int main() {
	create_s();
	create_d();

	char cstr19[19] = "123456789012345678";
	char cstr20[20] = "1234567890123456789";
	char cstr21[21] = "12345678901234567890";

	string str19;
	string str20;
	string str21;

	int num = 0;
	num = cstr2string(cstr19, str19, 20);
	cout << str19 << " copied bytes" << num << endl;
	
	num = cstr2string(cstr20, str20, 20); // NULL copied
	cout << str20 << " copied bytes" << num << endl;
	
	num = cstr2string(cstr21, str21, 20); // no NULL copied	
	cout << str21 << " copied bytes" << num << endl; // cout skips NULL when printing.

	set_sha1(NULL);

	unsigned char sha1_sum[20] = "";	
	FILE* fp = fopen("sv_node", "r");
	sha1_file(fp, sha1_sum);
	print_sha1(sha1_sum);
	fclose(fp);

	sha1_file("sv_node", sha1_sum);
	print_sha1(sha1_sum);
	
	return 0;
}
*/
