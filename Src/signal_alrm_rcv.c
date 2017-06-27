#include "stab_rcv.h"

extern void compute_stats();

extern void sig_alrm(int signo);

/* from Figure 5.6, UNIX network programming */

Sigfunc *Signal(int signo, Sigfunc *func)
{
  struct sigaction act, oact;

  act.sa_handler= func;
  sigemptyset(&act.sa_mask);
  act.sa_flags=0;
 
#ifdef SA_RESTART
    act.sa_flags |= SA_RESTART; /* SVR4, 4.4BSD */
#endif
  

  if (sigaction(signo, &act, &oact) <0)
    return (SIG_ERR);
  return (oact.sa_handler);

}
/* If received enough chirps, send OK packet to sender */
void send_ok()
{

  if(last_chirp>next_ok_due)
    {
      if (debug)
	fprintf(stderr,"Sending OK packet\n");

      send_pkt(RECV_OK,0);
      next_ok_due=last_chirp+MAX_RECV_OK_COUNT/5;
    }
  return;
}

/* when alarm goes off, perform bandwidth estimation */

void sig_alrm(int signo)
{

  struct timeval tp_stop;
  int count;

  (void) gettimeofday (&tp_stop, (struct timezone *) 0);

  if ((double)tp_stop.tv_sec+(((double)tp_stop.tv_usec)/1000000.0)>stop_time)
    {
      request_num++;
      send_pkt(STOP,0);
    }
  if (debug)
  fprintf(stderr,"num_pkts_in_info=%d\n",num_pkts_in_info);
 
  if (num_pkts_in_info>0){
    no_chirps_recd=0;
  if (debug)
  fprintf(stderr,"before compute stats\n");

    compute_stats();

  if (debug)
    fprintf(stderr,"Finished compute stats\n");
  /*don't increment request_number for RECV_OK packets*/
    send_ok();/*send packet saying recv is ok*/
    num_pkts_in_info=0;/*reset variables*/
    
    
    /*if previous update has not been made, resend*/
    if(sender_request_num<request_num)
      {
      send_pkt(UPDATE_RATES,last_ttl_update);
 if (debug)
   fprintf(stderr,"sender_request_num too low,send=%d,rec=%d\n",sender_request_num,request_num);
      }
    else/* if last update successful, check for new ones*/
      {
	if (debug)
	  fprintf(stderr,"sender_request_num ok\n");
	for(count=0;count<num_ttl;count++){
	  if (check_for_new_pars(count))
	    {
	      request_num++;
	      send_pkt(UPDATE_RATES,count);
	      last_ttl_update=count;
	    }
	}/*end for*/
      }
  }
  else{
    no_chirps_recd++;
    if (no_chirps_recd>4)
      {
	fprintf(stderr,"\nNot receiving packets from sender, aborting\n");
	close_all();
	exit(0);
      }
    
  }

  /* this timer is set just in case the sender goes down and we are not
     stuck forever waiting*/
  setitimer(ITIMER_REAL,&timeout,0);
  return;
  
}


/*returns minimum timer granularity in secs*/
double timer_gran()
{
/* Find out what is the system clock granularity.  */

  struct itimerval tv;

  tv.it_interval.tv_sec = 0;
  tv.it_interval.tv_usec = 1;
  tv.it_value.tv_sec = 0;
  tv.it_value.tv_usec = 0;
  setitimer (ITIMER_REAL, &tv, 0);
  setitimer (ITIMER_REAL, 0, &tv);
  if(debug) fprintf(stderr,"min timer gran=%f sec\n",(double)tv.it_interval.tv_sec+((double)tv.it_interval.tv_usec)/1000000.0);
  return((double)tv.it_interval.tv_sec+((double)tv.it_interval.tv_usec)/1000000.0);

}
