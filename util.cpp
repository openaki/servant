#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <fstream>

#include "util.h"
#include "error.h"
#include "constants.h"
#include "globals.h"

//Tokenize a stiring into vector of string .... 
void Tokenize(const string& str, vector<string>& tokens, const string& delimiters)
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


// Copies "len" bytes from c str to c++ string
//
int cstr2string(const char* cstr, string& s, int len) {
	int i = 0;
	
	if(cstr) {
		for(i = 0; i < len; i++) { 
			s += *cstr++;
		}
	}
	
	return i;
}


// Retrieve peerid from hdr&msg, write it to peerid
// CAUTION: previous contents of peerid will be erased.
int get_peerid_from_msg(Hdr* hdr, void* vmsg, string& peerid) {
	char _peerid[80] = "";

	if(!hdr || !vmsg) {
		return -1;
	}
			
	switch(hdr->msg_type) {
		case JNRQ:
			{
				Join_req* msg = (Join_req*)vmsg;
				sprintf(_peerid, "%s_%u", msg->host_name, msg->host_port);
			}
			break;
	
		case JNRS:
			{
				Join_res* msg = (Join_res*)vmsg;
				sprintf(_peerid, "%s_%u", msg->host_name, msg->host_port);
			}				
			break;
	
		case HLLO:
			{
				Hello* msg = (Hello*)vmsg;
				sprintf(_peerid, "%s_%u", msg->host_name, msg->host_port);
			}				
			break;
			
		default:
			break;		
	}

	peerid = string(_peerid);
	return 0;
}


string make_hostid(string name, unsigned long port) {
	string hostid = name;
	hostid += "_";
	stringstream out_stream;
	out_stream << port;
	string s_temp = out_stream.str();
	hostid += s_temp;
	
	return hostid;
}

// flips a coin.
// argument
//	 prob: probabilty to be true;
//
bool flip_coin(double prob) {
	double drand = drand48();

	if(drand < prob) {
		return true;				
	}				
	else {
		return false;
	}	
}

void print_hdr(Hdr* hdr) {
	if(!hdr) 
		return;
		
	printf("msg_type=%x\n", hdr->msg_type);
	printf("uoid=");
	for(int i = 0; i < 20; i++) {
		printf("%x", hdr->uoid[i]);
	}
	//printf("\n");
	printf("TTL=%u\n", hdr->ttl);
	printf("data_len=%u\n", hdr->data_len);
}

void temp_file_name(int file_no, string& meta_file, string& data_file) {
	char meta_file_name[200];
	char data_file_name[200];
	
	sprintf(meta_file_name, "%s%s%d%s", ini.homedir, "files/temp_", file_no, ".meta");
	sprintf(data_file_name, "%s%s%d%s", ini.homedir, "files/temp_", file_no, ".data");

	meta_file = string(meta_file_name);
	data_file = string(data_file_name);
}

void file_name(int file_no, string& meta_file, string& data_file) {
	char meta_file_name[200];
	char data_file_name[200];
	
	sprintf(meta_file_name, "%s%s%d%s", ini.homedir, "files/", file_no, ".meta");
	sprintf(data_file_name, "%s%s%d%s", ini.homedir, "files/", file_no, ".data");

	meta_file = string(meta_file_name);
	data_file = string(data_file_name);
}

long long int get_filesize(string filename)
{
	struct stat sb;
	if (stat(filename.c_str(), &sb) == -1) {
		//perror("stat");
		return -1;
	}
	return sb.st_size;

}
			
// Takes file_name
// and compute SHA1 of the file.
//
int sha1_file(char* file_name, unsigned char* sha1_sum) {
	int err = 0;

	if(!file_name || !sha1_sum) {
		return ERR_ARG_NULL;
	}

	FILE* fp = fopen(file_name, "r");
	if(fp) {
		err = sha1_file(fp, sha1_sum);
		fclose(fp);
	}		

	return err;
}

