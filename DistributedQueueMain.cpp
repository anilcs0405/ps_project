#include <iostream>
#include <sstream>
#include <cstdlib>
#include <map>
#include <list>
#include <ctime>
#include <string.h>
#include <pthread.h>

#include "FileIO.h"
#include "DistributedQueue.h"

using namespace std;

int main (int argc, char *argv[])
{
	int num_master_of_masters;
	int num_masters;
	int num_procs;
	int num_threads;

	// decide on number of Master of Masters, Masters and initialize the master
	// the above logic goes here;
	
	if(argc > 1 && argv[1] != NULL){
		num_procs = atoi(argv[1]);
	}
	
	if(argc > 2 && argv[2] != NULL){
		num_procs = atoi(argv[2]);
	}	
	
	DistributedQueue *distributed_queue = new DistributedQueue(num_master_of_masters, num_masters, num_threads);
	distributed_queue -> ProcessFunction(&argc, argv);
    
}

