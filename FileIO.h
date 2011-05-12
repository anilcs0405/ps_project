#include <iostream>
#include <fstream>
#include <map>
#include <list>
#include <pthread.h>

#include "structures.h"

#ifndef FILEIO_H
#define FILEIO_H

using namespace std;

class FileIO
{
	private:
		ofstream outFile;
		pthread_mutex_t output_file;
		int lines;
	public:
		FileIO(char *);
		~FileIO(void);
		void writeToFile(work_item* head);
};

#endif
