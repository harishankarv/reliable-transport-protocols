#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 0

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg {
  char data[20];
  };

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
   int seqnum;
   int acknum;
   int checksum;
   char payload[20];
    };

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/



/* Statistics 
 * Do NOT change the name/declaration of these variables
 * You need to set the value of these variables appropriately within your code.
 * */



/* Globals 
 * Do NOT change the name/declaration of these variables
 * They are set to zero here. You will need to set them (except WINSIZE) to some proper values.
 * */
float TIMEOUT = 25.0; 
int WINSIZE;         //This is supplied as cmd-line parameter; You will need to read this value but do NOT modify it's value; 
int SND_BUFSIZE = 2000; //Sender's Buffer size
int RCV_BUFSIZE = 2000; //Receiver's Buffer size

float time;

int A_application = 0;
int A_transport = 0;
int B_application = 0;
int B_transport = 0;

void tolayer3(int, struct pkt);
void tolayer5(int, char*);
void starttimer(int, float);
void stoptimer(int);

int calc_checksum (struct pkt);

//A's variables
int sendbase_a;
int nextseqnum_a;
struct pkt* packet_a; // to maintain a buffer at A of packets to be sent to B
float* endtime_a; //to maintain an array of time each packet times out at
int* sentandacked_a; //to maintain whether a packet is sent and acked correctly
int timerison_a; //a flag for whether timer is on

//B's variables
//int expectedseqnum_b;
struct pkt ack_b;
struct msg* packet_b; // to maintain a buffer at B of packets to be sent to layer5
int rcvbase_b;
int* received_b; // to maintain an array of whether packet is received correctly

//struct msg buffer_a[SND_BUFSIZE];
//struct msg buffer_b[RCV_BUFSIZE];


int calc_checksum (struct pkt packet)
{
  //calculating checksum byte by byte
  int i;
  int checksum = 0;
  unsigned char byte;

  for (i = 0; i < sizeof(packet.seqnum); i++) 
  {
      byte = *((unsigned char *)&packet.seqnum + i);
      checksum += byte;
  }

  for (i = 0; i < sizeof(packet.acknum); i++) 
  {
      byte = *((unsigned char *)&packet.acknum + i);
      checksum += byte;
  }

  for (i = 0; i < sizeof(packet.payload); i++) 
  {
      byte = *((unsigned char *)&packet.payload + i);
      checksum += byte;
  }

  return checksum;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
  A_application++;
  printf("[A_output] Time: %f\n", time); 
  printf("[A_output] Received data from layer5: %.*s \n", 20, message.data);
  printf("[A_output] nextseqnum is: %d\n", nextseqnum_a);
  
  //make packet and always add packet to packet_a buffer
  bzero(packet_a + nextseqnum_a, (sizeof(struct pkt)));
  packet_a[nextseqnum_a].seqnum = nextseqnum_a;
  memcpy(packet_a[nextseqnum_a].payload, message.data, 20);
  packet_a[nextseqnum_a].checksum = calc_checksum(packet_a[nextseqnum_a]);
  
  printf("[A_output] packet created with payload: %.*s \n", 20, packet_a[nextseqnum_a].payload);
 
  //send packet if it falls within window
  while ((nextseqnum_a < (sendbase_a + WINSIZE )) && (packet_a[nextseqnum_a].seqnum != -1))
  {
    tolayer3(0, packet_a[nextseqnum_a]);  
    A_transport++;
  
    if (timerison_a == 0)
      {
        printf("[A_output] Timer was NOT on.\n");
        starttimer(0, TIMEOUT);
        timerison_a = 1;  
      }

    endtime_a[nextseqnum_a] = time + TIMEOUT;
    printf("[A_output] Timer for %d will timeout at %f\n", nextseqnum_a, endtime_a[nextseqnum_a]);
    
    printf("[A_output] Packet %d sent.\n", packet_a[nextseqnum_a].seqnum);
    nextseqnum_a++;    
    
  }


  printf("[A_output]  Returning\n");  
  return;
  
}

