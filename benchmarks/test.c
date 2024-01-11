#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "../thread-worker.h"

/* A scratch program template on which to call and
 * test thread-worker library functions as you implement
 * them.
 *
 * You can modify and use this program as much as possible.
 * This will not be graded.
 */

void t4();
void t3();
void* t2()
{
	int x = 1;
	while(x)
	{
		
		printf("in thread2\n");
		x=0;
	}
	//worker_exit(NULL);
	return NULL;
}


void t3()
{       
        int x = 1;
        while(x)
        {
         
                printf("in thread3\n");
                x=0;
        }

	//worker_exit(NULL);
	return;	
}


void t4()
{       
        int x = 1;
        while(x)
        {
         
                printf("in thread4\n");
                x=0;
        }

	//worker_exit(NULL);
	return;
}




int main(int argc, char **argv) {

	/* Implement HERE */
	pthread_t t2num;
	pthread_create(&t2num,NULL,t2,NULL);	
//	pthread_join(t2num,NULL);

	pthread_t t3num;
        pthread_create(&t3num,NULL,t3,NULL);
//	pthread_join(t3num,NULL);	
	
        pthread_t t4num;
        pthread_create(&t4num,NULL,t4,NULL);
//	pthread_join(t4num,NULL);

	
	pthread_join(t2num,NULL);
	pthread_join(t3num,NULL);	
	pthread_join(t4num,NULL);
	
	puts("test.c is terminating t2t3t4 shouldve printed\n");
	return 0;
}
