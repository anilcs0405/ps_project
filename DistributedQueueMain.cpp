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
	int num_master_of_masters = 2;
	int num_masters = 8;
	int num_procs = 10;
	int num_threads = 8;

	// decide on number of Master of Masters, Masters and initialize the master
	// the above logic goes here;
	
	if(argc > 1 && argv[1] != NULL){
		num_threads = atoi(argv[1]);
	}
	
	DistributedQueue *distributed_queue = new DistributedQueue(num_threads);
	distributed_queue -> ProcessFunction(&argc, &argv);
    
}

