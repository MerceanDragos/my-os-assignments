#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include "a2_helper.h"
#include <semaphore.h>
#include <fcntl.h>

void *P8_thread_function ( void *arg ) {

    int ID = *( int * ) arg;

    switch ( ID ) {
        case 1: {
            info ( BEGIN, 8, 1 );

            info ( END, 8, 1 );

            return NULL;
        }
        case 2: {

            sem_t *semT2 = sem_open ( "/T8.2", 0 );
            sem_t *semT3 = sem_open ( "/T8.3", 0 );

            sem_wait ( semT2 );

            info ( BEGIN, 8, 2 );

            sem_post ( semT3 );

            sem_wait ( semT2 );

            info ( END, 8, 2 );

            sem_close ( semT2 );
            sem_close ( semT3 );

            return NULL;
        }
        case 3: {

            sem_t *semT2 = sem_open ( "/T8.2", 0 );
            sem_t *semT3 = sem_open ( "/T8.3", 0 );

            sem_wait ( semT3 );

            info ( BEGIN, 8, 3 );

            info ( END, 8, 3 );

            sem_post ( semT2 );

            sem_close ( semT2 );
            sem_close ( semT3 );

            return NULL;
        }
        case 4: {
            sem_t *semP3_P8;

            while ( ( semP3_P8 = sem_open ( "/P3_P8", 0 ) ) == SEM_FAILED );

            int sem_val;

            do {
                sem_getvalue ( semP3_P8, &sem_val );
            } while ( sem_val != 2 );

            info ( BEGIN, 8, 4 );

            info ( END, 8, 4 );

            sem_post ( semP3_P8 );

            sem_close ( semP3_P8 );

            return NULL;
        }
        default: {
            printf ( "Thread ID not recognized (P8)\n" );
            return NULL;
        }
    }
}

int P8 ( ) {

    info ( BEGIN, 8, 0 );

    sem_t *semT82 = sem_open ( "/T8.2", O_CREAT | O_EXCL, 0600, 1 );

    if ( semT82 == SEM_FAILED ) {
        printf ( "Semaphore for T8.2 couldn't be created\n" );
        exit ( 1 );
    }

    sem_t *semT83 = sem_open ( "/T8.3", O_CREAT | O_EXCL, 0600, 0 );

    if ( semT83 == SEM_FAILED ) {
        printf ( "Semaphore for T8.3 couldn't be created\n" );
        exit ( 1 );
    }

    sem_t *semP3_P8;

    while ( ( semP3_P8 = sem_open ( "/P3_P8", 0 ) ) == SEM_FAILED );

    pthread_t threads[4];
    int threadIDs[4] = { 1, 2, 3, 4 };

    for ( int i = 0; i < 4; i++ )
        pthread_create ( threads + i, NULL, P8_thread_function, &threadIDs[i] );

    for ( int i = 0; i < 4; i++ )
        pthread_join ( threads[i], NULL );

    sem_close ( semT82 );
    sem_unlink ( "/T8.2" );
    sem_close ( semT83 );
    sem_unlink ( "/T8.3" );
    sem_close ( semP3_P8 );

    info ( END, 8, 0 );
    exit ( 0 );
}

int P7 ( ) {

    info ( BEGIN, 7, 0 );

    info ( END, 7, 0 );
    exit ( 0 );
}

int P6 ( ) {

    info ( BEGIN, 6, 0 );

    info ( END, 6, 0 );
    exit ( 0 );
}

int P5 ( ) {


    info ( BEGIN, 5, 0 );

    pid_t P7_id;

    switch ( P7_id = fork ( ) ) {
        case -1: {
            printf ( "Cannot create child process P7\n" );
            exit ( 1 );
        }
        case 0: {
            P7 ( );
        }
        default: {

        }
    }

    waitpid ( P7_id, NULL, 0 );

    info ( END, 5, 0 );
    exit ( 0 );
}

sem_t *semP4;

int P4_no_running_threads = 0;

pthread_cond_t condP4_no_running_threads;
pthread_mutex_t mutexP4_no_running_threads;

int thread_11_is_running = 0;

pthread_cond_t condP4_thread_11_is_running;
pthread_mutex_t mutexP4_thread_11_is_running;

int thread_11_is_done = 0;

