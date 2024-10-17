#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include "a2_helper.h"
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

#define NUM_THREADS 4
#define NUM_THREADS_P3 5

sem_t sem_p3_threads_finished;
sem_t sem_thread2_ready;
sem_t sem_thread3_ready;
sem_t sem_thread9_12; 
sem_t sem_thread9_running; //thread limit

void* thread_function(void* arg) {
    intptr_t thread_no = (intptr_t)arg; 
//•	Thread T3 must start before T2 starts and 
//terminate after T2 terminates.
    if(thread_no==3){
        info(BEGIN, 2, 3);
        sem_post(&sem_thread3_ready);
    }
    else if(thread_no==2){
        sem_wait(&sem_thread3_ready);
        info(BEGIN, 2, 2);
        info(END, 2, 2);
        sem_post(&sem_thread2_ready);
    }
    else if(thread_no!=2 && thread_no!=3){
        info(BEGIN, 2, (int)thread_no);
        info(END, 2, (int)thread_no);
    }
    if(thread_no==3){
        sem_wait(&sem_thread2_ready);
        info(END, 2, 3);
    }
    
    return NULL;
}

sem_t sem_thread9_running_count;


void* thread_function_p9(void* arg) {
    //At any time, at most 4 threads of process P9 could be running
    // simultaneously, not counting the main thread.
	//Thread T9.12 can only end while 4 threads (including itself) are 
    //running.

   intptr_t thread_no = (intptr_t)arg; 

    sem_wait(&sem_thread9_running); 

    info(BEGIN, 9, (int)thread_no); 

    if (thread_no == 12) {
        // Thread T9.12 can only end while 4 threads (including itself) are running
        for (int i = 0; i < 4; i++) {
            sem_wait(&sem_thread9_12);
        }
    } else {
        sem_post(&sem_thread9_12); 
    }

    info(END, 9, (int)thread_no); 

    sem_post(&sem_thread9_running); 

    return NULL;
}


void* thread_function_p3(void* arg) {
    intptr_t thread_no = (intptr_t)arg;

    info(BEGIN, 3, (int)thread_no);

    info(END, 3, (int)thread_no);
    sem_post(&sem_p3_threads_finished);

    return NULL;
}

int main() {
    init();
    sem_init(&sem_thread2_ready, 0, 0);
    sem_init(&sem_thread3_ready, 0, 0);
    sem_init(&sem_thread9_12, 0, 0); 
    sem_init(&sem_thread9_running, 0, 4);
    sem_init(&sem_p3_threads_finished, 0, 0);
    sem_init(&sem_thread9_running_count, 0, 0);
    info(BEGIN, 1, 0); 

    pid_t pid2, pid6, pid7;
    if ((pid2 = fork()) == 0) {
        info(BEGIN, 2, 0); 
         pthread_t threads[NUM_THREADS];
        for (int i = 0; i < NUM_THREADS; ++i) {
            int* thread_arg = malloc(sizeof(int)); 
            *thread_arg = i + 1; 
            pthread_create(&threads[i], NULL, thread_function, (void*)(intptr_t)(i+1));

        }

        for (int i = 0; i < NUM_THREADS; ++i) {
            pthread_join(threads[i], NULL);
        }
        pid_t pid3, pid4;
        if ((pid3 = fork()) == 0) {
            info(BEGIN, 3, 0);
               pthread_t threads[NUM_THREADS_P3];
    for (int i = 0; i < NUM_THREADS_P3; ++i) {
        int* thread_arg = malloc(sizeof(int));
        *thread_arg = i;
        pthread_create(&threads[i], NULL, thread_function_p3, (void*)(intptr_t)(i+1));
    }

    
    for (int i = 0; i < NUM_THREADS_P3; ++i) {
        sem_wait(&sem_p3_threads_finished);
    }
            pid_t pid5;
            if ((pid5 = fork()) == 0) {
                info(BEGIN, 5, 0);
                info(END, 5, 0); 
                return 0;
            }
            waitpid(pid5, NULL, 0);
            info(END, 3, 0); 
            return 0;
        } else {
            if ((pid4 = fork()) == 0) {
                info(BEGIN, 4, 0);
                info(END, 4, 0); 
                return 0;
            }
            waitpid(pid3, NULL, 0);
            waitpid(pid4, NULL, 0);
        }
        info(END, 2, 0); 
        return 0;
    } else {
         waitpid(pid2, NULL, 0);
        if ((pid6 = fork()) == 0) {
            info(BEGIN, 6, 0); 
            info(END, 6, 0); 
            return 0;
        } else {
            if ((pid7 = fork()) == 0) {
                info(BEGIN, 7, 0); 
                pid_t pid8;
                if ((pid8 = fork()) == 0) {
                    info(BEGIN, 8, 0);
                    pid_t pid9;
                    if ((pid9 = fork()) == 0) {
                        info(BEGIN, 9, 0);
                        pthread_t threads[38];
                        for (int i = 0; i < 38; ++i) {
                            int* thread_arg = malloc(sizeof(int)); 
                            *thread_arg = i + 1; 
                            pthread_create(&threads[i], NULL, thread_function_p9, (void*)(intptr_t)(i+1));
                        }
                        for (int i = 0; i < 38; ++i) {
                            pthread_join(threads[i], NULL);
                        }
                        info(END, 9, 0); 
                        return 0;
                    }
                    waitpid(pid9, NULL, 0);
                    info(END, 8, 0); 
                    return 0;
                }
                waitpid(pid8, NULL, 0);
                info(END, 7, 0); 
                return 0;
            }
            waitpid(pid6, NULL, 0);
            waitpid(pid7, NULL, 0);
        }
        info(END, 1, 0); 
        return 0;
    }
    sem_destroy(&sem_thread2_ready);
    sem_destroy(&sem_thread3_ready);
    sem_destroy(&sem_thread9_12); 
    sem_destroy(&sem_thread9_running);
    sem_destroy(&sem_p3_threads_finished);
    sem_destroy(&sem_thread9_running_count);
    return 0;
}
