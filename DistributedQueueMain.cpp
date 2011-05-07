#include <iostream>
#include <sstream>
#include <cstdlib>
#include <map>
#include <list>
#include <ctime>
#include <string.h>
#include <pthread.h>

#include "FileIO.h"
#include "SharedQueue.h"
#include "DistributedQueue.h"
#include "Master.h"

#define NUM_THREADS 16

using namespace std;

Master *master;

void *TFunction(void *threadid)
{
	master -> ProcessFunction(threadid);
}


int main (int argc, char *argv[])
{
	    
}

