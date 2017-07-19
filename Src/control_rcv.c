#include "stab_rcv.h"

/* send control packet to sender*/
void send_pkt(int state,int ttl_index)
{
  struct timeval tp_snd;
  double tmp_var;/* variable used in creating control packet*/

  (void) gettimeofday (&tp_snd, (struct timezone *) 0);

  /*timestamping packet*/
  pkt->timesec=htonl((u_int32_t) tp_snd.tv_sec);
  pkt->timeusec=htonl((u_int32_t) tp_snd.tv_usec);

  /*incrementing packet number*/
  cur_num++;
  pkt->num=htonl((u_int32_t) cur_num);

  /*request number should already have been incremented*/
  pkt->request_num=htonl((u_int32_t) request_num);
  if (debug) fprintf(stderr,"SEND PKT, state=%d,request_num=%d,cur_num=%d\n",state,request_num,cur_num);

  pkt->ttl_index=htonl((u_int32_t) ttl_index);
  switch(state)
  {
  case REQ_CONN:
    if(debug) fprintf(stderr,"sending REQ_CONN\n");

    pkt->request_type=htonl((u_int32_t)REQ_CONN);
    break;

  case CHALL_REPLY:
    if(debug) fprintf(stderr,"sending CHALL_REPLY\n");

    pkt->request_type=htonl((u_int32_t)CHALL_REPLY);

    /*this must not change during the rest of the connection*/
    pkt->chal_no=htonl((u_int32_t)chal_no);
    break;

  case FIND_TTL:
    if(debug) fprintf(stderr,"sending max TTL\n");

    pkt->request_type=htonl((u_int32_t)FIND_TTL);

    pkt->num_ttl=htonl((u_int32_t)num_ttl);
    pkt->ttl_gap=htonl((u_int32_t)ttl_gap);
    tmp_var=1000000.0*inter_chirp_time;
    if(debug) fprintf(stderr,"update rates, inter_chirp=%f microsecs \n",tmp_var);

    pkt->inter_chirp_time=htonl ((u_int32_t) tmp_var);/* in us*/
    tmp_var=low_rate[ttl_index]*10000.0;
    pkt->low_rate=htonl ((u_int32_t)tmp_var);
    pkt->num_interarrival=htonl ((u_int32_t)num_interarrival[ttl_index]);

    tmp_var=spread_factor*10000.0;
    pkt->spread_factor=htonl ((u_int32_t) tmp_var);
    pkt->pktsize=htonl ((u_int32_t) pktsize);
    pkt->jumbo=htonl ((u_int32_t) jumbo);

    break;


  case UPDATE_RATES:
    pkt->request_type=htonl((u_int32_t)UPDATE_RATES);

    tmp_var=1000000.0*inter_chirp_time;
    if(debug) fprintf(stderr,"update rates, inter_chirp=%f microsecs\n",tmp_var);

    pkt->inter_chirp_time=htonl ((u_int32_t) tmp_var);/* in us*/
    tmp_var=low_rate[ttl_index]*10000.0;
    pkt->low_rate=htonl ((u_int32_t)tmp_var);
    pkt->num_interarrival=htonl ((u_int32_t)num_interarrival[ttl_index]);

    tmp_var=spread_factor*10000.0;
    pkt->spread_factor=htonl ((u_int32_t) tmp_var);
    pkt->pktsize=htonl ((u_int32_t) pktsize);
    pkt->jumbo=htonl ((u_int32_t) jumbo);
    break;
  case STOP:
    pkt->request_type=htonl((u_int32_t)STOP);
    break;
  case RECV_OK:
    pkt->request_type=htonl((u_int32_t)RECV_OK);
    break;

  default:
    perror("stab_rcv: error in case of send_pkt\n");
    exit(0);
    break;
  }
  /* figure out checksum and write to buffer */
  pkt->checksum=htonl(gen_crc_rcv2snd(pkt));
  if (debug) fprintf(stderr,"END SEND PKT, state=%d,request_num=%d\n",state,request_num);

  cc = write (soudp, (char *)pkt,sizeof(struct control_rcv2snd));

  /*       cc = sendto (soudp,data_snd,sizeof(struct control_rcv2snd),0,(struct sockaddr *)&src,sizeof(src));*/

  if (cc<0){ fprintf(stderr,"write error,cc=%d\n",cc);
    exit(0);}
  if(debug) fprintf(stderr,"wrote packet,cc=%d\n",cc);
  return;

}

