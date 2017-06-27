#include "stab_rcv.h"

extern void check_reorder_loss();


double exp_smooth_fac=0.99;/* used in estimating probability of a link being thin */
double alpha=1.2; /* factor used for thin link prob */
double plot_time=0.0;/* last time the plot was updated */

int enough_data_for_plot=0;/*flag to say we have enough data for a plot*/
int int_coal_detected=0;/*flag to say interrupt coalescence detected*/

/* absolute value */
static double my_abs(double a)
{
  if (a<0)
    a=-1.0*a;
  
  return a;
  
}

/* Newton's method*/
static double my_sqrt(double a)
{
  double x,y;
  int i=0;
  x=a;

  if (a<0)
    {
      x=0.0;
      return x;
    }

  while( (x*x/(a)>1.001 || x*x/(a)<0.999) && (i<10))
    {
      y=x/2 + (a/(2*x));
      x=y;
      i++;
    }
  return x;
}


/* smooths bandwidth estimates and writes to to file */
void write_instant_bw(double inst_av_bw_excursion,double time_stamp,int local_ttl_index)
{
  /*  double inst_median=0.0;*/
  double inst_mean=0.0;
  int counter=0,sum=0;

  if (local_ttl_index>=num_ttl || local_ttl_index<0 )
    {
      fprintf(stderr,"write_inst_bw: local_ttl_index invalid: %d\n",local_ttl_index);
      exit(0);
    }

   refresh_plot();

  
  if (debug) 
    fprintf(stderr,"in write, ab=%f,time=%f,ttl_ind=%d\n",inst_av_bw_excursion,time_stamp,local_ttl_index);

  inst_av_bw_excursion=inst_av_bw_excursion/1000000.0;

  /*taking latest value and removing old one*/

 total_inst_bw_excursion[local_ttl_index]+=inst_av_bw_excursion;
 total_inst_bw_excursion[local_ttl_index]-=inst_bw_estimates_excursion[local_ttl_index][inst_head[local_ttl_index]];

 /*squared value*/
 total_inst_bw_excursion_sq[local_ttl_index]+=inst_av_bw_excursion*inst_av_bw_excursion;
 total_inst_bw_excursion_sq[local_ttl_index]-=inst_bw_estimates_excursion[local_ttl_index][inst_head[local_ttl_index]]*inst_bw_estimates_excursion[local_ttl_index][inst_head[local_ttl_index]];


 if (debug) 
   fprintf(stderr,"total_inst_bw_excursion[local_ttl_index]=%f\n",total_inst_bw_excursion[local_ttl_index]);

 inst_bw_estimates_excursion[local_ttl_index][inst_head[local_ttl_index]]=inst_av_bw_excursion;

 inst_head[local_ttl_index]++;
 /*circular buffer*/
 if (inst_head[local_ttl_index]>=num_inst_bw)
   inst_head[local_ttl_index]=0;

  if (debug) 
    fprintf(stderr,"num_ttl=%d,inst_head=%d\n",num_ttl,inst_head[local_ttl_index]);

  if (inst_bw_count[local_ttl_index]>=num_inst_bw)
    {
      /*mark that we have num_inst_bw chirps for this ttl*/
      got_num_inst_bw_chirps[local_ttl_index]=1;
      inst_mean=total_inst_bw_excursion[local_ttl_index]/((double) num_inst_bw);
      cur_avail_bw[local_ttl_index]=inst_mean;
      
      if(enough_data_for_plot==1)
	{
	  /*compute probability of thin link as exponentially weighted moving average*/
	  for (counter=0;counter<num_ttl;counter++)
	    {
	      if(counter>0) 
		{
		  if(cur_avail_bw[counter]*alpha<cur_avail_bw[counter-1])
		    cur_prob_thin[counter]=exp_smooth_fac*cur_prob_thin[counter]+(1-exp_smooth_fac);
		  else
		    cur_prob_thin[counter]=exp_smooth_fac*cur_prob_thin[counter];
		}
	      else
		cur_prob_thin[counter]=1;/*first link is always thin*/
	    }
	  
	}

     /* draw plot if necessary */
     if (time_stamp-plot_time>PLOT_TIME_GAP)
       {
	 if(enough_data_for_plot==0){
	   sum=0;
	   for (counter=0;counter<num_ttl;counter++)
	     sum+=got_num_inst_bw_chirps[counter];
	   if (sum==num_ttl)
	     enough_data_for_plot=1;
	 }
	 else	     /*compute conf int, prob of being thin*/
	   {
	     for (counter=0;counter<num_ttl;counter++)
	       {
		 conf_int[counter]=(2.0/my_sqrt((double)num_inst_bw))*my_sqrt((total_inst_bw_excursion_sq[counter]/(double) num_inst_bw)- (total_inst_bw_excursion[counter]*total_inst_bw_excursion[counter]/((double) num_inst_bw*num_inst_bw)));
	       }
	     clean_plot();
	     stem_plot(hops_plot,cur_prob_thin,num_ttl,1);
	    
	     make_plot(hops_plot,cur_avail_bw,conf_int,num_ttl,0);
	     

	     plot_time=time_stamp+PLOT_TIME_GAP;
	   }/*enough data*/
       }
       /*if (timestamp-plot_time ....*/
     
     /* if estimates are close to either end of chirp probing rates, change
	rates */
     if (inst_mean<1.5*low_rate[local_ttl_index])
       lowcount[local_ttl_index]++;
     else 
       lowcount[local_ttl_index]=0;
     
     if (inst_mean>0.66*high_rate[local_ttl_index])
       highcount[local_ttl_index]++;
     else 
       highcount[local_ttl_index]=0;
     
     /* write to file */
     fprintf(fd_instbw,"%f %f %d %f\n",time_stamp,inst_mean,(local_ttl_index+1)*ttl_gap,cur_prob_thin[local_ttl_index]);
     
     fflush(fd_instbw);
   }
 else 
   inst_bw_count[local_ttl_index]++;
 
 
}

