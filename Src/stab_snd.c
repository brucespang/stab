/*
 * stab_snd.c
 * Copyright (c) 2004 Rice University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Rice
 * University not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Rice University makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.  
 *
 * RICE UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL RICE UNIVERSITY BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Author: Vinay Ribeiro, vinay@rice.edu.  */

/*
 * "stab_snd" sends chirp packet trains to "stab_rcv" based
 * on specified parameters.
 *
 *  stab_snd.c is based on udpsend.c of the NetDyn tool.
 * */


/*
 * udpsend.c
 * Copyright (c) 1991 University of Maryland
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of U.M. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  U.M. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * U.M. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL U.M.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Dheeraj Sanghi, Department of Computer Science.
 */


/*
 * Return the address (in network byte order) of the requested host.
 *
 * We return 0 if we can't find anything.
 */

#include "stab_snd.h"
#include "delay.h"

#define UNCONNECTED 0
#define ESTIMATETTL 1
#define SENDCHIRPS 2

#define PKTSIZE	1000		/* default packet size */
#define MAXHNAME	100		/* maximum hostname size */

struct sockaddr_in snd;/* used in binding */
struct	sockaddr_in dst;	/* destination internet address, one is for the valid destination address and the other */

int sobigpkt;/*socket number, used to send big packets with limited TTL*/
u_int32_t request_num=0;
int *num_interarrival;/* number of packets in a chirp minus one */

double	*gap;		/* gap in us between 2 trains */
double   *largest_inter_arrival;/* min/largest inter arrival in the chirp in us*/

double   *low_rate;/*lowest rate (Mbps) to probe at*/

int allocated_mem=0;/* if already allocated memory */
int fromlen;/*must be non-zero, used in recvfrom*/
int debug=0;
double inter_chirp_time;
int state=UNCONNECTED;/*state*/
int sndPort=SNDPORT;
char	data[MAXMESG];		/* Maxm size packet */
char	data_rcv[MAXMESG];		/* Maxm size packet */
int soudp;

int prev_rcv_pkt_num=0;/*packet number of previous RECV_OK packet*/
int ttl=64;
int ttl_index=0;
int ttl_gap=64;
int num_ttl=1;/*how many ttl's to use?*/
  struct	timeval	*tp, tp1;		/* used for getting timeofday */
int cc;
int recv_ok_count=0;
int	pktsize = PKTSIZE;	/* packet size */

u_int32_t jumbo=1;/*each jumbo chirp "packet" consists of jumbo number of packets*/
u_int32_t chal_no;
double	sleeptime = 0;	/* sleep in us between 2 bulks */

double chirp_duration=0.0;/* duration of a chirp */
double spread_factor=1.2; /* decrease in spread of packets within the chirp*/
int	write_error = NO;	/* error on sending a packet */
int	np;			/* index variable for packet-count */
int	nc=0;			/* index variable for chirp-count */

int udprecordsize=0;/*size of struct udprecord*/

struct	udprecord *sendpkt;	/* udp packet content */
struct control_rcv2snd *rcvpkt;
  

/* Usage information */

int usage()
{
   (void) fprintf (stderr, "usage: stab_snd [-h] <more options>\n");
   (void) fprintf (stderr, "The options are: \n");
   (void) fprintf (stderr, "\t -U <sender port, default=%d>\n",SNDPORT);
   (void) fprintf (stderr, "\t -h \tHelp: produces this output\n");
   (void) fprintf (stderr, "\t -v version\n");
   (void) fprintf (stderr, "\t -D print debug information\n");
   (void) exit (1);
}

void parse_cmd_line(argc,argv)
  int	argc;
     char	*argv[];
{
  char	*ptr;			/* to traverse the arguments */

  argv++; argc--;
  
  /* option processing */
  while (argc > 0) {
    ptr = *argv;
    while (*ptr) switch (*ptr++) {
      
     case 'U':		/* port */
      if( *ptr == 0) {
	argc--; argv++;
	if (*argv == 0) {
	  (void) fprintf (stderr,
			  "stab_snd: no port number given with '-U'.\n");
	  (void) exit (1);
	}
	sndPort = atoi (*argv);
      }
      else {
	sndPort = atoi (ptr);
	*ptr = 0;
      }
      break;
      
    case '-':
      break;
    case 'D':
      debug=1;
      break;

    case 'v':
      fprintf(stderr,"stab version %s\n",VERSION);
      exit(0);
      break;
      
    case 'h':		/* help */
    case 'H':
      usage();
      
    default:
      (void) fprintf (stderr,
		      "stab_snd: Unknown option '%c'\n", ptr[-1]);
      (void) exit (1);
      
    }
    argc--; argv++;
  }
}

