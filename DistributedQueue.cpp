#include <mpi.h>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <stdio.h>
#include <map>
#include <list>
#include <ctime>
#include <string.h>
#include <math.h>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "FileIO.h"
#include "WorkQueue.h"
#include "DistributedQueue.h"

#define NUM_THREADS 4

//MPI tags

#define LOAD_UPDATE 0
#define DIFF_UPDATE 1
#define SIZE_UPDATE 2
#define LOAD_TRANSFER 3

#define MEGA_MASTER 0
#define EPSILON 4

#define PORT_NO 517113

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
	int count = 0;
	while(item != NULL){
		buff.append(item->filename);
		buff.append("|");
		item = item -> next;
		count++;
	}
	final_buff = new char[buff.size() + 1];
	strcpy(final_buff, buff.c_str());
	*(size) = buff.size() + 1;
	queue->chop_from(start);
	fflush(stdout);
	//cout << "get_filenames_buffer:" << my_id << "::Removed:" << count << endl;
	return final_buff;
}

char* DistributedQueue::add_filenames(char *buffer){
	char *temp;
	temp = strtok(buffer, "|");
	int count = 0;
	fflush(stdout);
	while(temp != NULL){
		work_item *p = new work_item;
		p->load = 1;
		p->filename = new char[strlen(temp) + 1];
		strcpy(p->filename, temp);
		p->isspam = false;
		p->next = NULL;
		queue->enqueue(p);
		temp = strtok(NULL, "|");
		count++;	
	}
	//cout << "add_filenames:" << my_id << "::Added:" << count << endl;
}


