#include <stdio.h>
#include <papi.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

#include "libs/tclap/CmdLine.h"

#define NUM_EVENTS 1

//returns number of lines in a rankfile
int get_rank_count()
{
   FILE *f = fopen("rankfile.0","r");
   char line[80];
   int ln_count=0;
   while(fgets(line,sizeof(line),f)!=NULL)
      ln_count++;
   fclose(f);
   return ln_count;
}
   
//returns number of rankfiles, assumes this number to be less than 1000 
int get_rankfile_count()
{
   DIR *d;
   struct dirent *fs;
   int fl_count=0;
   d=opendir(".");
   while(fs=readdir(d))
   {
      if(strncmp(fs->d_name,"rankfile.",9)==0 && strlen(fs->d_name)<13)
         fl_count++;
   } 
   closedir(d);
   return fl_count;
}

void kbest(int turn,int ranks,int k,int M,double E)
{
    int times[1000];
    char cmd[200]; char num[4];char fname[15];

    //prepare the mpi command to send to system	
    strcpy(cmd,"mpirun --rankfile rankfile.");
    sprintf(num,"%d",turn);
    strcpy(fname,"rankfile.");
    strcat(fname,num);
    strcat(fname,".temp");
    strcat(cmd,num); 
    strcat(cmd," --hostfile hostfile.txt -np ");
    sprintf(num,"%d",ranks);
    strcat(cmd,num);
    strcat(cmd," impl >/dev/null 2>&1");
    fprintf(stderr,"%s\n",cmd);
     
    //configure the PAPI counters to be monitored
    int Events[NUM_EVENTS]={PAPI_TOT_CYC}, EventSet=PAPI_NULL;
    long_long values[NUM_EVENTS];
    /* Initialize the Library */
    int retval = PAPI_library_init(PAPI_VER_CURRENT);
    /* Allocate space for the new eventset and do setup */
    retval = PAPI_create_eventset(&EventSet);
    /* Add Flops and total cycles to the eventset */
    retval = PAPI_add_events(EventSet,Events,NUM_EVENTS);

    //prepare the file to save the monitored events
    FILE *tf = fopen(fname,"w");
    char buf[100];
    int i,j;
    for(i=0;i<M;i++) times[i]=9999999;	//initialize executions times to big number

    for(i=0;i<M;i++)   //each iteration is one monitored run
    {   
	fprintf(stderr,"run:%d\n",i+1);      
        retval = PAPI_start(EventSet);
        system(cmd);	//code to be evaluated
        retval = PAPI_stop(EventSet,values);
        times[i]=values[0];
        sprintf(buf,"Run:%d\tExec cycles:%d\n",i,times[i]);
        fwrite(buf,sizeof(char),strlen(buf),tf);
       
       //place new run time to its location in sorted array
        int ii=i;
        for(j=i-1;j>=0;j--,ii--)
            if(times[ii]<times[j]) //swap
            {
                int t=times[ii];
                times[ii]=times[j];
                times[j]=t;
            }
        
       // for(j=0;j<=i;j++) printf("%d ",times[j]); printf("\n");
		//now we r sure execution times array is sorted
		//we can compare the best time to the kth best time 
	      if(((double)(times[k-1]-times[0])/times[0])<= E)
	      {
		 sprintf(buf,"Number of runs=%d\n",i+1);   
	         fwrite(buf,sizeof(char),strlen(buf),tf);
  	         sprintf(buf,"Fastest execution=%d cycles\n",times[0]);
		 fwrite(buf,sizeof(char),strlen(buf),tf);
                 sprintf(buf,"%dth fastest execution=%d cycles\n",k,times[k-1]);
		 fwrite(buf,sizeof(char),strlen(buf),tf);
	         sprintf(buf,"%dth fastest = %f of the fastest.\n",k,1+((double)(times[k-1]-times[0])/times[0]));
	         fwrite(buf,sizeof(char),strlen(buf),tf);
	         break;
              }
    }

    if(i==M) //in case we did the maximum number of runs
    {
        sprintf(buf,"Could not converge\n");
        fwrite(buf,sizeof(char),strlen(buf),tf);
  	sprintf(buf,"Fastest execution=%d cycles\n",times[0]);
	fwrite(buf,sizeof(char),strlen(buf),tf);
    }
    fclose(tf);
}


main(int argc,char* argv[])
{
   int k=atoi(argv[1]),M=atoi(argv[2]);
   double E=atof(argv[3]);
   //int k=3,M=10;double E=0.2;
   int ranks=get_rank_count();
   int fl_count=get_rankfile_count();
   //loop on the rank files, for each iteration, k-best scheme is applied
   int i;
   for(i=0;i<fl_count;i++)
   {
      fprintf(stderr,"Running rankfile.%d\n",i);
      kbest(i,ranks,k,M,E);
   } 
}