/* receive the challenge packet and reply  */
void recv_chall_pkt()
{
  int cc;

  if(debug) fprintf(stderr,"In recv_chall_pkt\n");

  cc = read(soudp, data, MAXMESG);


  if (cc < 0) {
    perror ("stab_rcv: read");
    exit(0);
  }
  else
    if(debug) fprintf(stderr,"Got chall packet,cc=%d,chal_no=%d\n",cc,(int)ntohl(udprecord->chal_no));


  if (check_crc_snd2rcv(udprecord))
  {
    if(debug) fprintf(stderr,"check crc successful \n");
    if (ntohl(udprecord->request_num)==request_num)
    {
      chal_no=ntohl(udprecord->chal_no);
      if(debug) fprintf(stderr,"chal no=%u\n",chal_no);
      request_num++;
      send_pkt(CHALL_REPLY,0);
      ack_not_rec_count=0;
      state=CHALL_REPLY;
    }
    else{if(debug) fprintf(stderr,"req num, snd=%d,rcv=%d\n",(int)ntohl(udprecord->request_num),request_num);}
  }
  else{if(debug) fprintf(stderr,"crc wrong\n");}

  return;
}

/*receive the packets with varying ttl, these are packets with chirp number=0*/
void recv_ttl_pkt()
{
  int cc,chirp_num;
  int recv_ttl;

  if(debug) fprintf(stderr,"in recv_ttl_pkt \n");

#ifdef HAVE_SO_TIMESTAMP
  cc=recvmsg(soudp,&msg,0);
#else
  cc = read(soudp, data, MAXMESG);
#endif

  if(debug) fprintf(stderr,"after read \n");

  if (cc <= 0) {
    perror ("stab_rcv: read");
    exit(0);
  }

  if (check_crc_snd2rcv(udprecord))
  {
    if (ntohl(udprecord->request_num)==request_num && ntohl(udprecord->chal_no)==chal_no)
    {

      if (state!=FIND_TTL && state!=FOUND_TTL)
        state=FIND_TTL;

      ack_not_rec_count=0;

      chirp_num=ntohl ((u_int32_t) udprecord->chirp_num);
      if (chirp_num!=0)/*this is a chirp packet, not a ttl estimation one*/
      {
        state=CHIRPS_STARTED;
        first_packet=1;
        return;
      }

      recv_ttl= (int)ntohl ((u_int32_t) udprecord->big_pkt_ttl);

      if (state==FIND_TTL)
      {
        if (min_ttl>recv_ttl)
          min_ttl=recv_ttl;
      }
      if (debug)  fprintf(stderr,"Recvd ttl=%d,minttl=%d\n",recv_ttl,min_ttl);
    }
    else{if(debug) fprintf(stderr,"req num wrong, snd=%d,rcv=%d\n",(int)ntohl(udprecord->request_num),request_num);}
  }
  else{if(debug) fprintf(stderr,"crc wrong\n");}
  return;
}

/* keep checking for initial handshake reply */
void run_select(unsigned long time)
{
  int maxfd;
  struct   timeval tp_select,tp_start;
  fd_set rset;

  tp_select.tv_sec=time/1000000;
  tp_select.tv_usec=time%1000000;

  FD_ZERO(&rset);
  FD_SET(soudp,&rset);

  (void) gettimeofday (&tp_start, (struct timezone *) 0);
  if(debug) fprintf(stderr,"running select\n");
  maxfd=soudp+1;

  select(maxfd,&rset,NULL,NULL,&tp_select);

  if (FD_ISSET(soudp,&rset))
  {
    if(debug) fprintf(stderr,"Got packet\n");
    switch(state)
    {
    case REQ_CONN:
      recv_chall_pkt();
      break;
      /* from now on we do not check for checksum at the receiver, ideally we should to eliminate packets from fake spoofed sender packets. In future we must perform a read here along with crc check. Also in receive_chirp_pkts().*/

    case CHALL_REPLY:
    case FIND_TTL:
    case FOUND_TTL:
      recv_ttl_pkt();
      break;

    default:
      break;
    }
  }
  else
  {
    switch(state)
    {
    case REQ_CONN:
      send_pkt(REQ_CONN,0);
      break;
    case CHALL_REPLY:
      send_pkt(CHALL_REPLY,0);
      break;
    case FIND_TTL:
      num_ttl=ceil((double)min_ttl/(double)ttl_gap);
      if (debug)
        fprintf(stderr,"num_ttl =%d\n",num_ttl);

      create_arrays();/*in alloc_rcv.c*/
      state=FOUND_TTL;
      request_num++;/*increment request num for new packet*/

    case FOUND_TTL:
      send_pkt(FIND_TTL,0);
      break;
    default:
      break;
    }
    ack_not_rec_count++;
    if (ack_not_rec_count>max_ack_not_received_count)
    {
      if(debug) fprintf(stderr,"ack not received, state=%d\n",state);
      exit(0);
    }

  }
  return;
}