pthread_cond_t condP4_thread_11_is_done;
pthread_mutex_t mutexP4_thread_11_is_done;

void *P4_thread_function ( void *arg ) {

    int ID = *( int * ) arg;

    if ( ID == 11 ) {

        sem_wait ( semP4 );

        info ( BEGIN, 4, 11 );

        pthread_mutex_lock ( &mutexP4_no_running_threads );
        P4_no_running_threads++;
        pthread_mutex_unlock ( &mutexP4_no_running_threads );
        pthread_cond_broadcast ( &condP4_no_running_threads );

        pthread_mutex_lock ( &mutexP4_thread_11_is_running );
        thread_11_is_running = 1;
        pthread_mutex_unlock ( &mutexP4_thread_11_is_running );
        pthread_cond_broadcast ( &condP4_thread_11_is_running );

        pthread_mutex_lock ( &mutexP4_no_running_threads );
        while ( P4_no_running_threads < 4 )
            pthread_cond_wait ( &condP4_no_running_threads, &mutexP4_no_running_threads );
        pthread_mutex_unlock ( &mutexP4_no_running_threads );

        info ( END, 4, 11 );

        pthread_mutex_lock ( &mutexP4_thread_11_is_done );
        thread_11_is_done = 1;
        pthread_mutex_unlock ( &mutexP4_thread_11_is_done );
        pthread_cond_broadcast ( &condP4_thread_11_is_done );

        sem_post ( semP4 );
    }
    else {
        pthread_mutex_lock ( &mutexP4_thread_11_is_running );
        while ( thread_11_is_running == 0 )
            pthread_cond_wait ( &condP4_thread_11_is_running, &mutexP4_thread_11_is_running );
        pthread_mutex_unlock (  &mutexP4_thread_11_is_running );

        sem_wait ( semP4 );

        info ( BEGIN, 4, ID );

        pthread_mutex_lock ( &mutexP4_no_running_threads );
        P4_no_running_threads++;
        pthread_mutex_unlock ( &mutexP4_no_running_threads );
        pthread_cond_broadcast ( &condP4_no_running_threads );

        pthread_mutex_lock ( &mutexP4_thread_11_is_done );
        while ( thread_11_is_done == 0 )
            pthread_cond_wait ( &condP4_thread_11_is_done, &mutexP4_thread_11_is_done );
        pthread_mutex_unlock ( &mutexP4_thread_11_is_done );

        info ( END, 4, ID );

        sem_post ( semP4 );

    }

    return NULL;
}

int P4 ( ) {

    info ( BEGIN, 4, 0 );

    pid_t P6_id, P8_id;

    switch ( P6_id = fork ( ) ) {
        case -1: {
            printf ( "Cannot create child process P6\n" );
            exit ( 1 );
        }
        case 0: {
            P6 ( );
        }
        default: {
            switch ( P8_id = fork ( ) ) {
                case -1: {
                    printf ( "Cannot create child process P8\n" );
                    exit ( 1 );
                }
                case 0: {
                    P8 ( );
                }
                default : {

                }
            }
        }
    }

    pthread_t threads[47];
    int threadIDs[47];

    semP4 = sem_open ( "/semP4", O_CREAT | O_EXCL, 0600, 4 );
    if ( semP4 == SEM_FAILED ) {
        printf("Semaphore for P4 couldn't be created\n");
        exit ( 1 );
    }

    condP4_no_running_threads = ( pthread_cond_t ) PTHREAD_COND_INITIALIZER;
    mutexP4_no_running_threads = ( pthread_mutex_t ) PTHREAD_MUTEX_INITIALIZER;

    condP4_thread_11_is_running = ( pthread_cond_t ) PTHREAD_COND_INITIALIZER;
    mutexP4_thread_11_is_running = ( pthread_mutex_t ) PTHREAD_MUTEX_INITIALIZER;

    condP4_thread_11_is_done = ( pthread_cond_t ) PTHREAD_COND_INITIALIZER;
    mutexP4_thread_11_is_done = ( pthread_mutex_t ) PTHREAD_MUTEX_INITIALIZER;

    for ( int i = 0; i < 47; i++ ) {
        threadIDs[i] = i + 1;
        pthread_create ( threads + i, NULL, P4_thread_function, &threadIDs[i] );
    }

    for ( int i = 0; i < 47; i++ )
        pthread_join ( threads[i], NULL );

    sem_close ( semP4 );
    sem_unlink ( "/semP4" );
    pthread_cond_destroy ( &condP4_no_running_threads );
    pthread_mutex_destroy ( &mutexP4_no_running_threads );
    pthread_cond_destroy ( &condP4_thread_11_is_running );
    pthread_mutex_destroy ( &mutexP4_thread_11_is_running );
    pthread_cond_destroy ( &condP4_thread_11_is_done );
    pthread_mutex_destroy ( &mutexP4_thread_11_is_done );

    waitpid ( P6_id, NULL, 0 );
    waitpid ( P8_id, NULL, 0 );

    info ( END, 4, 0 );
    exit ( 0 );
}

