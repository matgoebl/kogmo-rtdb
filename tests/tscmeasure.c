//
//  tscmeasure.c - Genaue Zeitmessung mit dem Timestamp-Counter (TSC) auf i586-Systemen
//
//  (C) 2003-2006 by Matthias Goebl, Lehrstuhl fuer Realzeit-Computersysteme Technische Universitaet Muenchen (RCS-TUM)
//  matthias.goebl@rcs.ei.tum.de

//Test it with: gcc -o tscmeasure -DDEMO tscmeasure.c && ./tscmeasure

#include <stdlib.h>
#include <stdio.h>

typedef unsigned long long tscstamp_t;

tscstamp_t readtsc(void) {
  tscstamp_t tsc;
  volatile unsigned int tsc_lo,tsc_hi;
  // see linux-2.4/include/asm-i386/msr.h
  asm( "rdtsc": "=a" (tsc_lo), "=d" (tsc_hi) );
  tsc=((long long)tsc_hi<<32)+(long long)tsc_lo;
  return(tsc);
}

double cpu_MHz=0;
double tsc2seconds(tscstamp_t tsc) {
  double seconds;
  if(!cpu_MHz) {
   FILE *fp;
   char line[100];
   fp=fopen("/proc/cpuinfo","rb");
   if(fp==NULL) return(-1);
   while(fscanf(fp,"%99[^\n]\n",line)!=EOF) {
    sscanf(line,"cpu MHz : %lf",&cpu_MHz);
   }
   fclose(fp);
   if(!cpu_MHz) return(-1);
  }
  seconds=tsc/cpu_MHz/1000000;
  return(seconds);
}


static tscstamp_t tscmeasure_tscstart;
void tscmeasure_start(void) {
  tscmeasure_tscstart=readtsc();
}
static tscstamp_t tscmeasure_max=0;
tscstamp_t tscmeasure_stop(void) {
  tscstamp_t tscmeasure_tscstop;
  tscmeasure_tscstop=readtsc()-tscmeasure_tscstart;
  if(tscmeasure_tscstop>tscmeasure_max) tscmeasure_max=tscmeasure_tscstop;
  return tscmeasure_tscstop;
}
void tscmeasure_stop_print(void) {
  tscstamp_t tscmeasure_tscstop;
  tscmeasure_tscstop=readtsc();
  printf("Time is %.9f Seconds.\n",tsc2seconds(tscmeasure_tscstop-tscmeasure_tscstart));
}
void tscmeasure_print_max(void) {
  printf("Maximal time was %.9f Seconds.\n",tsc2seconds(tscmeasure_max));
  tscmeasure_max=0;
}
void tscmeasure_stop2(char *msg) {
  tscstamp_t tscmeasure_tscstop;
  tscmeasure_tscstop=readtsc();
  printf("Time for %s is %.3f ms.\n",msg,tsc2seconds(tscmeasure_tscstop-tscmeasure_tscstart)*1000.0);
  if(getenv("LOGTIMES")) fprintf(stderr,"%s\t%.3f\n",msg,tsc2seconds(tscmeasure_tscstop-tscmeasure_tscstart)*1000.0);
}

 
#ifdef DEMO
#include <unistd.h>
#include <sys/select.h>
main(){
  tscstamp_t start,stop,overhead,time_1s,time_1ms,time_1ns,time_0ns,time_1ms_select,time_printf;
  struct timeval tv;
  start=readtsc();
  stop=readtsc();

  start=readtsc();
  stop=readtsc();
  overhead=stop-start;

  start=readtsc();
  sleep(1);
  stop=readtsc();
  time_1s=stop-start;

  start=readtsc();
  usleep(1000);
  stop=readtsc();
  time_1ms=stop-start;

  start=readtsc();
  usleep(1);
  stop=readtsc();
  time_1ns=stop-start;

  start=readtsc();
  usleep(0);
  stop=readtsc();
  time_0ns=stop-start;

  start=readtsc();
  tv.tv_sec=0;
  tv.tv_usec=1;
  select(1, NULL, NULL, NULL, &tv);
  stop=readtsc();
  time_1ms_select=stop-start;

  start=readtsc();
  printf("hello world:-)\n");
  stop=readtsc();
  time_printf=stop-start;

  tsc2seconds(0); // init cpu_MHz
  printf("CPU Speed is %f Hz (/proc/cpuinfo)\n",cpu_MHz*1000000);
  printf("Time for call-overhead is %.6f milliseconds.\n",tsc2seconds(overhead)*1000);
  printf("Time for sleep(1) is %.6f milliseconds.\n",tsc2seconds(time_1s)*1000);
  printf("Time for usleep(1000) is %.6f milliseconds.\n",tsc2seconds(time_1ms)*1000);
  printf("Time for usleep(1) is %.6f milliseconds.\n",tsc2seconds(time_1ns)*1000);
  printf("Time for usleep(0) is %.6f milliseconds.\n",tsc2seconds(time_0ns)*1000);
  printf("Time for select(NULL..,1us) is %.6f milliseconds.\n",tsc2seconds(time_1ms_select)*1000);
  printf("Time for printf(hello\\n) is %.6f milliseconds.\n",tsc2seconds(time_printf)*1000);
}
#endif

#if 0
$ gcc -o tscmeasure -DDEMO tscmeasure.c && ./tscmeasure
hello world:-)
CPU Speed is 1797313000.000000 Hz (/proc/cpuinfo)
Time for call-overhead is 0.000015 milliseconds.
Time for sleep(1) is 1007.303082 milliseconds.
Time for usleep(1000) is 20.045577 milliseconds.
Time for usleep(1) is 19.950993 milliseconds.
Time for usleep(0) is 9.997457 milliseconds.
Time for select(NULL..,1us) is 10.006129 milliseconds.
Time for printf(hello\n) is 0.269148 milliseconds.
#endif
