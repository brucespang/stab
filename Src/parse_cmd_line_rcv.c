#include "stab_rcv.h"


/* get command line parameters */
void parse_cmd_line(argc,argv)
  int	argc;
     char	*argv[];
{
  char	*ptr;			/* to traverse the arguments */
  double duration=600.0;
  struct timeval tp_start;

      argc--; argv++;

  /* go through the arguments */
   while (argc > 0) {
      ptr = *argv;
      while (*ptr) switch (*ptr++) {
 case 'l':/* lowest rate to probe at */
       if (*ptr == 0) {
	 argc--; argv++;
	 if (*argv == 0) {
	   (void) fprintf (stderr,
			   "stab_rcv: no lowest rate '-l'.\n");
	   (void) exit (1);
	 }
	 spec_low_rate = atof (*argv);
       }
       else {
	 spec_low_rate = atof (*argv);
	 *ptr = 0;
       }
       fprintf(stderr,"low rate is %f \n",spec_low_rate);
       break;
       
     case 'u':/* highest rate to probe at */
       if (*ptr == 0) {
	 argc--; argv++;
	 if (*argv == 0) {
	   (void) fprintf (stderr,
			   "stab_rcv: no highest rate '-u'.\n");
	   (void) exit (1);
	 }
	 spec_high_rate = atof (*argv);
       }
       else {
	 spec_high_rate = atof (*argv);
	 *ptr = 0;
       }
       fprintf(stderr,"high rate is %f \n",spec_high_rate);
       break;
       
       
     case 'a':
       if (*ptr == 0) {
	 argc--; argv++;
	 if (*argv == 0) {
	   (void) fprintf (stderr,
			   "stab_rcv: no average rate '-a'.\n");
	   (void) exit (1);
	 }
	 avg_rate = atof (*argv);
       }
       else {
	 avg_rate = atof (*argv);
	 *ptr = 0;
       }
       fprintf(stderr,"avg_rate is %f \n",avg_rate);

       break;
       
     case 't':
       if (*ptr == 0) {
	      argc--; argv++;
	      if (*argv == 0) {
		(void) fprintf (stderr,
				"stab_rcv: no duration '-t'.\n");
		(void) exit (1);
	      }
	      duration = atof (*argv);
       }
       else {
	 duration = atof (*argv);
	 *ptr = 0;
       }

       break;
       
       
     case 's':
       if (*ptr == 0) {
	 argc--; argv++;
	 if (*argv == 0) {
	   (void) fprintf (stderr,
			   "stab_rcv: no spread_factor '-t'.\n");
	   (void) exit (1);
	 }
	 spread_factor = atof (*argv);
       }
       else {
	 spread_factor = atof (*argv);
	 *ptr = 0;
       }
       fprintf(stderr,"spread  is %f \n",spread_factor);

       break;
       
       
     case '-':
       break;


        case 'p':		/* packet size */
       if (*ptr == 0) {
	 argc--; argv++;
	 if (*argv == 0) {
	   (void) fprintf (stderr,
			   "stab_rcv: no packet size given with '-p'.\n");
	   (void) exit (1);
	 }
	 pktsize = atoi (*argv);
       }
       else {
	 pktsize = atoi (*argv);
	 *ptr = 0;
       }
       break;
       

        case 'J':		/* Jumbo size in packets*/
       if (*ptr == 0) {
	 argc--; argv++;
	 if (*argv == 0) {
	   (void) fprintf (stderr,
			   "stab_rcv: no Jumbo size given with '-J'.\n");
	   (void) exit (1);
	 }
	 jumbo = atoi (*argv);
       }
       else {
	 jumbo = atoi (*argv);
	 *ptr = 0;
       }
       break;
       


     case 'S':
       if (*ptr == 0) {
	 argc--; argv++;
	 if (*argv == 0) {
	   (void) fprintf (stderr,
			   "stab_rcv: no destination host given with '-S'.\n");
	   (void) exit (1);
	 }
	 (void) strcpy (hostname, *argv);
       }
       else {
	 (void) strcpy (hostname, ptr);
	 *ptr = 0;
       }
       fprintf(stderr,"Sender host is:  %s \n",hostname);
       break;

      case 'D':
	debug = 1;
	break;

      case 'v':
	fprintf(stderr,"stab version %s\n",VERSION);
	exit(0);
	break;
              
     case 'b':/* busy period length */
       if (*ptr == 0) {
	 argc--; argv++;
	 if (*argv == 0) {
	   (void) fprintf (stderr,
			   "stab_rcv: no busy period length '-b'.\n");
	   (void) exit (1);
	 }
	 busy_period_thresh = atoi (*argv);
       }
       else {
	 busy_period_thresh = atoi (*argv);
	 *ptr = 0;
       }
       fprintf(stderr,"busy period length is %d \n",busy_period_thresh);
       break;

     case 'd':/* decrease factor */
       if (*ptr == 0) {
	 argc--; argv++;
	 if (*argv == 0) {
	   (void) fprintf (stderr,
			   "stab_rcv: no decrease factor '-d'.\n");
	   (void) exit (1);
	 }
	 decrease_factor = atof (*argv);
       }
       else {
	 decrease_factor = atof (*argv);
	 *ptr = 0;
       }
       fprintf(stderr,"decrease factor is %f \n",decrease_factor);
       break;

     case 'n':/* number of inst. bw. estimates to smooth over*/
       if (*ptr == 0) {
	 argc--; argv++;
	 if (*argv == 0) {
	   (void) fprintf (stderr,
			   "stab_rcv: no averaging length '-n'.\n");
	   (void) exit (1);
	 }
	 num_inst_bw = atoi (*argv);
       }
       else {
	 num_inst_bw = atoi (*argv);
	 *ptr = 0;
       }

       break;
           
 case 'U':		/* UDP port */
       if( *ptr == 0) {
	 argc--; argv++;
	 if (*argv == 0) {
	   (void) fprintf (stderr,
			   "stab_rcv: no port number given with '-U'.\n");
	   (void) exit (1);
	 }
	 sndPort = atoi (*argv);
       }
       else {
	 sndPort = atoi (ptr);
	 *ptr = 0;
       }
       break;


    case 'g':
      if( *ptr == 0) {
        argc--; argv++;
        if (*argv == 0) {
          (void) fprintf (stderr,
                          "stab_rcv: no ttl_gap given with '-g'.\n");
          (void) exit (1);
        }
        ttl_gap = atoi (*argv);
      }
      else {  
        ttl_gap = atoi (ptr);
        *ptr = 0;
      }
      fprintf(stderr,"TTL_GAP=%d\n",ttl_gap);
      
      if(ttl_gap<1 || ttl_gap>6) 
	{
	  fprintf(stderr,"TTL_GAP must be between 1 and 6\n");
	  exit(1);
	}
      
      break;

      case 'h':
      case 'H':
	 usage();
       
      default:
	 (void) fprintf (stderr,
			 "stab_rcv: Unknown option '%c'\n", ptr[-1]);

      }
      argc--; argv++;
   }
 /* checking  parameters */

   if (debug) fprintf(stderr,"finished reading command line parameters\n");
   /* set the destination as far as sender is concerned
    * i.e. echo host or the final destination host */
     src_addr.s_addr = gethostaddr(hostname);
   if (src_addr.s_addr == 0) {
      (void) fprintf (stderr, "stab_rcv: %s: unknown host\n",
		      hostname);
     usage();
      (void) exit (1);
   }
   else{
     /* initializing */
       bzero((char *)&src, sizeof (src));

     src.sin_addr = src_addr;

   }

  if (decrease_factor <=1.0)
     {perror ("stab_rcv: decrease_factor invalid");
	 (void) exit (1);
     }

  if (duration <0)
     {perror ("stab_rcv: time duration invalid");
	 (void) exit (1);
     }
  else
    {
      (void) gettimeofday (&tp_start, (struct timezone *) 0);
      stop_time=(double)tp_start.tv_sec+(((double)tp_start.tv_usec)/1000000.0)+duration;
    }

  if (jumbo<1 || jumbo>20)
     {perror ("stab_rcv: jumbo must be between 1 and 20");
	 (void) exit (1);
     }

  if (busy_period_thresh <=2)
     {perror ("stab_rcv: busy_period_thresh invalid, must be integer >2");
	 (void) exit (1);
     }

  if (sndPort<1024 ||  sndPort>65535)
     {perror ("stab_rcv: Port number must be in [1024-65535]");
	 (void) exit (1);
     }

 /*check if parameters ok*/
  if (pktsize<MINPKTSIZE)
    {
      fprintf(stderr,"stab_rcv: packet size too small, using minimum sized %d byte packet\n",MINPKTSIZE);
      pktsize=MINPKTSIZE;
    }
  else
    {
      if (pktsize>MAXMESG)
	{
	  fprintf(stderr,"stab_rcv: packet size too big, using maximum sized %d byte packet\n",MAXMESG);
	  pktsize=MAXMESG;
	}
    }

  if(spread_factor<MINSPREAD)
    fprintf(stderr,"stab_rcv: packet spread too small, using minimum packet spread %f\n",MINSPREAD); 

  if( spec_low_rate<0.0 || spec_high_rate<0.0 || avg_rate<0.0 || spec_low_rate>spec_high_rate)
    {
      perror("stab_rcv: probing rates invalid\n"); 
      exit(0);
    }


 if (avg_rate>MAX_AVG_RATE)
    {
      fprintf(stderr,"Average rate too high, reducing to %f Mbps\n",MAX_AVG_RATE);
      avg_rate=MAX_AVG_RATE;

    }

  if (avg_rate<MIN_AVG_RATE)
    {
      fprintf(stderr,"Average rate too low, increasing to %f Mbps\n",MIN_AVG_RATE);
      avg_rate=MIN_AVG_RATE;

    }
  if (spec_high_rate>2000.0*spec_low_rate)
    {
      fprintf(stderr,"Ratio of high/low rate very large, increasing low_rate\n");
      spec_low_rate=spec_high_rate/1000.0;

    }
  if (spec_high_rate<5.0*spec_low_rate)
    {
      fprintf(stderr,"Ratio of high/low rate very low, increasing high_rate\n");
      spec_high_rate=5*spec_low_rate;

    }

  /* opening files */
  gethostname(localhost,sizeof(localhost));
  open_dump_files(hostname,localhost);
   if (debug) fprintf(stderr,"exiting parse_cmd_line\n");

}


