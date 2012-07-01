#include "bitvector.h"


bitvector::bitvector()
{
	int i;
	for(i = 0; i< 128; i++)
		byte[i] = 0;
}

int bitvector::set(int pos)
{
	int index = 127 - pos/8;
	int offset = 7 - pos%8;
	char old;
	char k = 0x01;
	if (pos > 1023 || pos < 0)
		return -1;
	k = k << (7-offset);
	old = byte[index];
	byte[index] = (old | k);
	return 1;
}

int bitvector::set(string kwrd) {
	// Taken from Akshat's code
	unsigned char str_buf[20];
	unsigned char md5_out[16];
	
	transform(kwrd.begin(), kwrd.end(), kwrd.begin(), (int(*)(int)) std::tolower);
	char *p = (char *)calloc(kwrd.length(), sizeof(char));
	strcpy(p, kwrd.c_str());
	//fprintf(fp,"%s ", p);	

	unsigned int value;
	SHA1((unsigned char *)p, strlen(p), str_buf);
	value = str_buf[18];
	value = value & 1;
	value = value << 8;
	value = value | str_buf[19];
	set(512 + value);

	MD5((unsigned char *)p, strlen(p), md5_out);
	value = md5_out[14];
	value = value & 1;
	value = value << 8;
	value = value | md5_out[15];
	set(value);

	return 0;
} 

// To set bitvector directly from the metafile.
// CAUTION: Overwrite to the previous data!
//
int bitvector::set(char* hex) 
{
	char* p = hex;
	unsigned char hex1;
	unsigned char hex2;
	unsigned char _byte;
		
	for(int i = 0; i < 128; i++) {
		hex1 = *p++;
		hex2 = *p++;

		if (isupper(hex1)) {
			hex1 = 10 + hex1 - 'A';
		}
		else if(islower(hex1)) {
			hex1 = 10 + hex1 - 'a';
		}
		else {
			hex1 -= '0';
		}

		if (isupper(hex2)) {
			hex2 = 10 + hex2 - 'A';
		}
		else if(islower(hex2)) {
			hex2 = 10 + hex2 - 'a';
		}
		else {
			hex2 -= '0';
		}

		hex1 = hex1 << 4;
		_byte = (unsigned char) hex1 | (unsigned char) hex2;

		byte[i] = _byte;
	}
		
	return 1;
}


int bitvector::reset(int pos)
{
	int index = 127 - pos/8;
    int offset = 7 - pos%8;
    unsigned char k = 0x01;
	unsigned char l = 0xff;
	unsigned char old = byte[index];
	if(pos > 1023 || pos < 0)
		return -1;
	k = k << (7-offset);
	l = l ^ k;

    byte[index] = (old & l);
	return 1;
}

bitvector bitvector::bool_and(bitvector b)
{
	bitvector c;
	for(int i = 0; i< 128; i++)
		c.byte[i] = byte[i] & b.byte[i];
	return c;

}

bitvector bitvector::bool_or(bitvector b)
{
	bitvector c;
	for(int i = 0; i< 128; i++)
		c.byte[i] = byte[i] | b.byte[i];
	return c;

}

bool bitvector::is_equal(bitvector b)
{
	for(int i = 0; i< 128; i++) {
		if(byte[i] != b.byte[i])
			return 0;
	}
	return 1;
}

bool bitvector::is_hit(bitvector& b)
{
	bitvector match = bool_and(b);
	
	if(match.is_equal(b)) {
		return true;
	}
	else {
		return false;
	}
}

bool bitvector::operator<(const bitvector &right) const // To be used as a key in STL map.
{
	//return strcmp((char*)byte, (char*)right.byte);
	int i=0;
	while(byte[i] == right.byte[i])
		i++;
	if(i<128) {
		if(byte[i] < right.byte[i])
			return 1;
		else
			return 0;
	}
	return 0;
}

bool bitvector::operator==(bitvector &right) 
{
	return is_hit(right);
}

//bool bitvector::operator!=(bitvector &right) 
//{
//	return not is_hit(right);
//}

void bitvector::print_hex()
{
	int i;
	for(i = 0; i< 128; i++)
		printf("%02x",  byte[i]);

	printf("\n");
}

void bitvector::print_hex(FILE *fp)
{
    int i;
    for(i = 0; i< 128; i++)
        fprintf(fp, "%02x",  byte[i]);
}

void bitvector::read_hex(FILE *fp)
{
    char hex[257] = "";
    
    fscanf(fp, "%s", hex);
	set(hex);
}

void bitvector::print()
{
	int i;
	unsigned char val, k;
	int j;
	
	
	for(i = 0; i<128; i++) {
		k = 0x80;
		for(j = 0; j<8; j++) {
			val = byte[i];
			val = (val & k);
			
			val = val >> (7 - j);
			if(val)
				printf("1");
			else
				printf("0");
			k = k >> 1;
		}
	}
	printf("\n");
}

/*
// Test case for set(char*)
// Bob 11-19-2008
int main() {
	bitvector vec0;
	bitvector vec1;
	bitvector vec2;
	bitvector vec3;
	

	char hex0[1000]= 
"112233445566778899aabbccddeeffAABBCCDDEEFF0102030405060708090A0B0C0D0E0F102030485060708090A0B0C0D\
0E0F0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\
0000000000000000000000000000000000000000000000000000000000000";

	char hex1[1000]= 
"0123456789abcdef000000000000000000000000000000000000000000000000000000000000000000000000000000000\
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\
0000000000000000000000000000000000000000000000000000000000000";

	char hex2[1000]= 
"00000400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\
0000000000000000000000000000000000000000000000000000000000000";

	char hex3[1000]= 
"1240000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\
0000000000000000000000000000000000000000000000000000000000000";

	vec0.set(hex0);
	vec0.print();
	vec0.print_hex();
	
	vec1.set(hex1);
	vec2.set(hex2);
	vec3.set(hex3);
	
	if(vec1 == vec2) {
		printf("vec1 hits vec2.\n");
	}
	
	if(vec1 == vec3) {
		printf("vec1 hits vec3.\n");	
	}
	return 0;
}
*/

/*
int main() 
{
	bitvector obj, obj1;
	int i=0;

//	for(i = 0; i< 1024; i=i+1)
		obj1.set(i);
/ *	
	for(i = 0; i<1024; i = i+3)
		obj.reset(i);
	
	obj.print();
	printf("\n\n\n");
    for(i = 0; i< 128; i++)
		printf("%02x", obj.byte[i]);
	
	
	unsigned char k = 0xf0 | 0x0f;
	printf("\n%02x\n", k);
// * /
* /
	string s = "categories";
    unsigned char str_buf[20];
	char *p;
	p = (char *)calloc(s.length(), sizeof(char));
	strcpy(p, s.c_str());
	SHA1((unsigned char *)p, strlen(p), str_buf);
	unsigned int value;
	value = str_buf[18];
	value = value & 1;
	value = value << 8;
	value = value | str_buf[19];
	obj.set(512 + value);
	unsigned char md5_out[16];
	MD5((unsigned char *)p, strlen(p), md5_out);
    value = md5_out[14];
    value = value & 1;
    value = value << 8;
    value = value | md5_out[15];
	obj.set(value);	
	
	bitvector result = obj1.bool_and(obj);
	result.print_hex();
	cout<<endl<<endl<<result.is_equal(obj)<<endl<<endl;
	obj.print_hex();
	return 1;
}

*/		