/* compute the instantaneous bandwidth during this chirp, new algorithm */

double compute_inst_bw_excursion(int local_ttl_index)
{
  int cur_loc=0;/* current location in chirp */
  int cur_inc=0;/* current location where queuing delay increases */
  int count;
  double inst_bw=0.0,sum_iat=0.0,max_q=0.0;

  if (local_ttl_index>=num_ttl || local_ttl_index<0 )
    {
      fprintf(stderr,"write_inst_bw: local_ttl_index invalid: %d\n",local_ttl_index);
      exit(0);
    }


  /* set all av_bw_per_pkt to zero */
  memset(av_bw_per_pkt[local_ttl_index], 0, (int)(num_interarrival[local_ttl_index]) * sizeof(double));
    
  /*find first place with an increase in queuing delay*/
  while(qing_delay[local_ttl_index][cur_inc]>=qing_delay[local_ttl_index][cur_inc+1] && cur_inc<max_good_pkt_this_chirp)
    cur_inc++;

  cur_loc=cur_inc+1; 
  
  /* go through all delays */

  /* Note: in case of interrupt coalescence we consider only 
     that part of the chirp unaffected by it. This should not be
     an issue on low speed links (less than Gigabit). It should also not
be an issue on Gigabit links if you use large packets (say 8KB). */
  if(debug) 
    fprintf(stderr,"\nnum_pkts=%d,max_good_pkt=%d\n",num_interarrival[local_ttl_index]+1,max_good_pkt_this_chirp);

  /*    fprintf(stdout,"%d %d -2 -2\n",num_interarrival[local_ttl_index]+1,max_good_pkt_this_chirp);*/
  
  while(cur_loc<=max_good_pkt_this_chirp)
    {
      
      /* check if queuing delay has decreased to "almost" what it was at cur_inc*/
      if (qing_delay[local_ttl_index][cur_loc]>NEG_THRESH+1.0)/* marked as negative in case of coalescence, skip this packet*/
	{

      /* find maximum queuing delay between cur_inc and cur_loc*/
      if (max_q<(qing_delay[local_ttl_index][cur_loc]-qing_delay[local_ttl_index][cur_inc]))
	max_q=qing_delay[local_ttl_index][cur_loc]-qing_delay[local_ttl_index][cur_inc];

	  if (qing_delay[local_ttl_index][cur_loc]-qing_delay[local_ttl_index][cur_inc]<(max_q/decrease_factor))
	    {
	      if (cur_loc-cur_inc>=busy_period_thresh)
		{
		  for (count=cur_inc;count<=cur_loc-1;count++)
		    {
		      /*mark all increase regions between cur_inc and cur_loc as having
			available bandwidth equal to their rates*/
		      if (qing_delay[local_ttl_index][count]<qing_delay[local_ttl_index][count+1])
			av_bw_per_pkt[local_ttl_index][count]=rates[local_ttl_index][count];
		    }
		}
	      /* find next increase point */
	      cur_inc=cur_loc;

	      while(qing_delay[local_ttl_index][cur_inc]>=qing_delay[local_ttl_index][cur_inc+1] && cur_inc<max_good_pkt_this_chirp)
		cur_inc++;
	      cur_loc=cur_inc;
	      /* reset max_q*/
	      max_q=0.0;
	    }
	}
      cur_loc++;      
    }

  if(cur_inc==max_good_pkt_this_chirp)
    cur_inc--;
  
    /*mark the available bandwidth during the last excursion as the rate at its beginning*/

  for(count=cur_inc;count<max_good_pkt_this_chirp;count++)
    {
      av_bw_per_pkt[local_ttl_index][count]=rates[local_ttl_index][cur_inc];
    }
  
  /*  fprintf(stdout,"%d %f -3 -3\n",cur_inc,rates[local_ttl_index][cur_inc]);*/
  
  /* all unmarked locations are assumed to have available 
     bandwidth described by the last excursion */
  
  for(count=0;count<max_good_pkt_this_chirp;count++)
    {
      if (av_bw_per_pkt[local_ttl_index][count]<1.0)
	av_bw_per_pkt[local_ttl_index][count]=rates[local_ttl_index][cur_inc];
      sum_iat+=iat[local_ttl_index][count];
      inst_bw+=av_bw_per_pkt[local_ttl_index][count]*iat[local_ttl_index][count];
    }

  inst_bw=inst_bw/sum_iat;
  return(inst_bw);
  
}