/* challenge number must be included in every control packet from rcv to snd */
void create_challenge_number()
{
  struct timeval chal_tp;
  (void) gettimeofday (&chal_tp, (struct timezone *) 0);

  srand(hash((u_int32_t)chal_tp.tv_usec,(u_int32_t)chal_tp.tv_sec));

  chal_no=(u_int32_t)(rand());
  if(debug) fprintf(stderr,"Creating chall number=%u\n",chal_no);
  
  return;
}

/* Reset all variables to original values  */
void reset_variables()
{

  fromlen=(int)sizeof(struct sockaddr);/*must be non-zero, used in recvfrom*/
  nc=0; /*resetting chirp number */
  state=UNCONNECTED;
  prev_rcv_pkt_num=0;/*packet number of previous RECV_OK packet*/
  ttl=64;
  ttl_index=0;
  ttl_gap=64;
  num_ttl=1;/*how many ttl's to use?*/
  jumbo=1;
  recv_ok_count=0;
  
  sleeptime = 0;	/* sleep in us between 2 bulks */
  
  chirp_duration=0.0;/* duration of a chirp */
  spread_factor=1.2; /* decrease in spread of packets within the chirp*/
  write_error = NO;	/* error on sending a packet */
    
}

/* connect to  receiver machine that has passed the test*/
void connect_so()
{
  unsigned int dst_ip=0;

  if (connect (soudp, (struct sockaddr *) &dst, sizeof (dst)) < 0) {
      perror ("stab_snd: could not connect to the receiver");
      (void) exit (1);
   }


  if (connect (sobigpkt, (struct sockaddr *) &dst, sizeof (dst)) < 0) {
      perror ("stab_snd: could not connect to the receiver");
      (void) exit (1);
   }


  dst_ip = ntohl((unsigned int)dst.sin_addr.s_addr);

  fprintf(stderr,"Connected to %u.%u.%u.%u\n",(dst_ip>>24)&0x000000ff,(dst_ip>>16)&0x000000ff,(dst_ip>>8)&0x000000ff,(dst_ip)&0x000000ff);

  if(debug) fprintf(stderr,"connecting socket,state=ESTIMATETTL\n");
  state=ESTIMATETTL;
  return;
}

void free_mem()
{
  free(largest_inter_arrival);
  
  free(num_interarrival);
  
  free(gap);
  
  free(low_rate);
  allocated_mem=0;

}

/* unconnect sockets, reset variables, free memory*/
void unconnect_so()
{

  dst.sin_family = AF_UNSPEC;
  
  dst.sin_addr.s_addr=htonl(INADDR_ANY);
  
  dst.sin_port = htons ((u_short) sndPort);
  
  connect (soudp, (struct sockaddr *) &dst, sizeof (dst));
  /*if (err< 0 && err!=EAFNOSUPPORT) {
    perror ("stab_snd: could not unconnect\n");
    (void) exit (1);
    }*/
  
  connect (sobigpkt, (struct sockaddr *) &dst, sizeof (dst));
  
  if(debug) fprintf(stderr,"disconnecting socket\n");
  create_challenge_number();
  reset_variables();
  if (allocated_mem)
    free_mem();/*free array memory for next connection*/
  
  fprintf(stderr,"\nWaiting for receiver to setup connection\n");
  
  return;
}


