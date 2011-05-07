#include <iostream>
#include <fstream>
#include <pthread.h>

#include "Queue.h"

using namespace std;

class SharedQueue
{
	private:		

	public:
		SharedQueue();
		~SharedQueue();
		void ThreadFunction(void *threadid);
};
