#include <iostream>
#include <cstdlib>
#include <map>
#include <list>
#include <string.h>

#include "FileIO.h"
#include "structures.h"

using namespace std;

FileIO:: FileIO(char *filename){
	lines = 0;	
	pthread_mutex_init(&output_file, NULL);	
	outFile.open(filename);
	if (!outFile) {
   		cerr << "Unable to open file " << filename << " for writing the output." << endl;
    		exit(1);   // call system to stop
	}
}

FileIO:: ~FileIO(){
	outFile.close();
}

void FileIO::writeToFile(work_item* head){
	
	if(head == NULL){
		printf("Error in writing to file: Empty queue\n");
	}else{		
		pthread_mutex_lock (&output_file);
		outFile << ++lines << ". " << head->filename << " " << head->isspam << " " << endl;
		pthread_mutex_unlock (&output_file);
	}
	return;
}
