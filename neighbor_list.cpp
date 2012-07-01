#include <iostream>
#include <fstream>
#include "neighbor_list.h"

#include "startup.h"
extern Startup ini;

bool compare_distance(Node_info& node1, Node_info& node2) 
{
	if(node1.distance < node2.distance) {
		return true;
	}
	else {
		return false;
	}
}


// Constructor for Node_info
//
Node_info::Node_info(string _host_name, uint16_t _port, uint32_t _distance) 
{  
	host_name = _host_name;
	port = _port;
	distance = _distance;
}


/////////////////////////////////////////
// Private members of class Node_list
/////////////////////////////////////////


// sort_in_distance
//	 Sort nodes in increasing order of distance
//
void Neighbor_list::sort_in_distance() 
{
	nodes.sort(compare_distance);
}


/////////////////////////////////////////
// Public members of class Node_list
/////////////////////////////////////////

// Default constructor
Neighbor_list::Neighbor_list() {
}


// add node
//	 Extract node info from join respose message
//	 and add it to the node list.
//
int Neighbor_list::add_node(Join_res* msg) 
{
	Node_info node(string(msg->host_name), msg->host_port, msg->distance);
	nodes.push_back(node);

	return 0;
}

// add_node
//	 add a note to the node list
//
int Neighbor_list::add_node(Node_info node) 
{
	nodes.push_back(node);

	return 0;
}


// prune
//	 delete all nodes exceeding num
// argument
//	 num: number of nodes to remain.
// return
//	 Number of nodes remained
//
int Neighbor_list::prune(int num) 
{

	int size = nodes.size();
	int numSaved = 0;
	Node_info node;

	// Sort nodes in increasing distance
	sort_in_distance();

	for(int i = 0; i < size; i++) {
		// save the closest num nodes by pushing them back
		if(i < num) {
			node = nodes.front();
			nodes.push_back(node);
			nodes.pop_front();
			numSaved++;
		}
		// delete the others.
		else {
			nodes.pop_front();
		}
	}

	return numSaved;
}


// read_file
//	 Read neighbor information from the init_neighbor_list file
// return
//	 Number of nodes read
//
/*
int Neighbor_list::read_file() 
{
	int numRead = 0;
	Node_info node;
	char name[80];
	long port = 0;
	list<Node_info>::iterator itr;

	FILE* fp = fopen("init_neighbor_list", "r");

	if(!fp) {
		printf("Error reading init_neighbor_list.\n");
		exit(1);
	}

	while (fscanf(fp, "%s:%ld\n", name, &port) == 2) {
		Node_info(name, port );
		add_node(node);
		numRead++;
	}

	return numRead;
}
*/
int Neighbor_list::read_file() 
{
	int num_read = 0;
	Node_info node;
	ifstream file;
	char key[200] = ""; // host_name
	char val[200] = ""; // port
	char file_name[200] = "";
	ssize_t byte_read = 0;
	
	sprintf(file_name, "%s%s", ini.homedir, "init_neighbor_list");
	//sprintf(file_name, "%s%s", "./", "init_neighbor_list");
  	file.open(file_name, ios::in );

	while(!file.eof())
	{
		byte_read = 0;
		
	  	file.getline(key, 200, ':');
		byte_read = file.gcount();

	  	file.getline(val, 1000);
		byte_read = file.gcount();
		//val[byte_read-1] = '\0';
		long port = strtol(val, NULL, 10);
		
		if(file.eof())
			break;

		Node_info node(key, port);
		add_node(node);
		num_read++;		
	}

	file.close();	
	return num_read;
}


// write_file
//	 Write neighbor information on the init_neighbor_list file
// return
//	 Number of nodes written on the file
//
int Neighbor_list::write_file() 
{
	int numWritten = 0;
	Node_info node;
	list<Node_info>::iterator itr;

	char file_name[200] = "";	
	sprintf(file_name, "%s%s", ini.homedir, "init_neighbor_list");

	FILE* fp = fopen(file_name, "w");
	
	if(!fp) {
		printf("Error opening init_neighbor_list.\n");
		exit(1);
	}

	for( itr = nodes.begin(); itr != nodes.end(); itr++) {
		fprintf(fp, "%s:%d\n", itr->host_name.c_str(), itr->port);
		numWritten++;
	}

	fclose(fp);

	return numWritten;
}

