#include <iostream>
#include <fstream>
#include <map>
#include <list>
#include <pthread.h>

#ifndef STRUCTURES_H
#define STRUCTURES_H

using namespace std;

// work item inserted into the queue

struct work_item {
	unsigned short load;
	char *filename;
	bool isspam;
	work_item *next;
};

typedef struct work_item work_item;

struct multiple_args{
        void *obj;
        void *tid;
};

typedef struct multiple_args m_args;

#endif
