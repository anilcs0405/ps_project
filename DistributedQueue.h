#include <iostream>
#include <fstream>
#include <map>
#include <list>
#include <pthread.h>

#include "structures.h"

#ifndef DISTRIBUTEDQUEUE_H
#define DISTRIBUTEDQUEUE_H

using namespace std;

class DistributedQueue
{
	private:
		int no_master_of_masters;
		int no_masters;
		int no_threads;
		static void* StaticThreadProc(void *args)
                {
                        return reinterpret_cast<DistributedQueue*>(((m_args*)args)->obj)->ThreadFunction(((m_args*)args)->tid);
                }
		
	public:
		DistributedQueue(int num_master_of_masters, int num_masters, int num_threads);
		~DistributedQueue(void);
		void ProcessFunction(void *pid);
		void* ThreadFunction(void* threadid);
};

#endif
