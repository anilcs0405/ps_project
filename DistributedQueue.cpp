#include <mpi.h>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <map>
#include <list>
#include <ctime>
#include <string.h>
#include <math.h>
#include <fstream>

#include "FileIO.h"
#include "Queue.h"
#include "DistributedQueue.h"

#define NUM_THREADS 8

using namespace std;

DistributedQueue::DistributedQueue(int num_master_of_masters, int num_masters, int num_threads){
		no_master_of_masters = num_master_of_masters;
		no_masters = num_masters;
		no_threads = num_threads;
}

DistributedQueue::~DistributedQueue(){

}

void DistributedQueue::ProcessFunction(void *pid)
{

		// Are you master of master? then initialize stuff

		// Else stay quiet

		// MPI Barrier;

		// create Queue

		// local threads

		pthread_t threads[NUM_THREADS];
                for(int t=0; t < NUM_THREADS; t++){
                        m_args *args = new m_args;
                        args->obj = (void *) this;
                        args->tid = (void *) t;
                        pthread_create(&threads[t], NULL, &DistributedQueue::StaticThreadProc , args);
                }

                for(int t=0; t < NUM_THREADS; t++) {
                        pthread_join(threads[t],NULL);
                }

}

void* DistributedQueue::ThreadFunction(void* thread_id){                
                return (void *)1;
}