void B_output(message)  /* need be completed only for extra credit */
  struct msg message;
{

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
  int i;
  float min = 1000000000.0;
  int min_index;

  printf("[A_input] Time: %f\n", time); 
  printf("[A_input] Base: %d\n", sendbase_a);
  printf("[A_input] nextseqnum_a: %d\n", nextseqnum_a);
  printf("[A_input] ACK received: %d\n", packet.acknum);
  
  int checksum = calc_checksum(packet);
  printf("[A_input] Calculated Checksum: %d\n", checksum);
  printf("[A_input] Received Checksum: %d\n", packet.checksum);
  
  if (checksum != packet.checksum)
  {
    printf("[A_input] Checksum does not match. Received ACK is corrupt\n");
    return;
  }
  
  //packet is not corrupt
  printf("[A_input] Packet % d received successfully at Receiver\n", packet.acknum);

  //if  ack received & within window
  if (packet.acknum < sendbase_a + WINSIZE )
  {
    //find minimum in endtime array
    for (i = sendbase_a; i<sendbase_a + WINSIZE; i++)
    {
      if (  (endtime_a[i] < min) && (endtime_a[i] != 0) && (endtime_a[i] != -1) && (sentandacked_a[i] != 1) )
      {
        min = endtime_a[i];
        min_index = i;     
       }
    }  

    printf("[A_input] Minimum endtime: %f\n", min);
    printf("[A_input] Minimum endtime index: %d\n", min_index);
    printf("[A_input] Timer is on: %d\n", timerison_a);
    
    //if ack was for lowest endtime packet, stop timer
    if ( (min_index == packet.acknum) && (timerison_a == 1) )
    {
      stoptimer(0);
      timerison_a = 0;
      printf("[A_input] Timer stopped for Packet % d\n", packet.acknum); 
    }

    sentandacked_a[packet.acknum] = 1; //mark as sent and acked
    endtime_a[packet.acknum] = -1;  //stop timer for that packet
  
    //again find minimum in endtime array. 
    //it will remain same if above if condition was not satisfied.
    min = 1000000000.0;
    for (i = sendbase_a; i<sendbase_a + WINSIZE; i++)
    {
      if (  (endtime_a[i] < min) && (endtime_a[i] != 0 ) && (endtime_a[i] != -1 ) && (sentandacked_a[i] != 1)  )
      {
        min = endtime_a[i];
        min_index = i;
      }
    }  
    printf("[A_input] Minimum endtime: %f\n", min);
    printf("[A_input] Minimum endtime index: %d\n", min_index);
    printf("[A_input] Timer is on: %d\n", timerison_a);

    //effectively starting timer for the packet with least time remaining to timeout
    //time left to timeout = (value of the least time -  current time)  
    if (endtime_a[min_index] != -1)
    {
      stoptimer(0);
      starttimer(0, (min - time));   
      //same as starttimer(0, ((endtime_a[min_index] - time)));
      timerison_a = 1; 
      printf("[A_input] Timer now started for for packet %d.", min_index); 
      printf("[A_input] Will timout in %f, at %f", (min - time), endtime_a[min_index]); 
  
    }  
  
  }
  
  //if ack was for base
  if (sendbase_a == packet.acknum /*&& sentandacked_a[sendbase_a] != 0*/)
  {
    //move the window to the rightmost possible
    for (i = sendbase_a; i<sendbase_a + WINSIZE; i++)
    {
      if (sentandacked_a[i] == 0 /*&& endtime_a[i] != -1*/ ) //break on first non-acked packet
        break;
    }

    sendbase_a = i; //move base to the first non acked packet

  }  

  //now send packets which now fall within window
  //or else these will be sent when next packet is sent from layer5
  

  /*while ((nextseqnum_a < (sendbase_a + WINSIZE )) && (packet_a[nextseqnum_a].seqnum != -1))
  {
    tolayer3(0, packet_a[nextseqnum_a]);  
    A_transport++;
    buffercount_a--;

    if (timerison_a == 0)
      {
        printf("[A_output] Timer was NOT on.\n");
        starttimer(0, TIMEOUT);
        timerison_a == 1;  
      }

    endtime_a[nextseqnum_a] = time + TIMEOUT;
    printf("[A_output] Timer started for %d. Will timeout at %d\n", nextseqnum_a, endtime_a[nextseqnum_a]);
    
    nextseqnum_a++;    
    printf("[A_output] Packet %d sent.\n", packet_a[nextseqnum_a].seqnum);
  }*/

