#include <iostream>
#include <cstdlib>
#include <map>
#include <list>
#include <string.h>

#include "structures.h"
#include "WorkQueue.h"

using namespace std;

WorkQueue:: WorkQueue(){	
	//initialize;
	start = NULL;
	end = NULL;
	size = 0;
	pthread_mutex_init(&queue_mutex, NULL);
}

WorkQueue:: ~WorkQueue(){
	pthread_mutex_destroy(&queue_mutex);
}

bool WorkQueue::enqueue(work_item *item){
	pthread_mutex_lock (&queue_mutex);
	if(start == NULL && end == NULL){
		start = end = item;
		size++;
	}else{
		end->next = item;
		end = item;
		size++;
	}
	pthread_mutex_unlock(&queue_mutex);
}

bool WorkQueue::enqueue(work_item **items, int num){
	if(items == NULL){
		return false;
	}
	for(int i = 0; i < num; i++){
		if(items[i] == NULL)
			return false;
		enqueue(items[i]);
	}
	return true;
}

work_item* WorkQueue::dequeue(void){
	pthread_mutex_lock(&queue_mutex);
	if(start == NULL && end == NULL){
		pthread_mutex_unlock(&queue_mutex);
		return NULL;		
	}else{
		work_item* temp = start;
		start = start->next;
		if(start == NULL)
			end = NULL;
		size--;
		pthread_mutex_unlock(&queue_mutex);
		return temp;
	}	
}

work_item** WorkQueue::dequeue(int num){
	pthread_mutex_lock(&queue_mutex);
	work_item **items;
	if(num <= size){
		items = new work_item*[num];	
		for(int i = 0; i < num; i++){
			work_item* temp = start;
			start = start->next;
			size--;
			pthread_mutex_unlock(&queue_mutex);
			items[i] = temp;
		}
		if(start == NULL)
			end = NULL;
		pthread_mutex_unlock(&queue_mutex);
		return items;
	}else{
		pthread_mutex_unlock(&queue_mutex);
		return NULL;
	}
}

int WorkQueue::get_size(void){
	return size;
}

work_item* WorkQueue::get_at(int index){
	work_item *temp = start;
	int count = 0;
	while(temp != NULL){
		count++;
		if(count == index){
			return temp;		
		}
		temp = temp->next;	
	}
	return NULL;
}

work_item* WorkQueue::chop_from(int index){
	work_item *temp = start;
	int count = 0;
	while(temp != NULL){
		count++;
		if(count == (index - 1)){
			end = temp;
			size = (index - 1);
			free_from(end->next);
			end->next = NULL;
			return NULL;				
		}
		temp = temp->next;
	}
	return NULL;
}

void WorkQueue::free_from(work_item *temp){
	work_item *temp1;
	while(temp != NULL){
		temp1 = temp->next;
		delete temp->filename;
		delete temp;
		temp = temp1;			
	}

}

void WorkQueue::reset_queue(void){
	pthread_mutex_lock(&queue_mutex);
	start = end = NULL;
	size = 0;	
	pthread_mutex_unlock(&queue_mutex);
}

