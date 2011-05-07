#include <iostream>
#include <sstream>
#include <cstdlib>
#include <map>
#include <list>
#include <ctime>
#include <string.h>
#include <math.h>
#include <fstream>
#include <pthread.h>
#include "Queue.h"
#include "SharedQueue.h"


#define NUM_THREADS 16

using namespace std;

SharedQueue::SharedQueue(void){
	
}

SharedQueue::~SharedQueue(){

}

void SharedQueue::ThreadFunction(void *threadid)
{
	pthread_t threads[NUM_THREADS];
	
	/*
	for(int t=0; t < NUM_THREADS; t++){
      		error_code = pthread_create(&threads[t], NULL, TFunction, (void *)t);
      		if (error_code){
	 		printf("ERROR; return code from pthread_create() is %d\n", error_code);
	 		exit(-1);
      		}
   	} 
   	for(int t=0; t<NUM_THREADS; t++) {
	 	int rc = pthread_join(threads[t], NULL);
	 	if (rc) {
	    		printf("ERROR; return code from pthread_join() is %d\n", rc);
	    		exit(-1);
	 	}
   	}  
	*/

}