inline void send_pkt(int big_pkt_ttl)
{
  u_int32_t crc;
  struct timeval tp_snd;
  
  (void) gettimeofday (&tp_snd, (struct timezone *) 0);
  
  sendpkt->timesec = htonl ((u_int32_t) tp_snd.tv_sec);
  sendpkt->timeusec = htonl ((u_int32_t) tp_snd.tv_usec);
  sendpkt->num = htonl ((u_int32_t) (np));
  sendpkt->chirp_num = htonl ((u_int32_t) (nc));
  sendpkt->request_num = htonl ((u_int32_t) request_num);

  sendpkt->chal_no=htonl ((u_int32_t) (chal_no));
  sendpkt->big_pkt_ttl=htonl ((u_int32_t) big_pkt_ttl);	

  if (debug) 
    fprintf(stderr,"SEND_PKT req_num=%d\n",request_num);
  
  /*NOTE MUST CHANGE CRC TO INCLUDE BIG_PKT_TTL FIELD*/
  crc=gen_crc_snd2rcv(sendpkt);
  sendpkt->checksum = htonl ((u_int32_t) (crc));
  
  /* Send jumbo packet out*/
  
  switch (state)
    {
    case UNCONNECTED:
      cc=sendto(soudp,(char *)sendpkt,sizeof(struct udprecord), 0, (struct sockaddr *)&dst, sizeof(dst));
      break;

    case ESTIMATETTL:
      cc = write(soudp, data, (size_t)udprecordsize);
      if (debug) fprintf(stderr,"wrote big packet ttl\n");      
      break;

   case SENDCHIRPS:
     /*send a big packet followed by a small one except for max ttl*/
     if(big_pkt_ttl<(num_ttl*ttl_gap))
       {
	 cc = write(sobigpkt, data, (size_t)pktsize);
	 cc = write(soudp, data, (size_t)udprecordsize);
       }
     else
       {
	 cc=write(soudp,data, (size_t)pktsize);
       }
     break;
    }


  /* if receiver shuts down, reset and wait for new connection */
  if (cc < 0) {
    (void) fprintf (stderr,
		    "Packet number %d\n", np);
    if(debug) fprintf(stderr,"stab_snd: write/sendto error, resetting\n");
    unconnect_so();
  }

}
  


/*
  Setting number of packets, packet interarrival times etc.
*/

int compute_parameters(int hops)
{
  int count;
  int pars_ok=1;
  double interarrival;
 
  /* get data from packet */
  inter_chirp_time=(double)(ntohl (rcvpkt->inter_chirp_time));/* in us*/
  low_rate[hops]=(double)(ntohl (rcvpkt->low_rate))/10000.0;
  spread_factor=(double)(ntohl (rcvpkt->spread_factor))/10000.0;
  pktsize=(int) ntohl (rcvpkt->pktsize);
  jumbo=ntohl (rcvpkt->jumbo);
  num_interarrival[hops]=(int) ntohl (rcvpkt->num_interarrival);


  if(debug) fprintf(stderr,"ict=%f,low=%f,sf=%f,psize=%d,rqnum=%d,numiat=%d\n",inter_chirp_time,low_rate[hops],spread_factor,pktsize,request_num,num_interarrival[hops]);
  /*check if parameters ok*/
  if (pktsize<40 || spread_factor<1.05 || inter_chirp_time<0.0 || low_rate[hops]<0.0 || num_interarrival[hops]<1 || jumbo<1 || jumbo>20)
    pars_ok=0;

  chirp_duration=0;
  interarrival=((double)pktsize)*8.0*(double)jumbo/(low_rate[hops]);/*in us*/
  largest_inter_arrival[hops]=interarrival/(double)jumbo;

  for(count=1;count<=num_interarrival[hops];count++)
    {
      chirp_duration+=interarrival;
      interarrival=interarrival/spread_factor;
     }
 
  gap[hops] = inter_chirp_time - chirp_duration;

  if (debug) fprintf(stderr,"gap[%d]=%f\n",hops,gap[hops]);

  /* if gap between chirps is negative, set gap to the interchirp time*/
  if (gap[hops]<=0) 
    gap[hops]=inter_chirp_time;
 
  return(pars_ok);
}


void alloc_mem()
{
  largest_inter_arrival=(double *)calloc((int)(num_ttl),sizeof(double));
  num_interarrival=(int *)calloc((int)(num_ttl),sizeof(int));
  gap=(double *)calloc((int)(num_ttl),sizeof(double));
  low_rate=(double *)calloc((int)(num_ttl),sizeof(double));
  allocated_mem=1;
}