/* contact sender and request connection */
void initiate_connection()
{

  int	rcv_size = MAXRCVBUF;	/* socket receive buffer size */

  int flag_on_recv;/*flag for setting SO_REUSEADDR*/

  /* fix msg for recvmsg*/

#ifdef HAVE_SO_TIMESTAMP
  msg.msg_control=control_un.control;
  msg.msg_controllen=sizeof(control_un.control);
  msg.msg_name=NULL;
  msg.msg_namelen=0;

  msg.msg_iov=iov;
  iov[0].iov_base=(void *)data;
  iov[0].iov_len=MAXMESG;
  msg.msg_iovlen=1;
#endif


  /* create a socket to receive/send UDP packets */
  soudp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (soudp < 0) {
    perror("stab_rcv: socket");
    exit(1);
  }

  /* initialize socket address for connection */
  src.sin_family = AF_INET;
  src.sin_port = htons(sndPort);

  /*the following is to use socket even if already bound */
  flag_on_recv = YES;

  if (setsockopt(soudp, SOL_SOCKET, SO_REUSEADDR,(char *) &flag_on_recv,
                 sizeof(flag_on_recv))<0){
    perror ("stab_rcv: setsockopt failed");
    (void) exit (1);
  }

  /* set the socket receive buffer to maximum possible.
   * this will minimize the chance of losses at the endpoint */
  if (setsockopt(soudp, SOL_SOCKET, SO_RCVBUF, (char *)&rcv_size,
                 sizeof (rcv_size)) < 0) {
    perror ("stab_rcv: receive buffer option");
    exit (1);
  }

  /* kernel timestamp option for solaris */
#ifdef HAVE_SO_TIMESTAMP
  if (setsockopt(soudp, SOL_SOCKET, SO_TIMESTAMP,&flag_on_recv, sizeof(flag_on_recv))<0){
    perror ("stab_rcv: setsockopt SO_TIMESTAMP failed");
    (void) exit (1);
  }
#endif

  /* connect so that we only receive packets from sender */
  if (connect (soudp, (struct sockaddr *) &src, sizeof (src)) < 0) {
    perror ("stab_rcv: could not connect to the sender");
    (void) exit (1);
  }
  else{
    if(debug) fprintf(stderr,"connected to sender\n");
  }

  /* have send packet point to buffer and initialize fields*/
  pkt=(struct control_rcv2snd *) data_snd;

  bzero((char *)pkt,sizeof(struct control_rcv2snd));
  request_num=0;
  cur_num=(u_int32_t)(rand()/2);/*initial packet number is random*/
  /* pkt->num_interarrival=htonl((u_int32_t)num_interarrival[0]);

     tmp_var=inter_chirp_time*1000000.0;
     pkt->inter_chirp_time=htonl ((u_int32_t) tmp_var);*//* in us*/
  /* tmp_var=low_rate[0]*10000.0;
     pkt->low_rate=htonl ((u_int32_t) tmp_var);
     tmp_var=high_rate[0]*10000.0;
  */

  pkt->spread_factor=htonl ((u_int32_t) (spread_factor*10000.0));
  pkt->pktsize=htonl ((u_int32_t ) pktsize);
  pkt->jumbo=htonl ((u_int32_t ) jumbo);

  /* request connection */

  state=REQ_CONN;
  send_pkt(state,0);

  while (state!=CHIRPS_STARTED)
  {
    if (debug) fprintf(stderr,"RTTUSEC=%d\n",RTTUSEC);
    run_select((unsigned long)RTTUSEC);
    if (debug) fprintf(stderr,"state=%d\n",state);
  }
  return;
}
