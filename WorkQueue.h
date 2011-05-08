#include <iostream>
#include <fstream>
#include <map>
#include <list>
#include <pthread.h>

#include "structures.h"

#ifndef WORKQUEUE_H
#define WORKQUEUE_H

using namespace std;

class WorkQueue
{
	private:
		pthread_mutex_t queue_mutex;
		work_item *start;
		work_item *end;
		int size;
	public:
		WorkQueue(void);
		~WorkQueue(void);
		bool enqueue(work_item *);
		bool enqueue(work_item **, int);
		work_item* dequeue(void);
		work_item** dequeue(int num);
		void reset_queue(void);
};

#endif
