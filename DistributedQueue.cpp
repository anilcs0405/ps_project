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
#define DIFF_UPDATE 1
#define SIZE_UPDATE 2
#define LOAD_TRANSFER 3

#define MEGA_MASTER 0
#define EPSILON 5


void gen_random(char *s, const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
}

using namespace std;

DistributedQueue::DistributedQueue(int num_threads){
		no_threads = num_threads;
		pthread_mutex_init(&entire_queue_mutex, NULL);		
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

char* DistributedQueue::get_filenames_buffer(int diff, int *size){
	int size_here = queue->get_size();
	work_item* item;
	int start = size_here - diff;
	item = queue->get_at(start);
	string buff;
	char *final_buff;
	while(item != NULL){
		buff.append(item->filename);
		buff.append("|");
		item = item -> next;
	}
	final_buff = new char[buff.size() + 1];
	strcpy(final_buff, buff.c_str());
	*(size) = buff.size() + 1;
	return final_buff;
}

void DistributedQueue::load_dummy_data(void){
	srand ( (my_id + 1) * time(NULL) );
	int load = rand() % 400;
	for(int i = 0; i < load; i++){
		work_item *item = new work_item;
		item->filename = new char[30];
		gen_random(item->filename, 15);
		item->load = (rand() % 2) + 1;
		item->isspam = false;
		item->next = NULL;
		queue->enqueue(item);
	}
}


char* DistributedQueue::add_filenames(char *buffer){
	char *temp;
	temp = strtok(buffer, "|");
	while(temp != NULL){
		work_item *p = new work_item;
		p->load = 1;
		p->filename = new char[strlen(temp) + 1];
		strcpy(p->filename, temp);
		p->isspam = false;
		p->next = NULL;
		queue->enqueue(p);
		temp = strtok(NULL, "|");		
	}
}

// Each machine runs ProcessFunction

void DistributedQueue::ProcessFunction(int *argc, char ***argv)
{
		// MPI Init

		MPI_Init(argc,argv);
		MPI_Comm_size(MPI_COMM_WORLD, &no_procs);
		MPI_Comm_rank(MPI_COMM_WORLD, &my_id);

		MPI_Group mmasters_group, masters_group, temp1, temp2; 
		MPI_Comm mmasters_comm, masters_comm; 
	
		MPI_Comm_group( MPI_COMM_WORLD, &temp1);
		MPI_Comm_group( MPI_COMM_WORLD, &temp2);

		no_master_of_masters = no_procs/4;
		no_masters = no_procs - no_master_of_masters;

		int *ranks, *mranks;
		ranks = new int[no_master_of_masters];
		mranks = new int[no_masters];
		for(int i = 0; i < no_master_of_masters; i++){
			ranks[i] = i;
		}
		for(int i = no_master_of_masters; i < no_procs; i++){
			mranks[i-no_master_of_masters] = i;
		}
		//create communicator for masters of masters
		MPI_Group_incl(temp1, no_master_of_masters, ranks, &mmasters_group);
		MPI_Comm_create(MPI_COMM_WORLD, mmasters_group, &mmasters_comm);

		//create communicator for masters
		MPI_Group_incl(temp2, no_masters, mranks, &masters_group);
		MPI_Comm_create(MPI_COMM_WORLD, masters_group, &masters_comm);


		/*MPI Test */
		if(!ismasterofmaster(my_id)){
			queue = new WorkQueue();
			load_dummy_data();
			cout << "Queue size:" << queue->get_size() << ":: Process:" << my_id << endl;
		}

		

		// Are you master of master? then initialize stuff
			
		// Else stay quiet

		// MPI Barrier;

		MPI_Barrier(MPI_COMM_WORLD);
		
		
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
			local_load = new int[no_masters];
			// set initial load to 0
			memset(load, 0, no_masters);
			memset(local_load, 0, no_masters);

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

		MPI_Barrier(MPI_COMM_WORLD);

		// This process runs indefinitely till the program is cancelled
		
		while(1){
			// loads are updated by all the masters to masters of masters
			
			pthread_mutex_lock (&entire_queue_mutex);
			
			//MPI_Barrier(MPI_COMM_WORLD);

			if(ismasterofmaster(my_id)){
				memset(local_load, 0, no_masters);
				MPI_Request requests[no_myslaves];
				MPI_Status status[no_myslaves];
				for(int i = 0 ; i < no_myslaves; i++){
					MPI_Irecv(&local_load[myslaves[i] - no_master_of_masters], 1, MPI_INT, myslaves[i], LOAD_UPDATE, MPI_COMM_WORLD, &requests[i]);
				}
				for(int i = no_master_of_masters ; i < no_procs; i++){
					if(i % no_master_of_masters != my_id){
						local_load[i - no_master_of_masters] = 0;
					}
				}
				MPI_Waitall(no_myslaves, requests, status);
				
			}else{
				MPI_Request request;
				MPI_Status status;
				int queue_size = queue->get_size();
				MPI_Isend(&queue_size, 1, MPI_INT, my_master, LOAD_UPDATE, MPI_COMM_WORLD, &request);
				MPI_Wait(&request, &status);
			}
			MPI_Barrier(MPI_COMM_WORLD);

			
			// syncronize load information amongst masters of masters
			if(ismasterofmaster(my_id)){
				memset(load, 0, no_masters);
				MPI_Reduce(&local_load[0], &load[0], no_masters, MPI_INT, MPI_SUM, MEGA_MASTER, mmasters_comm);
				MPI_Bcast(&load[0], no_masters, MPI_INT, MEGA_MASTER, mmasters_comm);
			}
			
			MPI_Barrier(MPI_COMM_WORLD);

			
			// Analyze loads and send each process a message
			int steps = 0;
			if(ismasterofmaster(my_id)){
				int average_load = 0;
				int sum = 0;
				for(int i = 0; i < no_masters; i++){
					sum += load[i];
				}
				average_load = ((1.0) * sum) / no_masters;
				int out_of_order = 0;
				double underload = 0.75 * (average_load);
				double overload = 1.25 * (average_load);
				if(my_id == MEGA_MASTER){
					cout << "underload:" << floor(underload) << endl;
					cout << "overload:" << floor(overload) << endl;
				}
				for(int i = 0; i < no_masters; i++){
					if((load[i] < floor(underload) || load[i] > floor(overload)) && load[i] > 4 * EPSILON){
						out_of_order++;
					}
				}
				if(my_id == MEGA_MASTER){
					cout << "Out of Order:" << out_of_order << endl;
				}
				if(out_of_order > 0 && out_of_order < no_masters/4){
					steps = 4;
				}else if(out_of_order > no_masters/4 && out_of_order < no_masters/2){
					steps = 3;
				}else if(out_of_order > no_masters/2 && out_of_order < (3*no_masters)/4){
					steps = 2;
				}else{
					steps = 1;
				}
				MPI_Bcast(&steps, 1, MPI_INT, MEGA_MASTER, MPI_COMM_WORLD);
			}else{
				MPI_Bcast(&steps, 1, MPI_INT, MEGA_MASTER, MPI_COMM_WORLD);
			}
			
			fflush(stdout);
			if(my_id == MEGA_MASTER){
				cout << "steps for balance:" << steps << endl;
				cout << "-------------------------------\nCompleted Final\n-------------------------------" << endl;
			}
			/*if(my_id == MEGA_MASTER){
				cout << "Load at Slave 1:" << load[myslaves[0] - no_master_of_masters + 1] << endl;
				cout << "-------------------------------\nCompleted Final\n-------------------------------" << endl;
			}*/

			MPI_Barrier(MPI_COMM_WORLD);

			// initiate queue item transfers 'steps' times

			// lock external loading and queue processing
			
			if(!ismasterofmaster(my_id)){
				int my_curr_rank;
				MPI_Comm_rank(masters_comm, &my_curr_rank);
				steps  = 1;
				int test_proc = 8;
				for(int i = 0; i < steps; i++){
					MPI_Status stat;
					if(my_id == test_proc || my_id == test_proc + 1){
					if(((my_curr_rank + i) % no_masters) % 2 == 0){
						int neighbor_load = 0;
						int diff = 0;
						int right_neighbor = (my_curr_rank + 1) % no_masters;
						MPI_Recv(&neighbor_load, 1, MPI_INT, right_neighbor, LOAD_UPDATE, masters_comm, &stat);
						if(my_id == test_proc){
							cout << "Neighbor_load:" << neighbor_load << endl;
						}
						//calculate to send the send diff

						diff = (queue->get_size() - neighbor_load)/2;
						
						if(my_id == test_proc){
							cout << "Diff:" << diff << endl;
						}
						
						MPI_Send(&diff, 1, MPI_INT, right_neighbor, DIFF_UPDATE, masters_comm);
						
						//Initiate transfer Send or Recv
						if(diff > EPSILON || diff < -EPSILON){
							int size; 
							char *buffer;
							if(diff > EPSILON){
								buffer = get_filenames_buffer(diff, &size);
								if(my_id == test_proc){
									cout << "1: size:" << size << endl;
								}
								// send the size and create buffer to hold the message
								MPI_Send(&size, 1, MPI_INT, right_neighbor, SIZE_UPDATE, masters_comm);
								// send the message
								MPI_Send(&buffer[0], size, MPI_CHAR, right_neighbor, LOAD_TRANSFER, masters_comm);
							
							}else{
								diff = -1 * diff;
								// receive the size
								MPI_Recv(&size, 1, MPI_INT, right_neighbor, SIZE_UPDATE, masters_comm, &stat);
								buffer = new char[size];
								// recieve the message
								MPI_Recv(&buffer, size, MPI_CHAR, right_neighbor, LOAD_TRANSFER, masters_comm, &stat);
								add_filenames(buffer);
							}
						}
						
					}else{								
						int my_load = queue->get_size();
						int diff = 0;
						int left_neighbor = ((my_curr_rank - 1) < 0) ? (no_masters - 1) : (my_curr_rank - 1);
						MPI_Send(&my_load, 1, MPI_INT, left_neighbor, LOAD_UPDATE, masters_comm);
						//recieve difference
						MPI_Recv(&diff, 1, MPI_INT, left_neighbor, DIFF_UPDATE, masters_comm, &stat);						
							
						//Initiate transfer: Send or Recv
						if(diff > EPSILON || diff < -EPSILON){
							int size;
							char *buffer;
							if(diff > EPSILON){
								// receive the size
								MPI_Recv(&size, 1, MPI_INT, left_neighbor, SIZE_UPDATE, masters_comm, &stat);
								buffer = new char[size];
								// recieve the message
								if(my_id == test_proc + 1){
									cout << "size:" << size << endl;
								}
								MPI_Recv(&buffer, size, MPI_CHAR, left_neighbor, LOAD_TRANSFER, masters_comm, &stat);
								/*add_filenames(buffer);*/
							
							}else{
								diff = -1 * diff;
								buffer = get_filenames_buffer(diff, &size);
								if(my_id == test_proc + 1){
									cout << "size:" << size << endl;
								}
								//send the size and create buffer to hold the message
								MPI_Send(&size, 1, MPI_INT, left_neighbor, SIZE_UPDATE, masters_comm);
								
								// send the message
								MPI_Send(&buffer[0], size, MPI_CHAR, left_neighbor, LOAD_TRANSFER, masters_comm);
								
							}
						}
													
					}
					}
					MPI_Barrier(masters_comm);												
				}
			}			
			
			pthread_mutex_unlock (&entire_queue_mutex);
			// wait for transfers to complete before starting the loop again
			sleep(1);			
			MPI_Barrier(MPI_COMM_WORLD);
			
		}
		
		MPI_Barrier(MPI_COMM_WORLD);

		// once you break join all the threads on each of the master nodes

		if(!ismasterofmaster(my_id)){
		        for(int t=0; t < NUM_THREADS; t++) {
		                pthread_join(threads[t],NULL);
		        }
		}
		
		// finalize 
		
		MPI_Barrier(MPI_COMM_WORLD);
		
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

		while(1){
			pthread_mutex_lock (&entire_queue_mutex);
			int size = queue->get_size();
			if(size == 0){
				pthread_mutex_unlock (&entire_queue_mutex);
				return (void *)1;	
			}
			work_item *item = queue->dequeue();
			pthread_mutex_unlock (&entire_queue_mutex);
			if(item != NULL){
				//sleep(item->load);
				//fflush(stdout);
				//cout << "Processed File:" << item->filename << ":: Process:" << my_id << "::Thread:" << ((int) thread_id) << endl;
				
				delete item->filename;
				delete item;
			}			
		}		
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

