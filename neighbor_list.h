#ifndef NEIGHBOR_LIST_H__ 
#define NEIGHBOR_LIST_H__

#include <list>
#include <string>
#include "msg.h"

using namespace std;

struct Node_info {
	string host_name;
	uint16_t port;
	uint32_t distance;

	Node_info(string _host_name = "", uint16_t _port = 0, uint32_t _distance = 0);
};

class Neighbor_list {
	void sort_in_distance();

public:
	list<Node_info> nodes;

	Neighbor_list();
	int add_node(Join_res* msg);
	int add_node(Node_info node);
	int prune(int num_node);
	int read_file();
	int write_file();
};

bool compare_distance(Node_info& node1, Node_info& node2);


#endif
