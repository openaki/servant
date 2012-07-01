#include <fstream>
#include <functional>
#include "lru.h"

struct Num_match: public std::binary_function< Pair_num_size, int, bool > {
	bool operator () ( const Pair_num_size item, const int &number ) const {
		return item.first == number;
	}
};

Lru::Lru() {
	m_quota = 0;
	m_size = 0;
}

void Lru::init_home(string homedir) {
	m_homedir = homedir;	
	return;
}

void Lru::init_quota(unsigned long q) {
	m_quota = q * 1024; // q is in KB
}

unsigned long Lru::get_quota() {
	return m_quota;
}

unsigned long Lru::get_size() {
	return m_size;
}


// function push
//   Push a new item or update an existing one's position.
//   In both cases, it will be the most recently used one.
// return 
//   true if item pushed or updated.
//   false if disk quota exceeds.
bool Lru::push(int num, unsigned long size, vector<int>& victims) {

	if (size > m_quota) {
		return false;
	}

	list<Pair_num_size>::iterator it = find_if(m_list.begin(), m_list.end(), std::bind2nd(Num_match(), size));
	Pair_num_size new_item(num, size);
	
	if(it == m_list.end()) { // Not found
		while(m_size + size > m_quota) {
			// Evacuate occupants.
			Pair_num_size victim = m_list.front();
			m_list.pop_front();

			m_size -= victim.second;
			victims.push_back(victim.first);
		}

		// Push it.
		m_list.push_back(new_item);
		m_size += size;
		return true;		
	}
	else { // found
		// Just update order ONLY.
		list<Pair_num_size>::iterator it1 = it;
		it1++;
		m_list.splice(it, m_list, it1, m_list.end());
		return true;
	}
}


// function pop
//   Pop the least recently used (LRU) item.
// return 
//   true if success
// note
//   item will be updated only when popping succeeded
//
bool Lru::pop(int& num, unsigned long& size) {
	if(m_list.empty()) {
		return false;
	}
	else {
		Pair_num_size pair = m_list.front();
		num = pair.first;
		size = pair.second;
		m_size -= pair.second;
		m_list.pop_front();	
		return true;
	}
}

bool Lru::erase(int num) {
	if(m_list.empty()) {
		return false;
	}
	else {
		list<Pair_num_size>::iterator itr;	
		for(itr = m_list.begin(); itr != m_list.end(); itr++) {
			if (itr->first == num) {
				m_size -= itr->second;
				m_list.erase(itr);
				return true;
			}			
		}
	}
	return false;
}

void Lru::print() {
	list<Pair_num_size>::iterator it;

	for(it = m_list.begin(); it != m_list.end(); it++) {
		cout << it->first << ":" << it->second << "  ";
	}
	cout << "size: " << get_size();
	cout << endl;
}

int Lru::write_lru_file() {
	int err = 0;
	FILE* fp = NULL;

	char file_name[200] = "";
	sprintf(file_name, "%slru.txt", m_homedir.c_str());
	fp = fopen(file_name, "w");
	if(!fp) {
		return -1;
	}
	
	fprintf(fp, "%d", m_list.size());		

	list<Pair_num_size>::iterator itr;
	for(itr = m_list.begin(); itr != m_list.end(); itr++) {		
		fprintf(fp, " %d %lu", itr->first, itr->second);
	}	
	fprintf(fp, "\n");
		
	fclose(fp);	
	return err;
}

int Lru::read_lru_file() {
	int err = 0;
	char file_name[200] = "";
	
	sprintf(file_name, "%slru.txt", m_homedir.c_str());

	ifstream file (file_name);
	if (file.is_open()) {
		while(!file.eof())
		{
			int num = 0;
			int file_no = 0;
			unsigned long file_size = 0;
			vector<int> victims;

		  	file >> num;
			for(int i = 0; i < num; i++) {	  	
				file >> file_no;
				file >> file_size;
				push(file_no, file_size, victims);				
			}
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
	Lru lru;
	lru.init_quota(10);
	
	int num = -99;
	unsigned long size = 0;
	
	lru.push(1, 1);
	lru.print();

	lru.push(2, 2);
	lru.print();

	lru.push(3, 3);
	lru.print();

	lru.push(4, 4);
	lru.print();

	lru.push(1, 1);
	lru.print();

	lru.push(5, 5);
	lru.print();

	lru.push(3, 3);
	lru.print();

	if(lru.pop(num, size)) {
		cout << "popping LRU: " << num << endl;
		lru.print();	
	}
	
	lru.push(5, 5);	
	lru.print();

	if(lru.pop(num, size)) {
		cout << "popping LRU: " << num << endl;
		lru.print();	
	}

	lru.push(5, 5);	
	lru.print();

	if(lru.pop(num, size)) {
		cout << "popping LRU: " << num << endl;
		lru.print();	
	}

	if(lru.pop(num, size)) {
		cout << "popping LRU: " << num << endl;
		lru.print();	
	}

	if(lru.pop(num, size)) {
		cout << "popping LRU: " << num << endl;
		lru.print();	
	}	
	
	return 0;
}
*/
