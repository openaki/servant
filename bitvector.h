#ifndef BIT_VECTOR_H__
#define BIT_VECTOR_H__

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <ctype.h>

using namespace std;

//A bitvector class of the size 1024 bits
//Considering a 128 byte char to represent the vector
//The MSB is the byte 127 and the LSB is the byte 0
//The index for the bit vector is 0 to 1023
class bitvector
{
public:
	unsigned char byte[128];


	//Initializes all the bits to 0
	bitvector();

	//set functions
	int reset(int pos);
	int set(int pos); 		// Sets the particular bit at pos from the MSB (Cumulative Write)
	int set(string kwrd);	// Sets bits corresponding to kwrd. (Cumulative Write)
							// CAUTION: set(s.c_str()) calls this function.
	int set(char* hex); 	// Sets from 256-byte char array in hex form. (Overwrite)
							// Use: set((char*) s.c_str()) to call this function.

	// Accumulate functions
	bitvector bool_and(bitvector b);
	bitvector bool_or(bitvector b);

	// Compare functions
	bool is_equal(bitvector b);				// true if exact match.
	bool is_hit(bitvector& b);  // true if all bits of b match. (for AND based keywords search)
	
	bool operator<(const bitvector &) const; 		// To be used as a key in STL map.
	bool operator==(bitvector &right);	// bloom filtering!
//	bool operator!=(bitvector &right);
	// Print functions
	void print();
	void print_hex();
	void print_hex(FILE *fp);
	void read_hex(FILE *fp);
};

#endif
