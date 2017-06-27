/* FILE: alloc_rcv.c */

#include "stab_rcv.h"

/* create arrays */


void create_arrays()
{
  int count;
 /* The first part of the data packet can be accessed
     as the log record */
  if (debug) fprintf(stderr,"in created arrays\n");
  
  inst_bw_estimates_excursion=(double **)calloc((int)(num_ttl),sizeof(double *));

  qing_delay=(double **)calloc((int)(num_ttl),sizeof(double *));

  qing_delay_cumsum=(double **)calloc((int)(num_ttl),sizeof(double *));

  rates=(double **)calloc((int)(num_ttl),sizeof(double *));
  iat=(double **)calloc((int)(num_ttl),sizeof(double *));

  av_bw_per_pkt=(double **)calloc((int)(num_ttl),sizeof(double *));
  got_num_inst_bw_chirps=(int *)calloc((int)(num_ttl),sizeof(int));
  hops_plot=(double *)calloc((int)(num_ttl),sizeof(double));
  cur_avail_bw=(double *)calloc((int)(num_ttl),sizeof(double));
  cur_prob_thin=(double *)calloc((int)(num_ttl),sizeof(double));
  conf_int=(double *)calloc((int)(num_ttl),sizeof(double));

  low_rate=(double *)calloc((int)(num_ttl),sizeof(double));
  high_rate=(double *)calloc((int)(num_ttl),sizeof(double));
  total_inst_bw_excursion=(double *)calloc((int)(num_ttl),sizeof(double));
  total_inst_bw_excursion_sq=(double *)calloc((int)(num_ttl),sizeof(double));

  lowcount=(int *)calloc((int)(num_ttl),sizeof(int));
  highcount=(int *)calloc((int)(num_ttl),sizeof(int));
  num_interarrival=(int *)calloc((int)(num_ttl),sizeof(int));
  inst_head=(int *)calloc((int)(num_ttl),sizeof(int));
  inst_bw_count=(int *)calloc((int)(num_ttl),sizeof(int));

  for (count=0;count<num_ttl;count++)
    {
      hops_plot[count]=(count+1)*ttl_gap;
      low_rate[count]=spec_low_rate;
      high_rate[count]=spec_high_rate;

      inst_bw_estimates_excursion[count]=(double *)calloc((int)(num_inst_bw),sizeof(double));
      
      qing_delay[count]=(double *)calloc((int)(MAXCHIRPSIZE-1+1),sizeof(double));
      
      qing_delay_cumsum[count]=(double *)calloc((int)(MAXCHIRPSIZE-1+1),sizeof(double));
      
      rates[count]=(double *)calloc((int)(MAXCHIRPSIZE-1),sizeof(double));
      iat[count]=(double *)calloc((int)(MAXCHIRPSIZE-1),sizeof(double));
      
      av_bw_per_pkt[count]=(double *)calloc((int)(MAXCHIRPSIZE-1),sizeof(double));

    }
  for (count=0;count<num_ttl;count++)
    {
  if (debug) fprintf(stderr,"bef comp pars\n");
      compute_parameters(count);
  if (debug) fprintf(stderr,"update rates\n");
      update_rates_iat(count);
    }
  /*allocating enough memory to store packet information */

  chirps_per_write=(int)(2*(1+(write_interval/(inter_chirp_time))));

  pkts_per_write=(int)((MAXCHIRPSIZE)*chirps_per_write);

  if (debug)
    fprintf(stderr,"create arrays: chirps_per_write=%d, pkts_per_write=%d\n",chirps_per_write,pkts_per_write);

  packet_info=(struct pkt_info *)calloc((int)pkts_per_write*2,sizeof(struct pkt_info));

  chirp_info=(struct chirprecord *)calloc((int)((double)chirps_per_write*2.0),sizeof(struct chirprecord));


 created_arrays=1;

}

/* update the chirp bit rates and interarrival times*/
void update_rates_iat(int local_ttl_index)
{
  int count;

  if (debug)
  fprintf(stderr,"low rate=%f, high_rate=%f\n",low_rate[local_ttl_index],high_rate[local_ttl_index]);

  rates[local_ttl_index][num_interarrival[local_ttl_index]-1]=high_rate[local_ttl_index]*1000000.0;
  iat[local_ttl_index][num_interarrival[local_ttl_index]-1]=8*((double) pktsize)/rates[local_ttl_index][num_interarrival[local_ttl_index]-1];
  
  /* compute instantaneous rates within chirp */
  for (count=num_interarrival[local_ttl_index]-2;count>=0;count--)
    {
      rates[local_ttl_index][count]=rates[local_ttl_index][count+1]/spread_factor;
      iat[local_ttl_index][count]=iat[local_ttl_index][count+1]*spread_factor;
    }
  return;  
}
