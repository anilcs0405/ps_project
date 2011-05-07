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

	DistributedQueue *distributed_queue = new DistributedQueue(num_master_of_masters, num_masters, num_threads);
	distributed_queue -> ProcessFunction((void *)0);
    
}