void *P3_thread_function ( void *arg ) {

    int ID = *( int * ) arg;

    switch ( ID ) {
        case 1: {
            info ( BEGIN, 3, 1 );

            info ( END, 3, 1 );

            return NULL;
        }
        case 2: {
            sem_t *semP3_P8 = sem_open ( "/P3_P8", 0 );

            int sem_val;

            do {
                sem_getvalue ( semP3_P8, &sem_val );
            } while ( sem_val != 3 );

            info ( BEGIN, 3, 2 );

            info ( END, 3, 2 );

            sem_close ( semP3_P8 );

            return NULL;

        }
        case 3: {
            info ( BEGIN, 3, 3 );

            info ( END, 3, 3 );

            return NULL;

        }
        case 4: {
            sem_t *semP3_P8 = sem_open ( "/P3_P8", 0 );

            info ( BEGIN, 3, 4 );

            info ( END, 3, 4 );

            sem_post ( semP3_P8 );

            sem_close ( semP3_P8 );

            return NULL;
        }
        default: {
            printf ( "Thread ID not recognized (P3)\n" );
            return NULL;
        }
    }
}

int P3 ( ) {

    info ( BEGIN, 3, 0 );

    pid_t P5_id;

    switch ( P5_id = fork ( ) ) {
        case -1: {
            printf ( "Cannot create child process P5\n" );
            exit ( 1 );
        }
        case 0: {
            P5 ( );
        }
        default: {

        }
    }

    sem_t *semP3_P8 = sem_open ( "/P3_P8", O_CREAT | O_EXCL, 0600, 1 );

    if ( semP3_P8 == SEM_FAILED ) {
        printf ( "Semaphore for P3 and P8 couldn't be created\n" );
        exit ( 1 );
    }

    pthread_t threads[4];
    int threadIDs[4] = { 1, 2, 3, 4 };

    for ( int i = 0; i < 4; i++ )
        pthread_create ( threads + i, NULL, P3_thread_function, ( void * ) &threadIDs[i] );

    for ( int i = 0; i < 4; i++ )
        pthread_join ( threads[i], NULL );

    waitpid ( P5_id, NULL, 0 );

    sem_close ( semP3_P8 );
    sem_unlink ( "/P3_P8" );

    info ( END, 3, 0 );
    exit ( 0 );
}

int P2 ( ) {

    info ( BEGIN, 2, 0 );

    pid_t P4_id;

    switch ( P4_id = fork ( ) ) {
        case -1: {
            printf ( "Cannot create child process P4\n" );
            exit ( 1 );
        }
        case 0: {
            P4 ( );
        }
        default: {

        }
    }

    waitpid ( P4_id, NULL, 0 );

    info ( END, 2, 0 );
    exit ( 0 );
}

//int P1 ( )
int main ( ) {
    init ( );

    info ( BEGIN, 1, 0 );

    pid_t P2_id, P3_id;

    switch ( P2_id = fork ( ) ) {
        case -1: {
            printf ( "Cannot create child process P2\n" );
            exit ( 1 );
        }
        case 0: {
            P2 ( );
        }
        default : {

            switch ( P3_id = fork ( ) ) {
                case -1: {
                    printf ( "Cannot create child process P3\n" );
                    exit ( 1 );
                }
                case 0: {
                    P3 ( );
                }
                default: {

                }
            }
        }
    }

    waitpid ( P2_id, NULL, 0 );
    waitpid ( P3_id, NULL, 0 );

    info ( END, 1, 0 );
    return 0;
}
