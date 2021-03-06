#include <stdio.h>
#include <stdlib.h>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).

 MODIFICATIONS by jsevilla274
   - include stdlib
   - comment out malloc redefinitions in emulator code
   - slightly alter function signatures in emulator code
   - move certain definitions and declarations to resolve compiler errors
**********************************************************************/

#define BIDIRECTIONAL 0    /* change to 1 if you're doing extra credit */
                           /* and write a routine called B_output */                        

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

/* included these definition and declarations to resolve compiler errors */

struct event {
   float evtime;           /* event time */
   int evtype;             /* event type code */
   int eventity;           /* entity where event occurs */
   struct pkt *pktptr;     /* ptr to packet (if any) assoc w/ this event */
   struct event *prev;
   struct event *next;
 };

void starttimer(int AorB, float increment);
void stoptimer(int AorB);
void tolayer3(int AorB, struct pkt packet);
void tolayer5(int AorB, char datasent[20]);
void init();
void generate_next_arrival();
void insertevent(struct event *p);

/********* STUDENT CODE START *********/

#define DATA_LEN 20   /* max length of layer 5 data */
#define ENTITY_A 0
#define ENTITY_B 1
#define EMPTY_PAYLOAD -1
#define TIMEOUT_LEN 200.0 /* timeout for retransmission. this value worked well for me
                             but your mileage may vary; tweak as necessary.*/
#define A_WINSIZE 5

int A_base = 1;
int A_nextseq = 1;
int B_expectedseq = 1;
struct pkt *A_sendwin[A_WINSIZE]; // array of pkt pointers
struct pkt *B_currack = NULL;

struct pkt *make_pkt(int seqnum, int acknum, char *payload)
{
  struct pkt *packet = malloc(sizeof(struct pkt));
  packet->seqnum = seqnum;
  packet->acknum = acknum;
  int checksum = seqnum + acknum;
  if (payload == NULL)
  {
    packet->payload[0] = EMPTY_PAYLOAD;
  }
  else // has payload
  {
    // generate checksum and populate packet payload
    for (int i = 0; i < DATA_LEN; i++)
    {
      checksum += (int) payload[i];
      packet->payload[i] = payload[i];
    }
  }
  packet->checksum = checksum;
  return packet;
}

/* returns 1 if a packet is corrupt, 0 otherwise */
int pkt_is_corrupt(struct pkt *packet)
{
  int checksum = packet->seqnum + packet->acknum;
  if ((int)(packet->payload[0]) != EMPTY_PAYLOAD)
  {
    for (int i = 0; i < DATA_LEN; i++)
    {
      checksum += packet->payload[i];
    }
  }

  return packet->checksum != checksum;
}

/* prints the contents of a packet, for debugging */
void pkt_info(struct pkt *packet)
{
  printf("\n[pkt_info]\nSEQ#: %d\nACK#: %d\nPayload: ", packet->seqnum, packet->acknum);
  if (packet->payload[0] == EMPTY_PAYLOAD)
  {
    printf("EMPTY\n");
  }
  else
  {
    printf("\n");
    for (int i = 0; i < DATA_LEN; i++)
    {
      printf("%d: %c\n", i, packet->payload[i]);
    }
  }
}