  printf("[A_input] Returning\n");      
  return;
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  printf("[A_timerinterrupt] nextseqnum_a: %d\n", nextseqnum_a);
  printf("[A_timerinterrupt] sendbase_a: %d\n", sendbase_a);
  printf("[A_timerinterrupt] Time: %f\n", time); 
  
  int i;
  float min = 1000000000.0;
  int min_index;

  //calculate minimum value in endtime array to get packet for which timer went off 
  for (i = sendbase_a; i<sendbase_a + WINSIZE; i++)
  {
    // if packet endtime is less than running minimum
    // if packet has been sent (endtime has some value and is not 0)
    // if packet timer has not stopped already 
    // if packet has not been acknowledged (same as condition 3??)

    //try using
    //if ((endtime_a[i] < min) && (endtime_a[i] > 0 )) as
    // endtime = 0 means packet has not been sent; endtime = -1 means packet has been acked.
    if (  (endtime_a[i] < min) && (endtime_a[i] != 0 ) && (endtime_a[i] != -1 ) && (sentandacked_a[i] != 1)  )
    {
      min = endtime_a[i];
      min_index = i;
    }
  }

  
  printf("[A_timerinterrupt] Minimum endtime: %f\n", min);
  printf("[A_timerinterrupt] Minimum endtime index: %d\n", min_index);
  printf("[A_timerinterrupt] Timer is on: %d\n", timerison_a);

  printf("[A_timerinterrupt] Timer went off for packet %d\n", min_index);  
  endtime_a[min_index] = time + TIMEOUT;
  tolayer3(0, packet_a[min_index]);
  A_transport++;
  printf("[A_timerinterrupt] Packet resent. endtime array updated. new timeout: %f\n", endtime_a[min_index]); 
  
  //again calculate minimum value in endtime array to 
  //start timer for packet with lowest timeout in endtime array
  min = 1000000000.0;
  for (i = sendbase_a; i<sendbase_a + WINSIZE; i++)
  {
    if (  (endtime_a[i] < min) && (endtime_a[i] != 0 ) && (endtime_a[i] != -1 ) && (sentandacked_a[i] != 1)  )
    {
      min = endtime_a[i];
      min_index = i;
    }
  }  

  printf("[A_timerinterrupt] Minimum endtime: %f\n", min);
  printf("[A_timerinterrupt] Minimum endtime index: %d\n", min_index);
  printf("[A_timerinterrupt] Timer is on: %d\n", timerison_a);

  //effectively starting timer for the packet with least time remaining to timeout
  //time left to timeout = (value of the least time -  current time)  
  starttimer(0, (min - time));   
  printf("[A_timerinterrupt] Timer now started for for packet %d.", min_index); 
  printf("[A_timerinterrupt] Will timeout in %f, at %f", (min - time), endtime_a[min_index]); 
  
   //same as starttimer(0, ((endtime_a[min_index] - time)));
  
  timerison_a = 1;     
  return;
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  int i;
  packet_a = malloc( (sizeof(struct pkt)) * SND_BUFSIZE);
  bzero(packet_a, ( (sizeof(struct pkt))* SND_BUFSIZE ) );

  for (i =0; i< SND_BUFSIZE; i++)
  {
    packet_a[i].seqnum = -1;
    
  }
  
  endtime_a = malloc( (sizeof(float)) * SND_BUFSIZE);
  bzero(endtime_a, ( (sizeof(float)) * SND_BUFSIZE ) );

  sentandacked_a = malloc( (sizeof(int)) * SND_BUFSIZE);
  bzero(sentandacked_a, ( (sizeof(int)) * SND_BUFSIZE ) );


  sendbase_a = 1;
  nextseqnum_a =1;
  timerison_a = 0;
}


