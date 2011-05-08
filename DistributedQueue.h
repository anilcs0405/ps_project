#include <iostream>
#include <fstream>
#include <map>
#include <list>
#include <pthread.h>

#include "structures.h"
#include "WorkQueue.h"

#ifndef DISTRIBUTEDQUEUE_H
#define DISTRIBUTEDQUEUE_H

using namespace std;

class DistributedQueue
{
	private:
		int no_master_of_masters;
		int no_masters;
		int no_threads;
		int no_procs;
		int my_id;
		WorkQueue *queue;
		int my_master;
		int *load;
		int *local_load;
		int *myslaves;
		int no_myslaves;
		static void* StaticThreadProc(void *args)
                {
                        return reinterpret_cast<DistributedQueue*>(((m_args*)args)->obj)->ThreadFunction(((m_args*)args)->tid);
                }
		static void* StaticExternalLoadProc(void *args)
                {
                        return reinterpret_cast<DistributedQueue*>(((m_args*)args)->obj)->AcceptExternalLoad(((m_args*)args)->tid);
                }
		static void* StaticCommProc(void *args)
                {
                        return reinterpret_cast<DistributedQueue*>(((m_args*)args)->obj)->CommunicationFunction(((m_args*)args)->tid);
                }
		static void* StaticManageProc(void *args)
                {
                        return reinterpret_cast<DistributedQueue*>(((m_args*)args)->obj)->ManageProcessesFunction(((m_args*)args)->tid);
                }
		
	public:
		DistributedQueue(int num_threads);
		~DistributedQueue(void);
		void ProcessFunction(int *argc, char ***argv);
		void* ThreadFunction(void* threadid);
		void* AcceptExternalLoad(void* threadid);
		void* CommunicationFunction(void* thread_id);
		void* ManageProcessesFunction(void* thread_id);
		bool ismasterofmaster(int pid);
};

#endif