/* prints the seqnums of the packets in the given send window */
void win_info(struct pkt *window[], int winlen)
{
  printf("sendwin: [");
  for (int i = 0; i < winlen; i++)
  {
    if (window[i] == NULL) // empty
    {
      printf(" N");
    }
    else
    {
      printf(" %d", window[i]->seqnum);
    }
  }
  printf(" ]\n");
  
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
  if (A_nextseq < A_base + A_WINSIZE)  // there is space in sendwin
  {
    // create new packet with payload in first empty space of sendwin
    int pkt_index;
    for (pkt_index = 0; pkt_index < A_WINSIZE; pkt_index++)
    {
      if (A_sendwin[pkt_index] == NULL) // not occupied by a packet
      {
        // for now, acknum will be zero because A is strictly a sender
        A_sendwin[pkt_index] = make_pkt(A_nextseq, 0, message.data);
        break;
      }
    }
    printf("A sends PKT %d into the network and starts the timer.\n", A_nextseq);
    win_info(A_sendwin, A_WINSIZE);

    // send currpkt by value
    tolayer3(ENTITY_A, *A_sendwin[pkt_index]);

    if (A_nextseq == A_base)  // is first pkt we sent since stopping timer
    {
      starttimer(ENTITY_A, TIMEOUT_LEN);
    }

    A_nextseq++;
  }
  else // exceeds sending window
  {
    printf("A's sending window is full, A drops Layer 5 message.\n");
  }
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */
void B_output(struct msg message)  
{
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
  int badpkt = 0;
  if (packet.acknum < A_base)
  {
    printf("A receives ACK %d, which falls outside of the sending window; A does nothing.\n", packet.acknum);
    badpkt = 1;
  }
  else if (pkt_is_corrupt(&packet))
  {
    printf("A receives a corrupt ACK, A does nothing.\n");
    badpkt = 1;
  }

  if (!badpkt)
  {
    // passing first badpkt check implies a new ACK has been received
    stoptimer(ENTITY_A); 
    printf("A receives ACK %d, which is new. A stops its timer.\n", packet.acknum);

    // base goes up depending on ACK
    A_base = packet.acknum + 1;

    // Delete ACKed packets in sendwin
    //
    // NOTE: In theory, we would want to keep a data structure like a queue
    // to speed up deleting ACKed packets, but since our sending window is
    // small it is sufficient to perform a linear search
    for (int i = 0; i < A_WINSIZE; i++)
    {
      if (A_sendwin[i] != NULL && A_sendwin[i]->seqnum <= packet.acknum)
      {
        free(A_sendwin[i]);
        A_sendwin[i] = NULL;
      }
    }

    if (A_base != A_nextseq)  // packets still in transit / send window not empty
    {
      // restart timer
      printf("A infers packets still in transit, A restarts timer.\n");
      starttimer(ENTITY_A, TIMEOUT_LEN);
    }
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  printf("A has timed out.\n");

  // reorder sendwin for retransmission
  struct pkt *orderedwin[A_WINSIZE];
  int ordered_index;
  int retransmit_count = 0;
  for (int j = 0; j < A_WINSIZE; j++)
  {
    if (A_sendwin[j] != NULL)
    {
      // order packets by seqnum in ascending order
      // NOTE: this works off the fact that any seqnum modulo base
      // will always produce an index within [0, WINSIZE)
      ordered_index = (A_sendwin[j]->seqnum) % A_base;
      orderedwin[ordered_index] = A_sendwin[j];
      retransmit_count++;
    }
  }
  
  // resend un-ACKed packets
  for (int i = 0; i < retransmit_count; i++)
  {
    // resend lost packet by value
    printf("A resends PKT %d.\n", orderedwin[i]->seqnum);
    tolayer3(ENTITY_A, *(orderedwin[i]));
  }

  // restart timer
  printf("A restarts timer.\n");
  starttimer(ENTITY_A, TIMEOUT_LEN);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  // Initialize window to null pkt pointers
  for (int i = 0; i < A_WINSIZE; i++)
  {
    A_sendwin[i] = NULL;
  }

  // printf("Checking A's initial sendwin contents.\n");
  // win_info(A_sendwin, A_WINSIZE);
}

/* called from layer 3, when a packet arrives for layer 4 at B*/
/* NOTE: I believe one major assumption we make here is the receiver (B)
is accepting packets and ACKing them in sequence as opposed to storing them
in some buffer. While this works under the current context, in real life your
receiver would necessarily buffer input packets and then ACK them because you 
can't make packets in the transmission medium wait.*/
void B_input(struct pkt packet)
{
  int badpkt = 0;
  if (packet.seqnum != B_expectedseq)
  {
    printf("B receives out of order PKT %d, ", packet.seqnum);
    badpkt = 1;
  }
  else if (pkt_is_corrupt(&packet))
  {
    printf("B receives a corrupt packet, ");
    badpkt = 1;
  }

  if (!badpkt)
  {
    printf("B receives PKT %1$d, sends ACK %1$d.\n", B_expectedseq);
    /* NOTE: specs say tolayer5 is expecting a struct msg, but we're passing
    a byte array as the code expects */
    tolayer5(ENTITY_B, packet.payload);

    // create a new ack packet with no payload
    free(B_currack);
    B_currack = make_pkt(0, B_expectedseq, NULL); // for now, seqnum will be zero because B is strictly a receiver

    // advance expected sequence number
    B_expectedseq++;
  }
  else
  {
    // ACK the last correctly received packet
    printf("resends ACK %d. (ACKing last correctly received PKT)\n", B_currack->acknum);
  }

  // send ack by value
  tolayer3(ENTITY_B, *B_currack);
}

/* called when B's timer goes off */
void B_timerinterrupt()
{
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  /* Since sender (A) base starts at 1, we make a "dummy" ACK 0 so that the
  receiver (B) has something to send. */
  B_currack = make_pkt(0, 0, NULL);

  /* NOTE: rdt3.0 had no use for sending an ACK for the last properly received
  packet because the sender took no action unless the ACK matched with the current
  sequence number. Go-Back-N senders, on the other hand, move their base
  depending on the ACK received. This leads to performance increases: Suppose the
  sender's base is N and the last packet it sent was N+4. If the sender receives
  an ACK N+3, then the sender can still increase it's base and reduce total
  retransmissions even if not all of its packets were ACKed*/
}

/********* STUDENT CODE END *********/

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

struct event *evlist = NULL;   /* the event list */

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
float lossprob;            /* probability that a packet is dropped  */
float corruptprob;         /* probability that one bit is packet is flipped */
float lambda;              /* arrival rate of messages from layer 5 */   
int   ntolayer3;           /* number sent into layer 3 */
int   nlost;               /* number lost in media */
int ncorrupt;              /* number corrupted by media*/

void main()
{
   struct event *eventptr;
   struct msg  msg2give;
   struct pkt  pkt2give;
   
   int i,j;
   char c; 
  
   init();
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
   printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n",time,nsim);
}



void init()                         /* initialize the simulator */
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

   srand(9999);              /* init random number generator */
   sum = 0.0;                /* test random number generator for students */
   for (i=0; i<1000; i++)
      sum=sum+jimsrand();    /* jimsrand() should be uniform in [0,1] */
   avg = sum/1000.0;
   if (avg < 0.25 || avg > 0.75) {
    printf("It is likely that random number generation on your machine\n" ); 
    printf("is different from what this emulator expects.  Please take\n");
    printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
    exit(1);
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
    // char *malloc();
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


void insertevent(struct event *p)
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
void stoptimer(int AorB) /* A or B is trying to stop timer */
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


void starttimer(int AorB, float increment)  /* A or B is trying to stop timer */
{

 struct event *q;
 struct event *evptr;
//  char *malloc();

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
void tolayer3(int AorB, struct pkt packet) /* A or B is trying to stop timer */
{
 struct pkt *mypktptr;
 struct event *evptr,*q;
//  char *malloc();
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

void tolayer5(int AorB,char datasent[20])
{
  int i;  
  if (TRACE>2) {
     printf("          TOLAYER5: data received: ");
     for (i=0; i<20; i++)  
        printf("%c",datasent[i]);
     printf("\n");
   }
  
}