/* takes care of requests  */
int handle_request(u_int32_t request_type)
{

   int	i,ok=1;
   if(debug) fprintf(stderr,"in handle_request\n");

    switch(request_type)
      {
      case REQ_CONN:/* waiting for initial packet */
	if(debug) fprintf(stderr,"got REQ_conn\n");
	
	if (state==UNCONNECTED)
	  {
	    request_num=ntohl(rcvpkt->request_num);
	    send_pkt(ttl);/*value of ttl does not matter here*/
	  }
	else
	  { ok=0;
	    if (debug)
	      fprintf(stderr,"Wrong request\n");
	  }
	
	break;
	
      case CHALL_REPLY:/*if checksum is ok then we only have to make sure that the challenge number is 
			 present in the packet*/
	if(debug) fprintf(stderr,"got challenge_reply\n");
	
	if (ntohl(rcvpkt->chal_no)==chal_no && state==UNCONNECTED)
	  {/*record request_num for this connection*/
	    request_num=ntohl(rcvpkt->request_num);
	    connect_so();
	  }
	else
	  { ok=0;
	    if (debug)
	      fprintf(stderr,"Wrong request\n");
	  }
	break;
	
      case FIND_TTL:
	if (ntohl(rcvpkt->chal_no)==chal_no && state!=UNCONNECTED)
	  {	
	    num_ttl=ntohl(rcvpkt->num_ttl);
	    ttl_gap=ntohl(rcvpkt->ttl_gap);
	    ttl=ttl_gap;/*initialize ttl*/

	    if(debug) fprintf(stderr,"ttl_gap=%d,num_ttl=%d\n",ttl_gap,num_ttl);
	    state=SENDCHIRPS;
	    
	    /* changing the TTL value*/
	    i=ttl_gap;
	    if (setsockopt(sobigpkt, IPPROTO_IP, IP_TTL, (char *)&i,
			   sizeof (i)) < 0) {
	      perror ("stab_snd: setsockopt failed\n");
	      exit (1);
	    }
	    i=64;
	    if (setsockopt(soudp, IPPROTO_IP, IP_TTL, (char *)&i,
			   sizeof (i)) < 0) {
	      perror ("stab_snd: setsockopt failed\n");
	      exit (1);
	    }
	    
	    nc=1;/*increment chirp number to 1 since 0 is for FIND_TTL packets*/
	    
	    ttl=ttl_gap;/*set ttl to 1*ttl_gap*/
	    ttl_index=0;
	    
	    alloc_mem();/*allocate memory*/
	    
	    for (i=0;i<num_ttl;i++)
	      {
		if(!compute_parameters(i))/*if parameters not weird*/
		  {
		    fprintf(stderr,"parameters not good\n");
		    exit(0);
		  }
	      }
	  }
	else
	  {
	    ok=0;
	    if (debug)
	      fprintf(stderr,"Wrong request\n");
	  }
	break;
	
      case UPDATE_RATES:
	/* if state!=UNCONNECTED and IP address matches that of receiver then update rates*/

	if (ntohl(rcvpkt->chal_no)==chal_no && state!=UNCONNECTED)
	  {	
	  compute_parameters(ntohl(rcvpkt->ttl_index));
	  }
	else
	  {
	    ok=0;
	    if (debug)
	      fprintf(stderr,"Wrong request\n");
	  }
	break;

      case STOP:

	if (ntohl(rcvpkt->chal_no)==chal_no && state!=UNCONNECTED)
	  {	
	    for (i = 1; i <= 10; i++) {
	      np = 0;
	      send_pkt(0);
	      if (state==UNCONNECTED) break;
	      if (sleeptime < 10000) sleeptime = 10000;
	      usleep (sleeptime);
	    }
	    fprintf(stderr,"\nFinished sending chirps to client\n");   
	    if (state!=UNCONNECTED)
	      unconnect_so();
	  }
	else
	  {
	    ok=0;
	    if (debug)
	      fprintf(stderr,"Wrong request\n");
	  }
      
	break;

      case RECV_OK:/*only if packet number has incremented, record the OK packet*/
	if (ntohl(rcvpkt->chal_no)==chal_no && ntohl (rcvpkt->num)>prev_rcv_pkt_num && state!=UNCONNECTED)
	  {
	    recv_ok_count=0;/*if exceeds certain number we reset*/
	    if (debug)
	    fprintf(stderr,"Receiving OK packet\n"); 
	  }
	else
	  {
	    ok=0;
	    if (debug)
	      fprintf(stderr,"Wrong request\n");
	  }
	break;

      default:
	ok=0;
	if(debug) fprintf(stderr,"Invalid request type\n");
	break;
	
      }
    return(ok);
}


