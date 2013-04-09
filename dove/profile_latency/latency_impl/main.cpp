/******************************************************************************
 * * FILE: mpi_latency.c
 * * DESCRIPTION:  
 * *   MPI Latency Timing Program - C Version
 * *   In this example code, a MPI communication timing test is performed.
 * *   MPI task 0 will send "reps" number of 1 byte messages to MPI task 1,
 * *   waiting for a reply between each rep. Before and after timings are made 
 * *   for each rep and an average calculated when completed.
 * * AUTHOR: Blaise Barney
 * * LAST REVISED: 04/13/05
 * ******************************************************************************/
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
// On Node b4 and b5, some experiments: 
// 1000              600 seconds to complete
// 1000*10*10        750 seconds
// 1000*100*100
#define	NUMBER_REPS	10000000

#ifdef DEBUG_LATENCY
 #define debug true
#else
 #define debug false
#endif

// TODO I'm not sure if there is such a thing as warming up the 
// network when doing core-core routing, but if there is then we
// are finding the average of 1000s of iterations, which effectively
// removes any warmup effects. When doing an actual system, the
// routing delay that's important is not the average delay, but the 
// delay when routes are random 
int main (int argc, char *argv[])
{
int reps,                   /* number of samples per test */
    tag,                    /* MPI message tag parameter */
    numtasks,               /* number of MPI tasks */
    rank,                   /* my MPI task number */
    dest, source,           /* send/receive task designators */
    avgT,                   /* average time per rep in microseconds */
    rc,                     /* return code */
    n;
double T1, T2,              /* start/end times per rep */
    sumT,                   /* sum of all reps times */
    deltaT;                 /* time for one rep */
char msg;                   /* buffer containing 1 byte message */
MPI_Status status;          /* MPI receive routine parameter */

MPI_Init(&argc,&argv);
MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
MPI_Comm_rank(MPI_COMM_WORLD,&rank);
if (rank == 0 && numtasks != 2) {
   printf("Number of tasks = %d\n",numtasks);
   printf("Only need 2 tasks - extra will be ignored...\n");
   }
MPI_Barrier(MPI_COMM_WORLD);

sumT = 0;
msg = 'x';
tag = 1;
reps = NUMBER_REPS;

if (rank == 0) {
   /* round-trip latency timing test */
   if (debug) printf("task %d has started...\n", rank);
   if (debug) printf("Beginning latency timing test. Number of reps = %d.\n", reps);
   if (debug) printf("***************************************************\n");
   if (debug) printf("Rep#       T1               T2            deltaT\n");
   dest = 1;
   source = 1;
   for (n = 1; n <= reps; n++) {
      T1 = MPI_Wtime();     /* start time */
      /* send message to worker - message tag set to 1.  */
      /* If return code indicates error quit */
      rc = MPI_Send(&msg, 1, MPI_BYTE, dest, tag, MPI_COMM_WORLD);
      if (rc != MPI_SUCCESS) {
         printf("Send error in task 0!\n");
         MPI_Abort(MPI_COMM_WORLD, rc);
         exit(1);
         }
      /* Now wait to receive the echo reply from the worker  */
      /* If return code indicates error quit */
      rc = MPI_Recv(&msg, 1, MPI_BYTE, source, tag, MPI_COMM_WORLD, 
                    &status);
      if (rc != MPI_SUCCESS) {
         printf("Receive error in task 0!\n");
         MPI_Abort(MPI_COMM_WORLD, rc);
         exit(1);
         }
      T2 = MPI_Wtime();     /* end time */

      /* calculate round trip time and print */
      deltaT = T2 - T1;
      if (debug) 
        printf("%4d  %8.8f  %8.8f  %2.8f\n", n, T1, T2, deltaT);
      
      sumT += deltaT;
      }
   // TODO while all T's used are in seconds, it's unclear the resolution of MPI_Wtime
   // and therefore we should be using MPI_wTick as well to determine how many 
   // iterations should be run
   avgT = (sumT*1000000000)/reps;
   if (debug) printf("***************************************************\n");
   if (debug) printf("\n*** Avg round trip time = %d nanoseconds\n", avgT);
   if (debug) printf("*** Avg one way latency = %d nanoseconds\n", avgT/2);
   printf("%d", avgT);
   } 

else if (rank == 1) {
   if (debug) printf("task %d has started...\n", rank);
   dest = 0;
   source = 0;
   for (n = 1; n <= reps; n++) {
      rc = MPI_Recv(&msg, 1, MPI_BYTE, source, tag, MPI_COMM_WORLD, 
                    &status);
      if (rc != MPI_SUCCESS) {
         printf("Receive error in task 1!\n");
         MPI_Abort(MPI_COMM_WORLD, rc);
         exit(1);
         }
      rc = MPI_Send(&msg, 1, MPI_BYTE, dest, tag, MPI_COMM_WORLD);
      if (rc != MPI_SUCCESS) {
         printf("Send error in task 1!\n");
         MPI_Abort(MPI_COMM_WORLD, rc);
         exit(1);
         }
      }
   }

MPI_Finalize();
exit(0);
}