/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{

  B_transport++;
  printf("[B_input] rcvbase_b: %d\n", rcvbase_b); 
  printf("[B_input] Packet received: %.*s \n", 20, packet.payload);
  printf("[B_input] Seqnum: %d\n", packet.seqnum);
  printf("[B_input] Time: %f\n", time); 
  

  int checksum = calc_checksum(packet);
  printf("[B_input] Calculated Checksum: %d\n", checksum);
  printf("[B_input] Received Checksum: %d\n", packet.checksum);

  if (checksum != packet.checksum)
  {
    printf("[B_input] Checksum does not match. Received packet is corrupt\n");
    return;
  }

  //if packet falls within receiver window
  if (  (packet.seqnum >= rcvbase_b) && (packet.seqnum <=(rcvbase_b + WINSIZE - 1))  )
  {
    //send selective ack
    bzero(&ack_b, sizeof(struct pkt));
    ack_b.acknum = packet.seqnum;
    ack_b.checksum = calc_checksum(ack_b);

    printf("[B_input] Sending selective ACK for packet %d\n", ack_b.acknum);
    tolayer3(1, ack_b);  
    //expectedseqnum_b++;

    //buffer packet if not already received
    //or buffer regardless ??
    if (received_b[packet.seqnum] != 1)
    {
      memcpy(packet_b + packet.seqnum, packet.payload, 20);
      received_b[packet.seqnum] = 1;
      printf("[B_input] Packet %d Added to buffer\n", packet.seqnum);
    }

    //if seqnum = rcvbase_b, deliver all packets already buffered starting with rcvbase_b
    //and move window forward by number of packets delivered
    if (packet.seqnum == rcvbase_b )
    {
      while(received_b[rcvbase_b] == 1)
      {
        tolayer5(1, packet_b[rcvbase_b].data);
        B_application++;
        printf("[B_input] Data:  %.*s \n", 20, packet_b[rcvbase_b].data);
        printf("[B_input] Packet %d delivered to layer5. Count: %d\n", rcvbase_b, B_application); 
        rcvbase_b++; //   move window forward
        printf("[B_input] rcvbase_b moved to %d\n", rcvbase_b);
  
       }
    }
   return;

  }

  //send duplicate ack if packet received was within one window behind
  if (  (packet.seqnum >= (rcvbase_b - WINSIZE)) && (packet.seqnum <= (rcvbase_b - 1))  )
  {
  bzero(&ack_b, sizeof(struct pkt));
  ack_b.acknum = packet.seqnum;
  ack_b.checksum = calc_checksum(ack_b);

  printf("[B_input] Sending duplicate ACK for packet %d\n", ack_b.acknum);
  tolayer3(1, ack_b);  

  return;
  
  }


  
  return;  
}

/* called when B's timer goes off */
void B_timerinterrupt()
{

}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  //int i;
  packet_b = malloc( (sizeof(struct msg)) * RCV_BUFSIZE);
  bzero(packet_b, ( (sizeof(struct msg))* RCV_BUFSIZE ) );

  received_b = malloc( (sizeof(int)) * RCV_BUFSIZE);
  bzero(received_b, ( (sizeof(int)) * RCV_BUFSIZE ) );

  rcvbase_b = 1;
  //expectedseqnum_b = 1;

  bzero(&ack_b, sizeof(struct pkt));
  ack_b.acknum = 0;
  ack_b.checksum = calc_checksum(ack_b);

 }

/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
    and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
    interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

struct event {
   float evtime;           /* event time */
   int evtype;             /* event type code */
   int eventity;           /* entity where event occurs */
   struct pkt *pktptr;     /* ptr to packet (if any) assoc w/ this event */
   struct event *prev;
   struct event *next;
 };
struct event *evlist = NULL;   /* the event list */

//forward declarations
void init();
void generate_next_arrival();
void insertevent(struct event*);

/* possible events: */
#define  TIMER_INTERRUPT 0  
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2

#define  OFF             0
#define  ON              1
#define   A    0
#define   B    1



