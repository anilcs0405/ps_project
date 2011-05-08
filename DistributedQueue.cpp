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

#define MEGA_MASTER 0

using namespace std;

DistributedQueue::DistributedQueue(int num_threads){
		no_threads = num_threads;		
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

void DistributedQueue::ProcessFunction(int *argc, char ***argv)
{
		// MPI Init

		MPI_Init(argc,argv);
		MPI_Comm_size(MPI_COMM_WORLD, &no_procs);
		MPI_Comm_rank(MPI_COMM_WORLD, &my_id);

		no_master_of_masters = no_procs/4;
		no_masters = no_procs - no_master_of_masters;

		// Are you master of master? then initialize stuff
			
		// Else stay quiet

		// MPI Barrier;
		

		queue = new WorkQueue();

		my_id = 0;

		pthread_t threads[NUM_THREADS];

		if(!ismasterofmaster(my_id)){
			for(int t=0; t < NUM_THREADS; t++){
		                m_args *args = new m_args;
		                args->obj = (void *) this;
		                args->tid = (void *) t;			
				if(t == 0){
					//accept external load
					pthread_create(&threads[t], NULL, &DistributedQueue::StaticExternalLoadProc , args);						
				}else{
					pthread_create(&threads[t], NULL, &DistributedQueue::StaticThreadProc , args);
				}
		                
		        }
			my_master = my_id % no_master_of_masters;
		}else{
			// init load
			load = new int[no_masters];

			// set initial load to 0
			memset(load, 0, no_masters);

			int k = 0;		
			for(int i = no_master_of_masters; i < no_procs; i++){
				if(i % no_master_of_masters == my_id){
					k++;
				}
			}
			myslaves = new int[k];
			no_myslaves = k;
			k = 0;
			for(int i = no_master_of_masters; i < no_procs; i++){				
				if(i % no_master_of_masters == my_id){
					myslaves[k++] = i;
				}
			}					
		}

		// MPI Barrier

		// This process runs indefinitely till the program is cancelled

		while(1){
			// loads are updated by all the masters to masters of masters
			if(ismasterofmaster(my_id)){
				MPI_Request requests[no_myslaves];
				MPI_Status status[no_myslaves];
				for(int i = 0 ; i < no_myslaves; i++){
					MPI_Irecv(&load[myslaves[i]], 1, MPI_INT, myslaves[i], LOAD_UPDATE, MPI_COMM_WORLD, &requests[i]);
				}
				MPI_Waitall(no_myslaves, requests, status);
			}else{
				MPI_Request request;
				int queue_size = queue->get_size();
				MPI_Isend(&queue_size, 1, MPI_INT, my_master, LOAD_UPDATE, MPI_COMM_WORLD, &request);
			}

			MPI_Barrier(MPI_COMM_WORLD);

			// syncronize load information amongst masters of masters
			if(!ismasterofmaster(my_id)){
				for(int i = 0; i < no_master_of_masters; i++){
					MPI_Reduce(&local_load[0], &load[0], no_masters, MPI_INT, MPI_SUM, MEGA_MASTER, MPI_COMM_WORLD);
				}
				MPI_Bcast(&load[0], no_masters, MPI_INT, MEGA_MASTER, MPI_COMM_WORLD);
			}

			MPI_Barrier(MPI_COMM_WORLD);

			// Analyze loads
			
			// initiate queue item transfers

			// wait for transfers to complete before starting the loop again

			break;
		}

		// once you break join all the threads on each of the master nodes

		if(!ismasterofmaster(my_id)){
		        for(int t=0; t < NUM_THREADS; t++) {
		                pthread_join(threads[t],NULL);
		        }
		}
		
		// finalize 

		MPI_Finalize();

		
}

/* Common functions for Masters of Masters and the Masters*/

//Communication

void* DistributedQueue::CommunicationFunction(void* thread_id){

		// This thread runs indefinitely till the program is cancelled
		int mysize;
		MPI_Request request;

		if(ismasterofmaster(my_id)){
			while(1){
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