/* receive packet and verify checksum*/
void recv_pkt()
{

  if(state)
    cc = read (soudp, data_rcv, MAXMESG);
  else
    cc = recvfrom (soudp, data_rcv, MAXMESG, 0, (struct sockaddr *) &dst,&fromlen);

  if (cc < 0) {
    fprintf (stderr,"\nstab_snd: read,state=%d\n",state);
    unconnect_so();
    return;
  } 
  
  if(debug) fprintf(stderr,"RECV_PKT, request_num=%d,req type=%d\n",(int)ntohl(rcvpkt->request_num),(int)ntohl(rcvpkt->request_type));

  if(debug) fprintf(stderr,"got packet,len=%d\n",cc);

  if(check_crc_rcv2snd(rcvpkt))/*if packet good*/
    {
      if(debug) fprintf(stderr,"crc ok\n");

      if (state==UNCONNECTED || ntohl(rcvpkt->request_type)==RECV_OK)/*handle all requests in UNCONNECTED state*/
	{
	  handle_request(ntohl(rcvpkt->request_type));
	}
      else /*only handle requests not already handled*/
	if (ntohl(rcvpkt->request_num)>request_num)
	  {
	    request_num=ntohl(rcvpkt->request_num);

	    if (handle_request(ntohl(rcvpkt->request_type)))
	      prev_rcv_pkt_num=(int) ntohl (rcvpkt->num);
	  }
      
    }
  else{  
     if(debug) 
      fprintf(stderr,"crc BAD\n");
  }

}

/* be in select mode for receiving packet, wait only for "time" in usec */
long run_select(long time)
{
  int num_so;
  struct   timeval tp_select,tp_start,tp_end;
  fd_set rset;

  long remaining_time;
  tp_select.tv_sec=time/1000000;
  tp_select.tv_usec=time%1000000;
    

  FD_ZERO(&rset);
  FD_SET(soudp,&rset);

  (void) gettimeofday (&tp_start, (struct timezone *) 0);

  num_so=soudp+1;
  select(num_so,&rset,NULL,NULL,&tp_select);

  if (FD_ISSET(soudp,&rset))
   {
     if (debug) fprintf(stderr,"run select packet\n");
     recv_pkt();
     /*     smartwait(time,&tp_start);*/
/*wait for remaining time. With this setup we only can receive one packet between chirps. To receive more packets we must run
			   select again*/
   }
  (void) gettimeofday (&tp_end, (struct timezone *) 0);
  if (debug) fprintf(stderr,"tesec=%ld,teusec=%ld,tssec=%ld,tsusec=%ld,time=%ld\n",tp_end.tv_sec,tp_end.tv_usec,tp_start.tv_sec,tp_start.tv_usec,time);
  remaining_time=time-(tp_end.tv_sec*1000000 + tp_end.tv_usec - tp_start.tv_sec*1000000 - tp_start.tv_usec);

  return(remaining_time);

}

void vary_ttl()
{

  int count,num_bursts;      

  if (debug) fprintf(stderr,"in vary_ttl\n");

  for (num_bursts=1;num_bursts<=2;num_bursts++){
    for (count=1;count<=32;count++)
      {
	/* changing the TTL value*/
	if (setsockopt(soudp, IPPROTO_IP, IP_TTL, (char *)&count,
		       sizeof (count)) < 0) {
	  perror ("stab_snd: ttl option\n");
	  exit (1);
	}
	
	send_pkt(count);
	if (debug) fprintf(stderr,"send pkt ttl=%d\n",count);
      }
    
    smartwait2((int)RTTUSEC/5);/* introducing some delay between bursts */
  }

  return;

}