int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */ 
int nsimmax = 0;           /* number of msgs to generate, then stop */
float time = 0.000;
float lossprob = 0.0;	   /* probability that a packet is dropped */
float corruptprob = 0.0;   /* probability that one bit is packet is flipped */
float lambda = 0.0; 	   /* arrival rate of messages from layer 5 */
int ntolayer3 = 0; 	   /* number sent into layer 3 */
int nlost = 0; 	  	   /* number lost in media */
int ncorrupt = 0; 	   /* number corrupted by media*/

/**
 * Checks if the array pointed to by input holds a valid number.
 *
 * @param  input char* to the array holding the value.
 * @return TRUE or FALSE
 */
int isNumber(char *input)
{
    while (*input){
        if (!isdigit(*input))
            return 0;
        else
            input += 1;
    }

    return 1;
}

int main(int argc, char **argv)
{
   struct event *eventptr;
   struct msg  msg2give;
   struct pkt  pkt2give;
   
   int i,j;
   char c;

   int opt;
   int seed;

   //Check for number of arguments
   if(argc != 5){
   	fprintf(stderr, "Missing arguments\n");
	printf("Usage: %s -s SEED -w WINDOWSIZE\n", argv[0]);
   	return -1;
   }

   /* 
    * Parse the arguments 
    * http://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html 
    */
   while((opt = getopt(argc, argv,"s:w:")) != -1){
       switch (opt){
           case 's':   if(!isNumber(optarg)){
                           fprintf(stderr, "Invalid value for -s\n");
                           return -1;
                       }
                       seed = atoi(optarg);
                       break;

           case 'w':   if(!isNumber(optarg)){
                           fprintf(stderr, "Invalid value for -w\n");
                           return -1;
                       }
                       WINSIZE = atoi(optarg);
                       break;

           case '?':   break;

           default:    printf("Usage: %s -s SEED -w WINDOWSIZE\n", argv[0]);
                       return -1;
       }
   }
  
   init(seed);
   A_init();
   B_init();
   
   while (1) {
        eventptr = evlist;            /* get next event to simulate */
        if (eventptr==NULL)
           goto terminate;
        evlist = evlist->next;        /* remove this event from event list */
        if (evlist!=NULL)
           evlist->prev=NULL;
        if (TRACE>=2) {
           printf("\nEVENT time: %f,",eventptr->evtime);
           printf("  type: %d",eventptr->evtype);
           if (eventptr->evtype==0)
	       printf(", timerinterrupt  ");
             else if (eventptr->evtype==1)
               printf(", fromlayer5 ");
             else
	     printf(", fromlayer3 ");
           printf(" entity: %d\n",eventptr->eventity);
           }
        time = eventptr->evtime;        /* update time to next event time */
        if (nsim==nsimmax)
	  break;                        /* all done with simulation */
        if (eventptr->evtype == FROM_LAYER5 ) {
            generate_next_arrival();   /* set up future arrival */
            /* fill in msg to give with string of same letter */    
            j = nsim % 26; 
            for (i=0; i<20; i++)  
               msg2give.data[i] = 97 + j;
            if (TRACE>2) {
               printf("          MAINLOOP: data given to student: ");
                 for (i=0; i<20; i++) 
                  printf("%c", msg2give.data[i]);
               printf("\n");
	     }
            nsim++;
            if (eventptr->eventity == A) 
               A_output(msg2give);  
             else
               B_output(msg2give);  
            }
          else if (eventptr->evtype ==  FROM_LAYER3) {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i=0; i<20; i++)  
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
	    if (eventptr->eventity ==A)      /* deliver packet by calling */
   	       A_input(pkt2give);            /* appropriate entity */
            else
   	       B_input(pkt2give);
	    free(eventptr->pktptr);          /* free the memory for packet */
            }
          else if (eventptr->evtype ==  TIMER_INTERRUPT) {
            if (eventptr->eventity == A) 
	       A_timerinterrupt();
             else
	       B_timerinterrupt();
             }
          else  {
	     printf("INTERNAL PANIC: unknown event type \n");
             }
        free(eventptr);
        }