void DistributedQueue::load_dummy_data(void){
	char command[50];
	srand ( (my_id + 1) * time(NULL) );
	int load = rand() % 500;
	system("cd temp;rm -rf *;cd ..");
	for(int i = 0; i < load; i++){
		work_item *item = new work_item;
		item->filename = new char[30];
		//gen_random(item->filename, 8);
		strcpy(item->filename, "README_T");
		item->load = 1;
		//item->load = (rand() % 2) + 1;
		item->isspam = false;
		item->next = NULL;
		queue->enqueue(item);
		//sprintf(command,"cp README ./temp/%s",item->filename);		
		system(command);
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

		no_master_of_masters = no_procs/5;
		no_masters = no_procs - no_master_of_masters;

		char file_name[20];

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
			result_queue = new WorkQueue();
			sprintf(file_name, "Output_%d.txt\0", my_id);
			file_obj = new FileIO(file_name);
			load_dummy_data();
			cout << "INITIAL Queue size at node " << my_id << " is:" << queue->get_size() << endl;
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

		/*if(my_id == 2){
			fflush(stdout);
			cout << my_id << "Initial files:" << endl;
			work_item *t;
			int count = 0;
			while((t = queue->dequeue()) != NULL){
				cout << (count++) << "::" << t->filename << endl;
			}		
		}

		MPI_Barrier(MPI_COMM_WORLD);


		if(my_id == 3){
			fflush(stdout);
			cout << my_id << ":Initial files:" << endl;
			work_item *t;
			int count = 0;
			while((t = queue->dequeue()) != NULL){
				cout << (count++) << "::" << t->filename << endl;
			}		
		}*/

		MPI_Barrier(MPI_COMM_WORLD);

		// This process runs indefinitely till the program is cancelled
		
		int round = 1;

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
					//cout << "underload:" << floor(underload) << endl;
					//cout << "overload:" << floor(overload) << endl;
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
					steps = 5;
				}else if(out_of_order > no_masters/4 && out_of_order < no_masters/2){
					steps = 4;
				}else if(out_of_order > no_masters/2 && out_of_order < (3*no_masters)/4){
					steps = 4;
				}else if(out_of_order > (3*no_masters)/4 && out_of_order < no_masters){
					steps = 5;
				}else{
					steps = 0;
				}
				if(my_id == MEGA_MASTER){
					cout << "Steps:" << steps << endl;
				}
				MPI_Bcast(&steps, 1, MPI_INT, MEGA_MASTER, MPI_COMM_WORLD);
			}else{
				MPI_Bcast(&steps, 1, MPI_INT, MEGA_MASTER, MPI_COMM_WORLD);
			}
			
			fflush(stdout);

			MPI_Barrier(MPI_COMM_WORLD);
			
			/*if(my_id == MEGA_MASTER){
				cout << "-------------------------------\nBeforeRound\n-------------------------------" << endl;
				cout << "Round:" << (round++) << endl;
				cout << "-------------------------------\nBeforeRound Final\n-------------------------------" << endl;
			}*/
	
			// initiate queue item transfers 'steps' times

			// lock external loading and queue processing
			
			if(!ismasterofmaster(my_id)){
				int my_curr_rank;
				MPI_Comm_rank(masters_comm, &my_curr_rank);
				int test_proc = 8;
				//steps = 1;
				for(int i = 0; i < steps; i++){
					MPI_Status stat;
					//if(my_id == test_proc || my_id == test_proc + 1){
					if(((my_curr_rank + i) % no_masters) % 2 == 0){
						int neighbor_load = 0;
						int diff = 0;
						int right_neighbor = (my_curr_rank + 1) % no_masters;
						MPI_Recv(&neighbor_load, 1, MPI_INT, right_neighbor, LOAD_UPDATE, masters_comm, &stat);				
						//calculate to send the send diff

						diff = (queue->get_size() - neighbor_load)/2;
						
						MPI_Send(&diff, 1, MPI_INT, right_neighbor, DIFF_UPDATE, masters_comm);
						
						//Initiate transfer Send or Recv
						if(diff > EPSILON || diff < -EPSILON){
							int size; 
							char *buffer;
							if(diff > EPSILON){
								//cout << "SB Queue size:" << queue->get_size() << endl;
								buffer = get_filenames_buffer(diff, &size);
								//cout << "SA Queue size:" << queue->get_size() << endl;
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
								MPI_Recv(&buffer[0], size, MPI_CHAR, right_neighbor, LOAD_TRANSFER, masters_comm, &stat);
								//cout << "RB Queue size:" << queue->get_size() << endl;
								add_filenames(buffer);
								//cout << "RA Queue size:" << queue->get_size() << endl;								
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
								MPI_Recv(&buffer[0], size, MPI_CHAR, left_neighbor, LOAD_TRANSFER, masters_comm, &stat);
								//cout << "RB Queue size:" << queue->get_size() << endl;
								add_filenames(buffer);
								//cout << "RA Queue size:" << queue->get_size() << endl;								
							
							}else{
								diff = -1 * diff;
								//cout << "SB Queue size:" << queue->get_size() << endl;
								buffer = get_filenames_buffer(diff, &size);
								//cout << "SA Queue size:" << queue->get_size() << endl;
								//send the size and create buffer to hold the message
								MPI_Send(&size, 1, MPI_INT, left_neighbor, SIZE_UPDATE, masters_comm);
								
								// send the message
								MPI_Send(&buffer[0], size, MPI_CHAR, left_neighbor, LOAD_TRANSFER, masters_comm);
								
							}
						}
													
					}
					//}
					MPI_Barrier(masters_comm);												
				}
			}

			fflush(stdout);

			if(my_id >= no_master_of_masters){
				cout << "Queue size at node " << my_id << " is:" << queue->get_size() << endl;
				//cout << "-------------------------------\nCompleted Final\n-------------------------------" << endl;
			}
		
			/*fflush(stdout);

			if(my_id >= no_master_of_masters){
				cout << "AFTER Queue size at" << my_id << " is:" << queue->get_size() << endl;
				if( queue->get_size() > 0 && queue->dequeue() == NULL){
					cout << "THIS IS CRAP" << endl;
				}
				//cout << "-------------------------------\nCompleted Final\n-------------------------------" << endl;
			}*/

			pthread_mutex_unlock (&entire_queue_mutex);
			// wait for transfers to complete before starting the loop again
			sleep(3);			
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


		char command[50];
		char spamString[10];
		float spamProb = 0.0;
		bool flag;
		int c;
		int initial_sleep = 0;
		
		while(1){
			
			pthread_mutex_lock (&entire_queue_mutex);
			int size = queue->get_size();
			if(size == 0){
				pthread_mutex_unlock (&entire_queue_mutex);
				if((int)thread_id == 1){
					work_item *p;
					p = result_queue->dequeue();
					if(p != NULL){
						file_obj->writeToFile(p);
						delete p->filename;
						delete p;
					}					
				}
				//return (void *)1;	
			}else{
				initial_sleep = 0;
				work_item *item = queue->dequeue();
				pthread_mutex_unlock (&entire_queue_mutex);
				if(item != NULL){
					//sleep(item->load);
					//if(my_id == 2 || my_id == 3){
					fflush(stdout);
					//cout << ":: Process:" << my_id << "::Thread:" << ((int) thread_id) << "Processed File:" << item->filename << endl;
					//}

					//sprintf(command,"cat ./temp/%s | ./bmf -t",item->filename);

				        sprintf(command,"cat %s | ./bmf -t",item->filename);

					FILE *bmfOutput = popen(command, "r"); // System call to bmf
				
					if(bmfOutput == NULL){
						cout << "THREAD_ID::" << ((int) thread_id) << "Error opening the bmf Output file" << endl;   
					}else{
						do{
		      					c = fgetc (bmfOutput);      					
		    				} while (c != ':');
						c = fgetc(bmfOutput); //File pointer now at the spamProb value

						fgets(spamString, 8, bmfOutput);	
						spamString[8] = '\0';
						spamProb = atof(spamString);			
						fclose (bmfOutput);				
					}			
					if(spamProb < 0.500000f){
						item->isspam = false;
					}else{
						item->isspam = true;
					}

					//cout << ":: Is SPAM:" << item->isspam <<  endl;

					work_item *temp = new work_item;
					temp->filename = new char[strlen(item->filename) + 1];
					strcpy(temp->filename,item->filename);
					temp->load = item->load;
					temp->isspam = item->isspam;
					temp->next = NULL;
					flag = result_queue->enqueue(temp);
					

					delete item->filename;
					delete item;
				}
			}		
		}		
		return (void *)1;

}