/* computes number of packets, chirp duration and inter-chirp time */
void compute_parameters(int ttl_index)
{
  double chirp_duration;
  double interarrival;
  double new_high_rate;/*used just in case given rates lead to an extra large chirp*/
  int count;
  double min_low_rate=100000.0;

  if(ttl_index>=num_ttl || ttl_index<0)
    {
      fprintf(stderr,"compute_parameters: ttl_index too high/low %d (num_ttl=%d)\n",ttl_index,num_ttl);
      exit(0);
    }

  if (debug) fprintf(stderr,"compute pars,ttl_index=%d\n",ttl_index);
  /* checking if high_rate is too high. Lower rate if true */
  if (high_rate[ttl_index]>MAX_HIGH_RATE)
    {
      high_rate[ttl_index]=MAX_HIGH_RATE;
      if (low_rate[ttl_index]>(double)MAX_HIGH_RATE/10.0)
	low_rate[ttl_index]=(double)MAX_HIGH_RATE/10.0;
    }

  new_high_rate=low_rate[ttl_index];


  interarrival=((double)pktsize)*8/(1000000.0*low_rate[ttl_index]);

  chirp_duration=0;
  num_interarrival[ttl_index]=0;

  while(interarrival>((double)pktsize)*8.0/(1000000.0*high_rate[ttl_index]))
    {
      num_interarrival[ttl_index]++;
      chirp_duration+=interarrival*(double)jumbo;
      interarrival=interarrival/spread_factor;

      new_high_rate=new_high_rate*spread_factor;
      /*if chirp too large lower high_rate and use smaller chirp*/
      if (num_interarrival[ttl_index]>=MAXCHIRPSIZE-1)
	{
	  if(debug) fprintf(stderr,"Chirp too big, using MAXCHIRPSIZE, reducing high_rate\n");
	  high_rate[ttl_index]=new_high_rate;
	  break;
	}
     }

  fprintf(stderr,"\nUpdating probing range:low=%f,high=%fMbps, ttl=%d\n",low_rate[ttl_index],high_rate[ttl_index],(ttl_index+1)*ttl_gap);

  max_good_pkt_this_chirp=num_interarrival[ttl_index];/*initializing*/

  if (debug) fprintf(stderr,"num_ttl=%d\n",num_ttl);
  for(count=0;count<num_ttl;count++)
    {
      if (min_low_rate>low_rate[count])
	min_low_rate=low_rate[count];
    }
  /*if this new low rate is overall lowest among all chirps, update the interchirp parameters etc.*/
  if (min_low_rate-low_rate[ttl_index]>-0.1)
    {
      if (debug) fprintf(stderr,"Changing interchirp time\n");

      inter_chirp_time=((double)(num_interarrival[ttl_index]+1))*8.0*jumbo*pktsize/(avg_rate*1000000.0);
      
      if (inter_chirp_time<=chirp_duration)
	inter_chirp_time=2.0*chirp_duration;
      
      /*write once every chirp */
      write_interval=chirp_duration+(inter_chirp_time-chirp_duration)/2;
      
      /* if timer granularity too large change write_interval*/
      if (write_interval<min_timer)
	{
	  if (debug) fprintf(stderr,"write int=min timer\n");
	  write_interval=min_timer;
	}
      /*allocating enough memory to store packet information if not enough*/
      
      if((int)(1+(write_interval/(inter_chirp_time)))>chirps_per_write && created_arrays)
	{
	  chirps_per_write=(int)(1+(write_interval/(inter_chirp_time)));
	  pkts_per_write=(int)((MAXCHIRPSIZE)*chirps_per_write);
	  
	  if (debug) fprintf(stderr,"comp_pars: chirps_per_write=%d, pkts_per_write=%d\n",chirps_per_write,pkts_per_write);
	  
	  realloc(packet_info,(int)pkts_per_write*2*sizeof(struct pkt_info));      
	  realloc(chirp_info,(int)chirps_per_write*2*sizeof(struct chirprecord));
	  
	}
    }
  return;
  
}