/* Send chirps */    
void chirps_snd()
{
 long remaining_time;
  count=0;

  fprintf(stderr,"\rChirp Numer: %d",nc);
  
  /********* BEGIN CHIRP ********/
  sleeptime = largest_inter_arrival[ttl_index]*spread_factor;
  
  /* Hack because sleeptime gets divided before the first sleep period*/
  (void) gettimeofday (&tp1, (struct timezone *) 0);
  tp1.tv_usec-=100000;  
  
  /* Hack so smartwait doesn't wait on 1st packet Be careful, if you add
     too much, tv_usec will wrap around and result in a positive
     time difference */
  
  for (np = 1; np <= num_interarrival[ttl_index]+1; np++) {
    
    for (count=1;count<=jumbo;count++)
      {
	smartwait((unsigned)(sleeptime-.5), &tp1);
	
	/* Wait for smartwait-1+0.5
	   The -1 is to account for gettimeofday, the +0.5 is for rounding*/
	send_pkt(ttl);
      }
    if(state==UNCONNECTED)
      return;

	/* Prepare for a shorter sleep next time */
	sleeptime = sleeptime/spread_factor;

  }
  /********* END CHIRP ********/

  nc++;
  ttl+=ttl_gap;
  if (ttl>num_ttl*ttl_gap)
    ttl=ttl_gap;

  /*changing big packet TTL*/

  if (setsockopt(sobigpkt, IPPROTO_IP, IP_TTL, (char *)&ttl,
		 sizeof (ttl)) < 0) {
    perror ("stab_snd: ttl option\n");
    exit (1);
  }

  if(debug) fprintf(stderr,"TTL=%d\n",ttl);


  ttl_index=ttl/ttl_gap - 1;

  /* gap between two successive packet trains */
      (void) gettimeofday (&tp1, (struct timezone *) 0);
      if(debug) fprintf(stderr,"gap=%f\n",gap[ttl_index]);
     recv_ok_count++;/* receiver must periodically send packets saying it is ok*/
     if (recv_ok_count>MAX_RECV_OK_COUNT)
       {if(debug) fprintf(stderr,"recv_ok_count=%d\n",recv_ok_count);
	 unconnect_so();
	 recv_ok_count=0;
       }

     remaining_time=(int)gap[ttl_index];
     while(state==SENDCHIRPS && remaining_time>1000)/*wait for another recv packet only if more than 1ms remains for next chirp*/
       {
	 remaining_time=run_select(remaining_time);
	 if (debug) fprintf(stderr,"remaining time=%ld\n",remaining_time);
       }

}


/*setup socket and wait for packets*/
void setup_socket_and_wait()
{

  fprintf(stderr,"Waiting for receiver to setup connection\n");

 /* initial part of data is actually the log record */
  sendpkt = (struct udprecord *) data;
  bzero((char *) sendpkt, sizeof (struct udprecord));
  
  /*check if packet ok, then send challenge*/ 
  rcvpkt=(struct control_rcv2snd *) data_rcv;
  
  /* create a socket to send and receive packets */
  soudp = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (soudp < 0) {
    perror ("stab_snd: socket");
    (void) exit (1);
  }

  /* create a socket to send to send large chirp packets */
  sobigpkt = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sobigpkt < 0) {
    perror ("stab_snd: socket");
    (void) exit (1);
  }
  
 /* setting the TTL value*/
  if (setsockopt(sobigpkt, IPPROTO_IP, IP_TTL, (char *)&ttl,
		 sizeof (ttl)) < 0) {
    perror ("stab_snd: ttl option\n");
    exit (1);
  }


  /* initialize address/port number */
  bzero ((char *) &dst, sizeof (dst));
  bzero ((char *) &snd, sizeof (snd));
  
  snd.sin_family = AF_INET;
  
  snd.sin_addr.s_addr=htonl(INADDR_ANY);
  
  snd.sin_port = htons (sndPort);
  
  bind(soudp,(struct sockaddr *) &snd, sizeof(snd));
  
  state=UNCONNECTED;
  
  if(debug) fprintf(stderr,"Waiting for packet\n");

  if(state)
    cc = read (soudp, data_rcv, MAXMESG);
  else
    cc = recvfrom (soudp, data_rcv, MAXMESG, 0, (struct sockaddr *) &dst,&fromlen);
  
   create_challenge_number();

   while(1)
     {
       switch(state)
	 {
	 case UNCONNECTED:
	   if(debug) fprintf(stderr,"state=UNCONNECTED\n");
	   recv_pkt();/*waiting for receiver to connect*/
	   break;
	 case ESTIMATETTL:
	   if(debug) fprintf(stderr,"state=ESTIMATETTL\n");
	   vary_ttl();/* send packets with incremented ttl */
	   run_select(3*RTTUSEC);/* wait for result of max ttl  */
	   break;
	 case SENDCHIRPS:
	   if(debug) fprintf(stderr,"state=SENDCHIRPS\n");
	   chirps_snd();/*send chirp packets */
	   break;
	 }
     }
   return;

}

/* main function  */

int main(argc, argv)
     int	argc;
     char	*argv[];
{
   /* allocate space for local timestamp */
   tp = (struct timeval *)(malloc (sizeof (struct timeval)));
   fromlen=(int)sizeof(struct sockaddr);
   udprecordsize=(int)sizeof(struct udprecord);


  lockMe();

  parse_cmd_line(argc,argv);

     if (debug) fprintf(stderr,"udprecordsize=%d\n",udprecordsize);
  
  setup_socket_and_wait();/*ideally we should not come out of this function*/
 
   return(0);
}