/* compute queuing delay, packet drops, and available bandwidth estimates*/
void compute_stats()
{ 
  int count;
  int local_ttl_index;
  double inst_bw_excursion=0.0;
  long nc,np;
  int prev_pkt=0,num_bad_chirps=0;
  int num_consec_bad=0;/* number of consecutive chirp interarrivals affected by 
			  interrupt coalescence and context switching */
  double snd_distortion=0.0;/*measures the distortion of interarrivals at the sender*/

  if (debug)
    fprintf(stderr,"\nCompute Stats\n");

    check_reorder_loss();
  
  for (count=0;count<num_pkts_in_info;count++)
    {
      nc=packet_info[count].chirp_num;
      np=packet_info[count].num;
      local_ttl_index=packet_info[count].ttl/ttl_gap - 1;

      /*skip packet if chirp_info overflow*/
      if ((nc-first_chirp)>=(2*chirps_per_write))
	{
	  continue;
	}

      if(local_ttl_index>=num_ttl || local_ttl_index<0)
	{
	  fprintf(stderr,"compute_parameters: ttl_index too high/low %d (num_ttl=%d)\n",local_ttl_index,num_ttl);
	  exit(0);
	}

      if (debug) 
	fprintf(stderr,"pktttl=%d,ttl index=%d\n",packet_info[count].ttl,local_ttl_index);

      /*if packet reorder */
      if (chirp_info[nc-first_chirp].reorder==2 || packet_info[count].context_switch==2){
	if (debug)
	  fprintf(stderr,"reorder=%d,context=%d\n",chirp_info[nc-first_chirp].reorder,packet_info[count].context_switch);
	continue;
	
      }
      else /*no reorder*/
	{
	 if (debug)
	  fprintf(stderr,"No reorder\n");
	  /* if loss */
	  if (chirp_info[nc-first_chirp].num<num_interarrival[local_ttl_index]+1)
	    {
	 if (debug)
	      fprintf(stderr,"Loss in chirp,rcv=%ld,expect=%d\n",chirp_info[nc-first_chirp].num,num_interarrival[local_ttl_index]+1);
	      
	      /*if more than 1 chirp*/
	      if (last_chirp>first_chirp)
		{
		  if (debug)
		    fprintf(stderr,"lastchirp  greater first\n");
		  
		  if (nc>first_chirp && nc<last_chirp && np==chirp_info[nc-first_chirp].last_pkt)/*if not first or last chirp*/
		    num_bad_chirps++;
		}
	      else
		{
		  if (np==chirp_info[nc-first_chirp].last_pkt)
		    write_instant_bw(low_rate[local_ttl_index]*1000000.0/2.0,packet_info[count].rcv_time,local_ttl_index);
		}
	      continue;
	    }
	  else /* if no loss  */
	    {
	      if (debug)
		fprintf(stderr,"No Loss\n");
	      qing_delay[local_ttl_index][np-1]=packet_info[count].rcv_time-packet_info[chirp_info[nc-first_chirp].first_pkt_loc].rcv_time-packet_info[count].snd_time+packet_info[chirp_info[nc-first_chirp].first_pkt_loc].snd_time;
	      
	      /* fprintf(stdout,"%f %f %ld %d\n",packet_info[count].snd_time,packet_info[count].rcv_time,np,local_ttl_index+1);*/

	      /*context switching/interrupt coalescence*/
	      if (np>1)
		{
		  if (debug)
		    fprintf(stderr,"packet_info[count].good_jumbo_pkt=%d, max_good_pkt_this_chirp=%d,num_interarrival[local_ttl_index]=%d \n",packet_info[count].good_jumbo_pkt,max_good_pkt_this_chirp,num_interarrival[local_ttl_index]);

		  /* used only for jumbo packets*/
		  if (packet_info[count].good_jumbo_pkt==0 && max_good_pkt_this_chirp==num_interarrival[local_ttl_index])
		    {
		      max_good_pkt_this_chirp=np-1;
		    }


		  
		  /* check if  packets were sent out with the expected interarrival time, if jumbo==1 */
		  if (jumbo==1)
		    snd_distortion+=my_abs(((packet_info[count].snd_time-packet_info[prev_pkt].snd_time)/iat[local_ttl_index][np-2]) - 1);


		  if(packet_info[count].rcv_time-packet_info[prev_pkt].rcv_time<context_receive_thresh)
		    {
		      /*		      fprintf(stderr,"np=%ld,time diff=%f\n",np,packet_info[count].rcv_time-packet_info[prev_pkt].rcv_time);*/
		      num_consec_bad++;

		      qing_delay[local_ttl_index][np-1]=NEG_THRESH;/*marked as negative*/

		      /* if CONSEC_BAD_PKTS consecutive interarrivals are 
			 influenced by coalescence */
		      if (num_consec_bad>=CONSEC_BAD_PKTS && max_good_pkt_this_chirp==num_interarrival[local_ttl_index])
			{
			  if (int_coal_detected==0){

			    fprintf(stderr,"\n Interrupt Coalescence detected. Use the '-J' option at the\n receiver for improved performance. Example: '-J 4'.\n STAB will take longer to generate results however.\n");
			    int_coal_detected=1;
			  }
			    
			  max_good_pkt_this_chirp=np-CONSEC_BAD_PKTS-1;
			  if (debug) fprintf(stderr,"max good pkt=%d\n",max_good_pkt_this_chirp);
			  if (max_good_pkt_this_chirp<2) 
			    max_good_pkt_this_chirp=2;
			}
		    }
		  else {
		    num_consec_bad=0;
		  }
		}
		  else /*np=1*/
		{
		  snd_distortion=0.0;
		    max_good_pkt_this_chirp=num_interarrival[local_ttl_index];
		}

	      prev_pkt=count;/*location of previous packet in this "good" chirp*/

	      /* if last chirp packet then compute available bandwidth */
	      if (np==(num_interarrival[local_ttl_index]+1))
		{
		  if((snd_distortion/(double)num_interarrival[local_ttl_index])>0.2)
		    fprintf(stderr,"\n Sender context switching detected, excluding chirp %ld\n",nc);
		  else{
		  inst_bw_excursion=compute_inst_bw_excursion(local_ttl_index);
		 
		  write_instant_bw(inst_bw_excursion,packet_info[count].rcv_time,local_ttl_index);
		  }
		  snd_distortion=0.0;

		}
	    }/*no loss*/
	}/*no reorder*/
    }/*for loop*/
}

