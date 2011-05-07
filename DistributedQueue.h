#include <iostream>
#include <fstream>
#include <map>
#include <list>
#include <pthread.h>

#ifndef DISTRIBUTEDQUEUE_H
#define DISTRIBUTEDQUEUE_H

using namespace std;

class DistributedQueue
{
	private:
		
	public:
		DistributedQueue(void);
		~DistributedQueue(void);
		void ProcessFunction(void *pid);
};

#endif
