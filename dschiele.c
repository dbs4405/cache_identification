/**************************************************
* Title: Cache Identifier
* Author: David Schiele
* To run: gcc -O dschiele.c -o dschiele && ./dschiele
* Approximate run time: 6 minutes
**************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define MAX ((1024*1024*32)/sizeof(int))//32 MB array
#define MIN 2048 //min array size =8kb
#define ITERATIONS 1024*1024*10
int array2_size = 1024*1024*30;
static int array2[1024*1024*30];
static int array[MAX];
int current_size;
struct timeval tvafter,tvpre;
int results_counter = 0; 
int elapsed_time = 0;
int levels_found = 0;//number of cache levels found
float output = 0.0;//current time / prev time
int time_results[28];//time to pass through at each array size
int size_results[28];//array size corresp to time
int test_index = 0;
int sizes[4];

void initialize_array() {
	for (int i = 0; i < MAX; i++) array[i] = rand() % MAX;
}

//update elements in array 
void get_size() {
 	current_size = MIN;
	double it = ITERATIONS;
	double total = current_size * it;
	int a = 0;
	while (current_size <= MAX) {
		gettimeofday (&tvpre , NULL);
		for (int k = 0; k < it; k++) {	
			for (int j = 0; j < current_size; j=j+64) array[j] = array[j]+1;
		}
		gettimeofday (&tvafter , NULL);
		elapsed_time = ((tvafter.tv_sec-tvpre.tv_sec)*1000+(tvafter.tv_usec-tvpre.tv_usec)/1000);
		size_results[results_counter] = current_size;
		time_results[results_counter] = elapsed_time;
		results_counter++;
		if (current_size <= 262144) {
			current_size = current_size*2;
			it = it*0.5;	
		} else if (current_size > 262144 && current_size < 2621440) {
			current_size = current_size+262144; //increase size by 1mb	
			it = total / current_size; } 
		else {
		    current_size = current_size+524288; //increase size by 2 mb	
			it = total / current_size;
		}
		total = current_size * it;
	}//end while
}

//find levels and size of each levels
void find_levels() {
	int size_counter = 0;
	float change[27];
	float max_change = 0;
	for (int i = 1; i < 28; i++) {
		output = ((float)time_results[i]/(float)time_results[i-1]);//output = current/previous
		change[i] = output;//store output in %change array
		if (output > max_change && size_results[i-1] > 16384) {
			max_change = output;
			test_index = i-1;//store previous prev index
		}
		if (output > 1.14 && levels_found < 3) {
			if (size_results[i] < 262144) {
				sizes[size_counter] = (size_results[i-1]);
				printf("Level %d: %d kb\n", levels_found+1,(sizes[size_counter] * 4)/1024);
				size_counter++;	
			} else {
				sizes[size_counter] = (size_results[i]);
				printf("Level %d: %d mb\n", levels_found+1, (sizes[size_counter] *4) / (1024*1024));
				size_counter++;	
			}
			levels_found ++;
		}
	}
	if (levels_found == 0) printf("This machine has 0 levels of cache.\n");
		else printf("This machine has %d levels of cache.\n", levels_found);
}	

//find the line size by varying the stride
void get_line_size() {
	elapsed_time = 0;
	int prevTime = -1;
	int prevSize = 0;
	float max = 0;
	int stride_counter = 0;
	int line_size = 0;
	for (int stride = 1; stride <= 128; stride=stride*2) {	
		long long accesses = 0;
		output = 0;
		gettimeofday (&tvpre , NULL); //start time
		for (int r = 0; r < 1024*10; r++) {
			for (int j = 0; j < MAX/4 && accesses < 1024*1024*1024; j = j + stride) {
				array[j] = array[j]+1;
				accesses++;
			}
		}
		gettimeofday (&tvafter , NULL);
		elapsed_time = ((tvafter.tv_sec-tvpre.tv_sec)*1000+(tvafter.tv_usec-tvpre.tv_usec)/1000);
		output = ((float)elapsed_time/(float)prevTime);//output = current/previous
		if (prevTime != -1) {
			if (output > max) {
				max = output;
				line_size = stride;		
			}	
		}
		prevTime = elapsed_time;
		prevSize = stride;
	}	
	printf("The line size is %d bytes.\n", line_size*4);
}

void initialize_array2() {
	for (int i = 0; i < array2_size; i++) array[i] = rand() % array2_size;
}

/*measure associativity for each level identified by setting stride = cache size
and increasing array size*/ 
void find_associativity() {
	int accesses = 0;
	initialize_array2();	
	int stride_a = 0;
	int number = 0;
	for (int j = 0; j < levels_found; j++) {	
		accesses = 0;
		float max = 0.0;
		int assoc_size = 0; 
		float output = 0;
		int prev_size = 0;
		int prev_time = -1;
		stride_a = sizes[j];
		int limit = stride_a * 20;
		for (number = stride_a * 4; number <= limit;) {	
			gettimeofday (&tvpre , NULL); //start time
			int element = 0;
			for (int i = 0; i < 1024*1024*1024; i++) {	
				array2[element] = array2[element] + 1;
				element = element + stride_a;
				accesses++;
				if (element > number) element = 0;		
			}
			gettimeofday (&tvafter , NULL);
			elapsed_time = ((tvafter.tv_sec-tvpre.tv_sec)*1000+(tvafter.tv_usec-tvpre.tv_usec)/1000);
			if (prev_time != -1) output = (float)elapsed_time / (float)prev_time;
			if (output > max) {
				max = output;
				assoc_size = prev_size;
			}
			prev_size = number;
			number = number+stride_a;
			prev_time = elapsed_time;	
		}
		printf("Level %d associativity: %d\n", j+1, (assoc_size/stride_a));
	}
}

int main () {	
	initialize_array();
    get_size();  	
  	find_levels();
  	if (levels_found != 0) {
  		get_line_size();
  		find_associativity(); 	
	}
	return 0;
}