/* if estimates too low or too high update probing range */
int check_for_new_pars(int local_ttl_index)
{
  int resetflag=0;
  double min_low_rate=10000.0;
  int count;

  if (debug) 
    fprintf(stderr,"in check_for_new_pars,low=%d,high=%d\n",lowcount[local_ttl_index],highcount[local_ttl_index]);


  if (lowcount[local_ttl_index]>2)
    {

      if (low_rate[local_ttl_index]<=1.0) /*less than 1Mbps, have smaller range of rates, that is fewer packets per chirp. This allows us to send more chirps per unit time.*/
	{
	  high_rate[local_ttl_index]=low_rate[local_ttl_index]*4;
	  low_rate[local_ttl_index]=low_rate[local_ttl_index]/4;
	}
      else{
	  high_rate[local_ttl_index]=low_rate[local_ttl_index]*7;
	  low_rate[local_ttl_index]=low_rate[local_ttl_index]/7;
	}

      resetflag=1;
      lowcount[local_ttl_index]=0;
    }

  if (highcount[local_ttl_index]>2 && high_rate[local_ttl_index]<0.9*(double)MAX_HIGH_RATE)
    {

      if (debug) 
	fprintf(stderr,"increasing rates\n");

      if (high_rate[local_ttl_index]>1.0) 
	{
	  low_rate[local_ttl_index]=high_rate[local_ttl_index]/7;
	  high_rate[local_ttl_index]=high_rate[local_ttl_index]*7;
	}
      else{
	  low_rate[local_ttl_index]=high_rate[local_ttl_index]/4;
	  high_rate[local_ttl_index]=high_rate[local_ttl_index]*4;
	}
      resetflag=1;
      highcount[local_ttl_index]=0;
    }
  else 
    if (highcount[local_ttl_index]>2)
      highcount[local_ttl_index]=0;

  if (resetflag)
    {
      for (count=0;count<num_ttl;count++)
	{
	  if(min_low_rate>low_rate[count])
	    min_low_rate=low_rate[count];
	}

      /* make sure average rate is not too high  */
      if (avg_rate>min_low_rate/5)
	avg_rate=min_low_rate/5;
      else
	if (avg_rate<min_low_rate/30)
	  {
	    avg_rate=min_low_rate/30;
	  }
      /*make sure avg_rate is less than MAX_AVG_RATE*/
      if(avg_rate>MAX_AVG_RATE)
	avg_rate=MAX_AVG_RATE;

      /*make sure avg_rate is more than MIN_AVG_RATE*/

      if(avg_rate<MIN_AVG_RATE)
	avg_rate=MIN_AVG_RATE;

      compute_parameters(local_ttl_index);
      update_rates_iat(local_ttl_index);
    }

  return(resetflag);

}