terminate:
	//Do NOT change any of the following printfs
	printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n",time,nsim);
	
	printf("\n");
	printf("Protocol: GBN\n");
	printf("[PA2]%d packets sent from the Application Layer of Sender A[/PA2]\n", A_application);
	printf("[PA2]%d packets sent from the Transport Layer of Sender A[/PA2]\n", A_transport);
	printf("[PA2]%d packets received at the Transport layer of Receiver B[/PA2]\n", B_transport);
	printf("[PA2]%d packets received at the Application layer of Receiver B[/PA2]\n", B_application);
	printf("[PA2]Total time: %f time units[/PA2]\n", time);
	printf("[PA2]Throughput: %f packets/time units[/PA2]\n", B_application/time);
	return 0;
}



void init(int seed)                         /* initialize the simulator */
{
  int i;
  float sum, avg;
  float jimsrand();
  
  
   printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
   printf("Enter the number of messages to simulate: ");
   scanf("%d",&nsimmax);
   printf("Enter  packet loss probability [enter 0.0 for no loss]:");
   scanf("%f",&lossprob);
   printf("Enter packet corruption probability [0.0 for no corruption]:");
   scanf("%f",&corruptprob);
   printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
   scanf("%f",&lambda);
   printf("Enter TRACE:");
   scanf("%d",&TRACE);

   srand(seed);              /* init random number generator */
   sum = 0.0;                /* test random number generator for students */
   for (i=0; i<1000; i++)
      sum=sum+jimsrand();    /* jimsrand() should be uniform in [0,1] */
   avg = sum/1000.0;
   if (avg < 0.25 || avg > 0.75) {
    printf("It is likely that random number generation on your machine\n" ); 
    printf("is different from what this emulator expects.  Please take\n");
    printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
    exit(0);
    }

   ntolayer3 = 0;
   nlost = 0;
   ncorrupt = 0;

   time=0.0;                    /* initialize time to 0.0 */
   generate_next_arrival();     /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand() 
{
  double mmm = 2147483647;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
  float x;                   /* individual students may need to change mmm */ 
  x = rand()/mmm;            /* x should be uniform in [0,1] */
  return(x);
}  

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/
 
void generate_next_arrival()
{
   double x,log(),ceil();
   struct event *evptr;
    //char *malloc();
   float ttime;
   int tempint;

   if (TRACE>2)
       printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");
 
   x = lambda*jimsrand()*2;  /* x is uniform on [0,2*lambda] */
                             /* having mean of lambda        */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + x;
   evptr->evtype =  FROM_LAYER5;
   if (BIDIRECTIONAL && (jimsrand()>0.5) )
      evptr->eventity = B;
    else
      evptr->eventity = A;
   insertevent(evptr);
} 


void insertevent(p)
   struct event *p;
{
   struct event *q,*qold;

   if (TRACE>2) {
      printf("            INSERTEVENT: time is %lf\n",time);
      printf("            INSERTEVENT: future time will be %lf\n",p->evtime); 
      }
   q = evlist;     /* q points to header of list in which p struct inserted */
   if (q==NULL) {   /* list is empty */
        evlist=p;
        p->next=NULL;
        p->prev=NULL;
        }
     else {
        for (qold = q; q !=NULL && p->evtime > q->evtime; q=q->next)
              qold=q; 
        if (q==NULL) {   /* end of list */
             qold->next = p;
             p->prev = qold;
             p->next = NULL;
             }
           else if (q==evlist) { /* front of list */
             p->next=evlist;
             p->prev=NULL;
             p->next->prev=p;
             evlist = p;
             }
           else {     /* middle of list */
             p->next=q;
             p->prev=q->prev;
             q->prev->next=p;
             q->prev=p;
             }
         }
}

void printevlist()
{
  struct event *q;
  int i;
  printf("--------------\nEvent List Follows:\n");
  for(q = evlist; q!=NULL; q=q->next) {
    printf("Event time: %f, type: %d entity: %d\n",q->evtime,q->evtype,q->eventity);
    }
  printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(AorB)
int AorB;  /* A or B is trying to stop timer */
{
 struct event *q,*qold;

 if (TRACE>2)
    printf("          STOP TIMER: stopping timer at %f\n",time);
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
 for (q=evlist; q!=NULL ; q = q->next) 
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) { 
       /* remove this event */
       if (q->next==NULL && q->prev==NULL)
             evlist=NULL;         /* remove first and only event on list */
          else if (q->next==NULL) /* end of list - there is one in front */
             q->prev->next = NULL;
          else if (q==evlist) { /* front of list - there must be event after */
             q->next->prev=NULL;
             evlist = q->next;
             }
           else {     /* middle of list */
             q->next->prev = q->prev;
             q->prev->next =  q->next;
             }
       free(q);
       return;
     }
  printf("Warning: unable to cancel your timer. It wasn't running.\n");
}


void starttimer(AorB,increment)
int AorB;  /* A or B is trying to stop timer */
float increment;
{

 struct event *q;
 struct event *evptr;
 //char *malloc();

 if (TRACE>2)
    printf("          START TIMER: starting timer at %f\n",time);
 /* be nice: check to see if timer is already started, if so, then  warn */
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
   for (q=evlist; q!=NULL ; q = q->next)  
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) { 
      printf("Warning: attempt to start a timer that is already started\n");
      return;
      }
 
/* create future event for when timer goes off */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + increment;
   evptr->evtype =  TIMER_INTERRUPT;
   evptr->eventity = AorB;
   insertevent(evptr);
} 


