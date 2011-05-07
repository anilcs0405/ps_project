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
#include "WorkQueue.h"
#include "DistributedQueue.h"

#define NUM_THREADS 8

//MPI tags

#define LOAD_UPDATE 0
#define INTITIATE_DATA_TRANSFER 1
#define ACCEPT_DATA_TRANSFER 2

using namespace std;

DistributedQueue::DistributedQueue(int num_master_of_masters, int num_masters, int num_threads){
		no_master_of_masters = num_master_of_masters;
		no_masters = num_masters;
		no_threads = num_threads;
		queue = new WorkQueue();
		load = new int[num_masters];
}

DistributedQueue::~DistributedQueue(){

}

// Helper functions

bool DistributedQueue::ismasterofmaster(int pid){
	if (pid < no_master_of_masters)
		return true;
	else
		return false;
}

// Each machine runs ProcessFunction

void DistributedQueue::ProcessFunction(void *pid)
{
		// MPI Init

		// Are you master of master? then initialize stuff

		// Else stay quiet

		// MPI Barrier;
		
		my_id = 0;

		pthread_t threads[NUM_THREADS];

                for(int t=0; t < NUM_THREADS; t++){
                        m_args *args = new m_args;
                        args->obj = (void *) this;
                        args->tid = (void *) t;
			if(ismasterofmaster(my_id)){
							
				if(t == 0){
					// manage nodes	
					pthread_create(&threads[t], NULL, &DistributedQueue::StaticThreadProc , args);					
				}else if(t == 1){
					// communication
					pthread_create(&threads[t], NULL, &DistributedQueue::StaticCommProc , args);

				}
			}else{
				if(t == 0){
					//accept external load
					pthread_create(&threads[t], NULL, &DistributedQueue::StaticExternalLoadProc , args);						
				}else if(t == 1){
					// communication
					pthread_create(&threads[t], NULL, &DistributedQueue::StaticCommProc , args);
				}else{
					pthread_create(&threads[t], NULL, &DistributedQueue::StaticThreadProc , args);
				}
			}
                        
                }

                for(int t=0; t < NUM_THREADS; t++) {
                        pthread_join(threads[t],NULL);
                }
		
		// This process runs indefinitely till the program is cancelled

		while(1){
			break;
		}
}

/* Common functions for Masters of Masters and the Masters*/

//Communication

void* DistributedQueue::CommunicationFunction(void* thread_id){

		// This thread runs indefinitely till the program is cancelled

		MPI_Request request;

		if(ismasterofmaster(my_id)){
			while(1){
				for(int i = no_master_of_masters ; i < no_masters; i++){
					//MPI_Irecv();					
				}
				for(int i = no_master_of_masters ; i < no_masters; i++){
					//MPI_Irecv();					
				}				
				break;		
			}				
		}
		
		return (void *)1;

}

// Communication helpers

/* Only for Masters */

void* DistributedQueue::ThreadFunction(void* thread_id){

		// This part is Shared Queue;

		/******* Application goes here ********/

		// queue.enqueue and apply bayesian filter

		/******* Application goes here ********/

		cout << "THREAD_ID::" << ((int) thread_id) << endl;        
                return (void *)1;

}

void* DistributedQueue::AcceptExternalLoad(void* thread_id){

		// Might be a socket/communication with another process

      		// This thread runs indefinitely till the program is cancelled

		while(1){
			break;
		}

                return (void *)1;

}

/* Only for Master of Masters */

void* DistributedQueue::ManageProcessesFunction(void* thread_id){

		// Only Master of Master nodes run this thread to manage other nodes

		// This thread runs indefinitely till the program is cancelled

		while(1){
			break;
		}
		return (void *)1;

}