void* DistributedQueue::AcceptExternalLoad(void* thread_id){

		// socket to accept filenames as input for queues

      		// This thread runs indefinitely till the program is cancelled
	     /*
	     int sockfd, newsockfd, portno, pid;
	     socklen_t clilen;
	     struct sockaddr_in serv_addr, cli_addr;
	     sockfd = socket(AF_INET, SOCK_STREAM, 0);
	     if (sockfd < 0) 
		fprintf(stderr, "ERROR opening socket");
	     bzero((char *) &serv_addr, sizeof(serv_addr));
	     portno = PORT_NO + my_id - 2;
	     serv_addr.sin_family = AF_INET;
	     serv_addr.sin_addr.s_addr = INADDR_ANY;
	     serv_addr.sin_port = htons(portno);
	     if (bind(sockfd, (struct sockaddr *) &serv_addr,
		      sizeof(serv_addr)) < 0) 
		      fprintf(stderr, "ERROR on binding");
	     listen(sockfd,5);
	     clilen = sizeof(cli_addr);
	     while (1) {
		 newsockfd = accept(sockfd, 
		       (struct sockaddr *) &cli_addr, &clilen);
		 if (newsockfd < 0) 
		     fprintf(stderr, "ERROR on accept");
		 char buffer[256];		      
	         bzero(buffer,256);
		 int n;
	         n = read(newsockfd,buffer,255);
	         work_item *item = new work_item;
	         item->filename = new char[strlen(buffer)+1];
	         strcpy(item->filename, buffer);
	         item->isspam = false;
	         item->load = 1;
	         item->next = NULL;
	         pthread_mutex_lock (&entire_queue_mutex);
	         queue->enqueue(item);
	         pthread_mutex_unlock (&entire_queue_mutex);
		 pid = fork();
		 if (pid < 0)
		     fprintf(stderr,"ERROR on fork");
		 if (pid == 0)  {
		     close(sockfd);
		     exit(0);
		 }
		 else close(newsockfd);
	     } 
	     close(sockfd);
	     return 0; 
	     */
	     return (void *)0;
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

