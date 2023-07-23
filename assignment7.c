#include "assignment7.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#define SORT_THRESHOLD      40

typedef struct _sortParams {
    char** array;
    int left;
    int right;
} SortParams;

static int maximumThreads;              /* maximum # of threads to be used */
int usedThreads = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;;

/* This is an implementation of insert sort, which although it is */
/* n-squared, is faster at sorting short lists than quick sort,   */
/* due to its lack of recursive procedure call overhead.          */

static void insertSort(char** array, int left, int right) {
    int i, j;
    for (i = left + 1; i <= right; i++) {
        char* pivot = array[i];
        j = i - 1;
        while (j >= left && (strcmp(array[j],pivot) > 0)) {
            array[j + 1] = array[j];
            j--;
        }
        array[j + 1] = pivot;
    }
}

/* Recursive quick sort, but with a provision to use */
/* insert sort when the range gets small.            */

static void quickSort(void* p) {
    SortParams* params = (SortParams*) p;
    char** array = params->array;
    int left = params->left;
    int right = params->right;
    int i = left, j = right;

    //Used to check if threads are already created
    int checkLeft = -1; 
    int checkRight = -1;

    //Used for error checking
    int threadAction; 

    //Create a thread for each side of the split
    pthread_t leftPartition;
    pthread_t rightPartition;

    
    if (j - i > SORT_THRESHOLD) {           /* if the sort range is substantial, use quick sort */

        int m = (i + j) >> 1;               /* pick pivot as median of         */
        char* temp, *pivot;                 /* first, last and middle elements */
        if (strcmp(array[i],array[m]) > 0) {
            temp = array[i]; array[i] = array[m]; array[m] = temp;
        }
        if (strcmp(array[m],array[j]) > 0) {
            temp = array[m]; array[m] = array[j]; array[j] = temp;
            if (strcmp(array[i],array[m]) > 0) {
                temp = array[i]; array[i] = array[m]; array[m] = temp;
            }
        }
        pivot = array[m];

        for (;;) {
            while (strcmp(array[i],pivot) < 0) i++; /* move i down to first element greater than or equal to pivot */
            while (strcmp(array[j],pivot) > 0) j--; /* move j up to first element less than or equal to pivot      */
            if (i < j) {
                char* temp = array[i];      /* if i and j have not passed each other */
                array[i++] = array[j];      /* swap their respective elements and    */
                array[j--] = temp;          /* advance both i and j                  */
            } else if (i == j) {
                i++; j--;
            } else break;                   /* if i > j, this partitioning is done  */
        }

        //Check if max # of threads are currently in use
        SortParams first;  first.array = array; first.left = left; first.right = j;
        if(usedThreads < maximumThreads) {
            //Wait for usedThreads (thread count) to catch up
            threadAction = pthread_mutex_lock(&mutex);
            if(threadAction != 0) {
                printf("%s\n", strerror(errno));
            }
            //Create a thread for LEFT partition
            threadAction = pthread_create(&leftPartition, NULL, (void*)quickSort, &first);
            checkLeft = threadAction; 
            if(threadAction != 0) {
                printf("%s\n", strerror(errno));
            }
            usedThreads++;
            //Unlock because usedThreads has caught up
            threadAction = pthread_mutex_unlock(&mutex);
            if(threadAction != 0) {
                printf("%s\n", strerror(errno));
            }
        }
        // Else don't create another thread (use the current one)
        else { quickSort(&first); } /* sort the left partition  */

        // Check if max # of threads are currently in use
        SortParams second; second.array = array; second.left = i; second.right = right;
        if(usedThreads < maximumThreads) {
            // Wait for usedThreads (thread count) to catch up
            threadAction = pthread_mutex_lock(&mutex);
            if(threadAction != 0) {
                printf("%s\n", strerror(errno));
            } 
            //Create a thread for RIGHT partition
            threadAction = pthread_create(&rightPartition, NULL, (void*)quickSort, &second);
            checkRight = threadAction;
            if(threadAction != 0) {
                printf("%s\n", strerror(errno));
            }
            usedThreads++;
            //Unlock because usedThreads has caught up
            threadAction = pthread_mutex_unlock(&mutex);
            if(threadAction != 0) {
                printf("%s\n", strerror(errno));
            }
        }
        // Else don't create another thread (use the current one)
        else { quickSort(&second); } /* sort the right partition */

        /* Check if left or right threads were created */
        /* pthread_create returns 0 on success */
        if(checkLeft == 0) {
            threadAction = pthread_join(leftPartition, NULL);
            if(threadAction != 0) {
                printf("%s\n", strerror(errno));
            }
        }
        if(checkRight == 0) {
            threadAction = pthread_join(rightPartition, NULL);
            if(threadAction != 0) {
                printf("%s\n", strerror(errno));
            }
        }

    } else insertSort(array,i,j);           /* for a small range use insert sort */
}

/* user interface routine to set the number of threads sortT is permitted to use */
void setSortThreads(int count) {
    maximumThreads = count;
}

/* user callable sort procedure, sorts array of count strings, beginning at address array */
void sortThreaded(char** array, unsigned int count) {
    SortParams parameters;
    parameters.array = array; parameters.left = 0; parameters.right = count - 1;
    quickSort(&parameters);
}

