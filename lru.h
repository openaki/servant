#ifndef LRU_H__
#define LRU_H__

#include <vector>
#include <list>
#include <algorithm>
#include <iostream>

using namespace std;

typedef pair<int, unsigned long> Pair_num_size;


class Lru 
{
protected:
	string m_homedir;

	unsigned long m_quota;
	unsigned long m_size;
	
	list<Pair_num_size> m_list;

public:
	Lru();
	void init_home(string homedir);
	void init_quota(unsigned long q); 		// capacity initializer
	
	unsigned long get_quota();
	unsigned long get_size();
	
	bool push(int num, unsigned long size, vector<int>& victims);
	//bool push(int num, unsigned long size);
	bool pop(int& num, unsigned long& size);
	bool erase(int num);
	void print();
	
	int write_lru_file();
	int read_lru_file();
};

#endif

