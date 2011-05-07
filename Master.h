#include <iostream>
#include <fstream>
#include <map>
#include <list>
#include <pthread.h>

#ifndef MASTER_H
#define MASTER_H

using namespace std;

class Master
{
	private:
		
	public:
		Master(void);
		~Master(void);
		void ProcessFunction(void *pid);
};

#endif