/************************** TOLAYER3 ***************/
void tolayer3(AorB,packet)
int AorB;  /* A or B is trying to stop timer */
struct pkt packet;
{
 struct pkt *mypktptr;
 struct event *evptr,*q;
 //char *malloc();
 float lastime, x, jimsrand();
 int i;


 ntolayer3++;

 /* simulate losses: */
 if (jimsrand() < lossprob)  {
      nlost++;
      if (TRACE>0)    
	printf("          TOLAYER3: packet being lost\n");
      return;
    }  

/* make a copy of the packet student just gave me since he/she may decide */
/* to do something with the packet after we return back to him/her */ 
 mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
 mypktptr->seqnum = packet.seqnum;
 mypktptr->acknum = packet.acknum;
 mypktptr->checksum = packet.checksum;
 for (i=0; i<20; i++)
    mypktptr->payload[i] = packet.payload[i];
 if (TRACE>2)  {
   printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
	  mypktptr->acknum,  mypktptr->checksum);
    for (i=0; i<20; i++)
        printf("%c",mypktptr->payload[i]);
    printf("\n");
   }

/* create future event for arrival of packet at the other side */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtype =  FROM_LAYER3;   /* packet will pop out from layer3 */
  evptr->eventity = (AorB+1) % 2; /* event occurs at other entity */
  evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */
/* finally, compute the arrival time of packet at the other end.
   medium can not reorder, so make sure packet arrives between 1 and 10
   time units after the latest arrival time of packets
   currently in the medium on their way to the destination */
 lastime = time;
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
 for (q=evlist; q!=NULL ; q = q->next) 
    if ( (q->evtype==FROM_LAYER3  && q->eventity==evptr->eventity) ) 
      lastime = q->evtime;
 evptr->evtime =  lastime + 1 + 9*jimsrand();
 


 /* simulate corruption: */
 if (jimsrand() < corruptprob)  {
    ncorrupt++;
    if ( (x = jimsrand()) < .75)
       mypktptr->payload[0]='Z';   /* corrupt payload */
      else if (x < .875)
       mypktptr->seqnum = 999999;
      else
       mypktptr->acknum = 999999;
    if (TRACE>0)    
	printf("          TOLAYER3: packet being corrupted\n");
    }  

  if (TRACE>2)  
     printf("          TOLAYER3: scheduling arrival on other side\n");
  insertevent(evptr);
} 

void tolayer5(AorB,datasent)
  int AorB;
  char datasent[20];
{
  int i;  
  if (TRACE>2) {
     printf("          TOLAYER5: data received: ");
     for (i=0; i<20; i++)  
        printf("%c",datasent[i]);
     printf("\n");
   }
  
}