// Takes FILE*
// and compute SHA1 of the file.
//
int sha1_file(FILE* fp, unsigned char* sha1_sum) {

	SHA_CTX ctx;
	char file_buff[FILE_BUFF_SIZE];
	size_t len = 0;

	if(!fp || !sha1_sum) {
		return ERR_ARG_NULL;
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

void print_sha1(unsigned char* _sha1) {
	for(int i = 0; i < 20; i++) {
		printf("%02x", _sha1[i]);
	}
	printf("\n");
}

void print_sha1(unsigned char* _sha1, FILE *fp) {
    for(int i = 0; i < 20; i++) {
        fprintf(fp, "%02x", _sha1[i]);
    }
}

void generate_password(unsigned char *ans) {
    long val = lrand48();
    char va[10];
    snprintf(va, 10, "%ld", val);
    SHA1((unsigned char *)va, strlen(va), ans);
}

void generate_nonce(unsigned char *ans, unsigned char *pass) {
    SHA1(pass, 20, ans);
}

int send_file(FILE* fp, int sd) {
	char file_buff[FILE_BUFF_SIZE] = "";
	int err = NO_ERR;
	
	if(!fp || !sd) {
		return ERR_ARG_NULL;
	}

	size_t num_read = 0;
	ssize_t num_write = 0;
	do {
		// Read from file
		num_read = fread(file_buff, 1, FILE_BUFF_SIZE, fp); 
		// You may add error check for fread() here.

		// Write to socket
		//file_buff[FILE_BUFF_SIZE-1] = '\0';
		//printf("%s \n", file_buff);
		num_write = write(sd, file_buff, num_read);
		if(num_write == -1 || num_read != (size_t)num_write) {
			err = ERR_SOCK_WRITE;
			break;
		}
	} while (num_read == FILE_BUFF_SIZE);
			
	return err;	
}


int recv_file(FILE* fp, int sd, uint32_t _size) {
	char file_buff[FILE_BUFF_SIZE] = "";
	int err = 0;
	unsigned int byte_written = 0;	
	uint32_t size = _size;

	fd_set readset; 
	FD_ZERO(&readset);
	FD_SET(sd, &readset);
	
	timeval timeout;
	timeout.tv_sec = SOC_TIME_OUT_SEC;
	timeout.tv_usec = SOC_TIME_OUT_USEC;

	if(!fp || !sd || !size) {
		return -2;
	}

	for(unsigned long i = 0; i < _size; i += byte_written) {
		fd_set readset_tmp = readset;
		timeval timeout_tmp = timeout;
		byte_written = 0;

		// First check if there is data to be read
		int result = select(sd+1, &readset_tmp, 0, 0, &timeout_tmp);

		if(result == -1) {        	// error in select
			err = ERR;
		}
		else if(result == 0) {    	// time out
			err =  ERR_SOCK_TIMEOUT;    
		}
		else {                    	// read data
			if(FD_ISSET(sd, &readset_tmp)) {
				ssize_t read_len = 0;
				ssize_t write_len = 0;

				// recv seems to be better at receiving EOF when peer disconnects.
				do {
					// Read from socket
					if(size < FILE_BUFF_SIZE){
						read_len = recv(sd, file_buff, size, MSG_WAITALL); //???
					}
					else {
						read_len = recv(sd, file_buff, FILE_BUFF_SIZE, MSG_WAITALL); //???	
					}
					size -= read_len;

					if(read_len <= 0) {
						err = ERR_SOCK_READ;
						break;
					}
					
					// Write to file
					//file_buff[FILE_BUFF_SIZE-1] = '\0';
					//printf("recv-file(): %s\n", file_buff);
					write_len = fwrite(file_buff, 1, read_len, fp); 
					if(read_len != write_len) {
						err = ERR_FILE_WRITE;
						break;
					}
					else {
						byte_written += write_len;
					}
				} while (write_len == FILE_BUFF_SIZE);
			}
		}
		
		if(err) {
			break;
		}
	}
		
	return err;
}


int copy_mini2tempfile(int fileno) {
	char src[200] = "";
	char dst[200] = "";
	int err = 0;
	int result = 0;

	sprintf(src, "%sfiles/%d.meta", ini.homedir, fileno);
	sprintf(dst, "%sfiles/temp_%d.meta", ini.homedir, fileno);

	result = copy_file(src, dst);
	if(result == -1) {
		return -1;
	}	

	sprintf(src, "%sfiles/%d.data", ini.homedir, fileno);
	sprintf(dst, "%sfiles/temp_%d.data", ini.homedir, fileno);
	
	result = copy_file(src, dst);
	if(result == -1) {
		// DELETE meta file here.
		char cmd1[200] = "";
		sprintf(cmd1, "%sfiles/temp_%d.meta", ini.homedir, fileno);
		err = remove(cmd1);
		return -1;
	}
	else {
		//g_fileno++;
		return 0;	
	}
}

/*
int copy_tempfile2mini(int fileno) {
	char src[200] = "";
	char dst[200] = "";
	int err = 0;
	int result = 0;
		
	sprintf(src, "%sfiles/temp_%d.meta", ini.homedir, fileno);
	sprintf(dst, "%sfiles/%d.meta", ini.homedir, g_fileno);

	result = copy_file(src, dst);
	if(result == -1) {
		return -1;
	}	
	
	sprintf(src, "%sfiles/temp_%d.data", ini.homedir, fileno);
	sprintf(dst, "%sfiles/%d.data", ini.homedir, g_fileno);
	
	result = copy_file(src, dst);
	if(result == -1) {
		// DELETE meta file here.
		char cmd1[200] = "";
		sprintf(cmd1, "%sfiles/%d.meta", ini.homedir, fileno);
		err = remove(cmd1);
		return -1;
	}
	else {
		return 0;	
	}
}
*/
int copy_tempfile2mini(int fileno) {
	char src[200] = "";
	char dst[200] = "";
	int err = 0;
		
	sprintf(src, "%sfiles/temp_%d.meta", ini.homedir, fileno);
	sprintf(dst, "%sfiles/%d.meta", ini.homedir, g_fileno);
	int result1 = copy_file(src, dst);

	sprintf(src, "%sfiles/temp_%d.data", ini.homedir, fileno);
	sprintf(dst, "%sfiles/%d.data", ini.homedir, g_fileno);
	int result2 = copy_file(src, dst);

	sprintf(src, "%sfiles/temp_%d.pass", ini.homedir, fileno);
	sprintf(dst, "%sfiles/%d.pass", ini.homedir, g_fileno);
	int result3 = copy_file(src, dst); 
	result3 = 1; // For now, don't test result3. (Only originator have one.)
	
	if(result1 == -1 || result2 == -1 || result3 == -1) {
		// DELETE meta file here.
		char cmd1[200] = "";
		sprintf(cmd1, "%sfiles/%d.meta", ini.homedir, fileno);
		err = remove(cmd1);

		// DELETE data file here.
		char cmd2[200] = "";
		sprintf(cmd2, "%sfiles/%d.data", ini.homedir, fileno);
		err = remove(cmd2);

		// DELET pass file here.
		char cmd3[200] = "";
		sprintf(cmd3, "%sfiles/%d.pass", ini.homedir, fileno);
		err = remove(cmd3);

		return -1;
	}	
	else {
		return 0;
	}	
}


int del_file(vector<int>& files) {
	int err = 0;
	
	vector<int>::iterator itr;
	for(itr = files.begin(); itr != files.end(); itr++) {
		err += del_file(*itr);	
	}
	
	return err;
}

int del_file(int file_no) {
	
	char cmd1[200] = "";
	char cmd2[200] = "";
	char cmd3[200] = "";
	int err1 = 0;
	int err2 = 0;
	int err3 = 0;
	
	sprintf(cmd1, "%sfiles/%d.meta", ini.homedir, file_no);
	sprintf(cmd2, "%sfiles/%d.data", ini.homedir, file_no);
	sprintf(cmd3, "%sfiles/%d.pass", ini.homedir, file_no);

	err1 = remove(cmd1);
	err2 = remove(cmd2);
	err3 = remove(cmd3);
	err3 = 0; // For 
	
	if(err1 == -1 || err2 == -1 || err3 == -1) {
		return -1;
	}
	else {
		return 0;
	}
	
}


int del_tempfile(int file_no) {
	
	char cmd1[200] = "";
	char cmd2[200] = "";
	char cmd3[200] = "";

	int err1 = 0;
	int err2 = 0;
	int err3 = 0;
	
	sprintf(cmd1, "%sfiles/temp_%d.meta", ini.homedir, file_no);
	sprintf(cmd2, "%sfiles/temp_%d.data", ini.homedir, file_no);
	sprintf(cmd3, "%sfiles/temp_%d.pass", ini.homedir, file_no);

	err1 = remove(cmd1);
	err2 = remove(cmd2);
	err3 = remove(cmd3);
	err3 = 0; // For 
	
	if(err1 == -1 || err2 == -1 || err3 == -1) {
		return -1;
	}
	else {
		return 0;
	}
	
}

int copy_file(const char *fnsource, const char *fntarget)
{
	int rc = 0; // 0 means good
	FILE *fpin = fopen(fnsource, "rb");
	if(fpin != NULL) {
		FILE *fpout = fopen(fntarget, "wb");
		if(fpout != NULL) {
			int ch = 0;
			while((ch = getc(fpin)) != EOF) {
				putc(ch, fpout);
			}
		
			if(ferror(fpin)) {
				rc = -1;
			}
			if(ferror(fpout)) {
				rc = -1;
			}
			if(fclose(fpout)) {
				rc = -1;
			}
			if(rc == -1) {
				remove(fntarget);
			}
		}
		else {
			rc = -1;
		}
		
		if(fclose(fpin) != 0) {
			rc = -1;
		}
	}
	else {
		rc = -1;
	}
	return rc;
}

void printarrayinhex(const char *data, int len) 
{
	for(int i = 0; i< len; i++) {
		printf("%02x", (unsigned char)data[i]);
	}
	printf("\n");
	return;
}


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


//Needs testing on Nunki ...
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

/*
void sha1byte2hex(char *byte20, string& hex40) 
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
*/

int read_temp_meta_file(int file_no, string& filename, string& sha1, string& nonce) {
	int err = 0;
	char key[200] = "";
	char val[1000] = "";
	char file_name[200] = "";
	ssize_t read;
	
	sprintf(file_name, "%sfiles/temp_%d.meta", ini.homedir, file_no);
	if(DEBUG) printf("read-meta-file(): %sfiles/temp_%d.meta \n", ini.homedir, file_no);
	
	ifstream file;
  	file.open(file_name, ios::in );
  	
  	file.getline(val, 200);
	if(strcmp(val, "[metadata]\r")) {
		err = -1;
		return err;
	}
	if(DEBUG) cout << string(val) << endl;
  	
	while(!file.eof())
	{
	  	file.getline(key, 200, '=');
		read = file.gcount();
	  	file.getline(val, 1000);
		read = file.gcount();
		val[read-2] = '\0';
		
		// Need to lowercase the key?
		
		if (strcmp(key, "FileName") == 0) {
			filename = string(val);
		}
		else if (strcmp(key, "SHA1") == 0) {
			sha1 = string(val);
		}
		else if (strcmp(key, "Nonce") == 0) {
			nonce = string(val);
		}
	}
	file.close();
	
	return err;
}

int read_meta_file(int file_no, string& filename, string& sha1, string& nonce) {
	int err = 0;
	char key[200] = "";
	char val[1000] = "";
	char file_name[200] = "";
	ssize_t read;
	
	sprintf(file_name, "%sfiles/%d.meta", ini.homedir, file_no);
	if(DEBUG) printf("read-meta-file(): %sfiles/%d.meta \n", ini.homedir, file_no);
	
	ifstream file;
  	file.open(file_name, ios::in );
  	
  	file.getline(val, 200);
	if(strcmp(val, "[metadata]\r")) {
		err = -1;
		return err;
	}
	if(DEBUG) cout << string(val) << endl;
  	
	while(!file.eof())
	{
	  	file.getline(key, 200, '=');
		read = file.gcount();
	  	file.getline(val, 1000);
		read = file.gcount();
		val[read-2] = '\0';
		
		// Need to lowercase the key?
		
		if (strcmp(key, "FileName") == 0) {
			filename = string(val);
		}
		else if (strcmp(key, "SHA1") == 0) {
			sha1 = string(val);
		}
		else if (strcmp(key, "Nonce") == 0) {
			nonce = string(val);
		}
	}
	file.close();
	
	return err;
}
/*
int main() {
	unsigned char sha1_sum[20] = "";	
	FILE* fp = fopen("sv_node", "r");
	sha1_file(fp, sha1_sum);
	print_sha1(sha1_sum);
	fclose(fp);

	sha1_file("sv_node", sha1_sum);
	print_sha1(sha1_sum);
	copy_file("out.nam", "test_out.nam");
	return 0;
}
*/

