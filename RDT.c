#include <stdio.h>
#include <stdlib.h>

#pragma warning(disable:4996)

int diu1;
int diu2;
int loss1;
int loss2;
float lossprob;            /* probability that a packet is dropped  */
float corruptprob;         /* probability that one bit is packet is flipped */

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
**********************************************************************/


//-----------------------------------------------------------------------写的部分------------------------------------------------------------------------------

#define BIDIRECTIONAL 0 /* 如果你要完成额外的任务，请将其更改为1 /
/ 并编写一个名为B_output的例程 */

/* "msg" 是从层5(教师代码)传递到层4(学生代码)的数据单元。它包含要传递到层5的数据(字符)，
通过学生传输层协议实体传递。*/
struct msg {
    char data[20];
};

/* "pkt" 是从层4(学生代码)传递到层3(教师代码)的数据单元。请注意，预定义的数据包结构，
所有学生必须遵循。*/
struct pkt {
    int seqnum;
    int acknum;
    int checksum;  //定义了checksum变量，以检测数据包是否出错
    char payload[20];
};

/********* 学生编写下面七个例程 *********/

struct sendwindow      //存储发送窗口中的数据包信息
{
    struct pkt* pkt;
    int recvack;
};

struct senderinfo     //管理发送方 A 的状态信息
{
    int base;
    int winsize;
    int nextseqnum;
    struct sendwindow sw[10];
    float timeoutinterval;   
    //定义了timeoutinterval为超时重传间隔的大小，在 A_init 函数中，我们可以初始化这个变量的值。
    //在 A_timerinterrupt 函数中，我们可以使用 starttimer 函数来启动计时器，并将 timeout_interval 作为参数传递给它。
    //在需要动态调整超时重传间隔的时候，我们只需要修改 timeout_interval 的值即可。
};

struct senderinfo Asender;

/* A_output 将要发送的消息从层5传递给远程端 */

A_output(message) struct msg message;

{ if (Asender.nextseqnum < Asender.base + Asender.winsize)
  {
    struct pkt packet; packet.seqnum = Asender.nextseqnum;
    packet.acknum = 0; strncpy(packet.payload, message.data, 20); packet.checksum = packet.seqnum + packet.acknum;
    for (int i = 0; i < 20; i++)
    {
        packet.checksum += packet.payload[i];
    }
    // 随机模拟出现错误的情况
    if (diu1 == 1)
    {
        float cor1 = corruptprob * 100;
        if (rand() % 100 < cor1) { // 概率出现错误
            packet.checksum++; // 修改校验和，使其出错
            srand(rand() + 1);
        }
    }
    // 随机模拟出现丢失的情况
    if (loss1 == 1)
    {
        float cor2 = lossprob * 100;
        if (rand() % 100 < cor2) { // 概率出现错误
            return; // 直接返回，数据包丢失
            srand(rand() + 1);
        }
    }
    Asender.sw[Asender.nextseqnum % 10].pkt = (struct pkt*)malloc(sizeof(struct pkt));
    *(Asender.sw[Asender.nextseqnum % 10].pkt) = packet; Asender.sw[Asender.nextseqnum % 10].recvack = 0; tolayer3(0, packet);
    if (Asender.base == Asender.nextseqnum)
    {
        starttimer(0, Asender.timeoutinterval);
    }
    Asender.nextseqnum++;
    
  }
}

/* 接收到从层3传递到层4的数据包时调用 */

A_input(packet) struct pkt packet;
{ int checksum = packet.seqnum + packet.acknum;
for (int i = 0; i < 20; i++)
{
    checksum += packet.payload[i];
}
// 随机模拟出现错误的情况
if (diu2 == 1)
{
    float cor3 = corruptprob * 100;
    if (rand() % 100 < cor3) { // 概率出现错误
        packet.payload[0] = 'E'; // 将ACK包的第一个字节修改为'E'，模拟出现错误
        srand(rand() + 1);
    }
}
// 随机模拟出现丢失的情况
if (loss2 == 1)
{
    float cor4 = lossprob * 100;
    if (rand() % 100 < cor4) { // 概率出现错误
        return; // 直接返回，数据包丢失
        srand(rand() + 1);
    }
}
if (checksum == packet.checksum && packet.acknum >= Asender.base && packet.acknum < Asender.nextseqnum)
{
    Asender.sw[packet.acknum % 10].recvack = 1;
    while (Asender.sw[Asender.base % 10].recvack == 1)
    {
        Asender.base++; stoptimer(0);
        if (Asender.base == Asender.nextseqnum)
        {
            break;
        }
        starttimer(0, Asender.timeoutinterval);
    }
}
}

/* A实体的定时器中断函数，当A实体的定时器到期时调用 */

A_timerinterrupt()
{
    starttimer(0, Asender.timeoutinterval);
    for (int i = Asender.base; i < Asender.nextseqnum; i++)
    {
        if (Asender.sw[i % 10].recvack == 0)
        {
            tolayer3(0, *(Asender.sw[i % 10].pkt));
        }
    }
}

/* 在调用其他任何A实体函数之前，将调用以下例程 */ 
/* 可以用它来做任何初始化 */

A_init()
{
    Asender.base = 1;
    Asender.winsize = 1;
    Asender.nextseqnum = 1;
    Asender.timeoutinterval = 30.0;    //timeoutinterval的量初始化
}

/* 注意：由于是从A到B的单向传输，因此不需要B_output()函数 */

/* 当B接收到层3传递给层4的数据包时调用 */

B_input(packet) struct pkt packet;
{
    int checksum = packet.seqnum + packet.acknum;
    for (int i = 0; i < 20; i++)
    {
        checksum += packet.payload[i];
    }
    if (checksum == packet.checksum && packet.seqnum == 1)
    {
        tolayer5(1, packet.payload);
        struct pkt ackpacket; ackpacket.seqnum = 0;
        ackpacket.acknum = packet.seqnum;
        ackpacket.checksum = ackpacket.seqnum + ackpacket.acknum;
        tolayer3(1, ackpacket);
    }
}

B_output(message)
struct msg message;
{

}

/* B实体的定时器中断函数，当B实体的定时器到期时调用 */

B_timerinterrupt() { }

/* 在调用其他任何B实体函数之前，将调用以下例程 / / 可以用它来做任何初始化 */

B_init() { }


//-------------------------------------------------------------------------------------------写的部分--------------------------------------------------------------------------

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
    struct pkt* pktptr;     /* ptr to packet (if any) assoc w/ this event */
    struct event* prev;
    struct event* next;
};
struct event* evlist = NULL;   /* the event list */

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
float lambda;              /* arrival rate of messages from layer 5 */
int   ntolayer3;           /* number sent into layer 3 */
int   nlost;               /* number lost in media */
int ncorrupt;              /* number corrupted by media*/

int recv_pkt_num = 0;     /* 接收端成功收到的包计数 */
float first_recv_time = 0.000; /* 接收端最开始成功收到包的时间 */
float last_recv_time = 0.000; /* 接收端最后成功接收到包的时间 */
float throughput;            /* 接收端平均吞吐率 */

main()
{
    struct event* eventptr;
    struct msg  msg2give;
    struct pkt  pkt2give;

    int i, j;
    char c;

    init();
    A_init();
    B_init();

    while (1) {
        eventptr = evlist;            /* get next event to simulate */
        if (eventptr == NULL)
            goto terminate;
        evlist = evlist->next;        /* remove this event from event list */
        if (evlist != NULL)
            evlist->prev = NULL;

        // -------------------trace-----------------------
        if (TRACE >= 2) {
            printf("\nEVENT time: %f,", eventptr->evtime);
            printf("  type: %d", eventptr->evtype);
            if (eventptr->evtype == 0)
                printf(", timerinterrupt  ");
            else if (eventptr->evtype == 1)
                printf(", fromlayer5 ");
            else
                printf(", fromlayer3 ");
            printf(" entity: %d\n", eventptr->eventity);
        }
        // -------------------trace-----------------------

        time = eventptr->evtime;        /* update time to next event time */
        if (nsim == nsimmax)
            break;                        /* all done with simulation */
        if (eventptr->evtype == FROM_LAYER5) {
            generate_next_arrival();   /* set up future arrival */
            /* fill in msg to give with string of same letter */
            j = nsim % 26;
            for (i = 0; i < 20; i++)
                msg2give.data[i] = 97 + j;

            // ----------------------------打印包中内容-----------------------------
            if (TRACE > 2) {
                printf("          MAINLOOP: data given to student: ");
                for (i = 0; i < 20; i++)
                    printf("%c", msg2give.data[i]);
                printf("\n");
            }
            // ----------------------------打印包中内容-----------------------------

            nsim++;
            if (eventptr->eventity == A)
                A_output(msg2give);
            else
                B_output(msg2give);
        }
        else if (eventptr->evtype == FROM_LAYER3) {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i = 0; i < 20; i++)
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
            if (eventptr->eventity == A)      /* deliver packet by calling */
                A_input(pkt2give);            /* appropriate entity */
            else
                B_input(pkt2give);
            free(eventptr->pktptr);          /* free the memory for packet */
        }
        else if (eventptr->evtype == TIMER_INTERRUPT) {
            if (eventptr->eventity == A)
                A_timerinterrupt();
            else
                B_timerinterrupt();
        }
        else {
            printf("INTERNAL PANIC: unknown event type \n");
        }
        free(eventptr);
    }

terminate:
    throughput = recv_pkt_num / time;
    printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n B receives %d data and throughput is %f", time, nsim, recv_pkt_num, throughput);
}



init()                         /* initialize the simulator */
{
    int i;
    float sum, avg;
    float jimsrand();


    printf("-----  网络环境模拟 版本 1.1 -------- \n\n");
    printf("-----  完成者 谢睿琪&孔信轲 -------- \n\n");
    printf("输入需要传输的数据包数量: ");
    scanf("%d", &nsimmax);  /* 仿真结束时发送的数据包总数至少会比设置的nsimmax少1，如果少了很多说明有很多数据包在调用A_output()时被忽略了，或者是被留在了自定义的缓冲区中 */
    printf("输入模拟的丢包率[0.0为不丢包]:");
    scanf("%f", &lossprob);
    printf("输入模拟的错误率[0.0为无错误]:");
    scanf("%f", &corruptprob);
    printf("输入数据包发送间隔[ > 0.0]:");
    scanf("%f", &lambda);
    printf("TRACE模块值:");
    scanf("%d", &TRACE);
    printf("输入是否数据包error[有为1，无为0]:");
    scanf("%d", &diu1);
    printf("输入是否ACK包error[有为1，无为0]:");
    scanf("%d", &diu2);
    printf("输入是否数据包loss[有为1，无为0]:");
    scanf("%d", &loss1);
    printf("输入是否ACK包loss[有为1，无为0]:");
    scanf("%d", &loss2);


    srand(9999);              /* init random number generator */
    sum = 0.0;                /* test random number generator for students */
    for (i = 0; i < 1000; i++)
        sum = sum + jimsrand();    /* jimsrand() should be uniform in [0,1] */
    avg = sum / 1000.0;
    if (avg < 0.25 || avg > 0.75) {
        printf("It is likely that random number generation on your machine\n");
        printf("is different from what this emulator expects.  Please take\n");
        printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
        exit(-1);
    }

    ntolayer3 = 0;
    nlost = 0;
    ncorrupt = 0;

    time = 0.0;                    /* initialize time to 0.0 */
    generate_next_arrival();     /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand()
{
    double mmm = 32767;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
    float x;                   /* individual students may need to change mmm */
    x = rand() / mmm;            /* x should be uniform in [0,1] */
    return(x);
}

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

generate_next_arrival()
{
    double x, log(), ceil();
    struct event* evptr;
    char* malloc();
    float ttime;
    int tempint;

    if (TRACE > 2)
        printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

    //  x = lambda*jimsrand()*2;  /* x is uniform on [0,2*lambda] */
                               /* having mean of lambda        */
    x = lambda;
    evptr = (struct event*)malloc(sizeof(struct event));
    evptr->evtime = time + x;
    evptr->evtype = FROM_LAYER5;
    if (BIDIRECTIONAL && (jimsrand() > 0.5))
        evptr->eventity = B;
    else
        evptr->eventity = A;
    insertevent(evptr);
}


insertevent(p)
struct event* p;
{
    struct event* q, * qold;

    if (TRACE > 2) {
        printf("            INSERTEVENT: time is %lf\n", time);
        printf("            INSERTEVENT: future time will be %lf\n", p->evtime);
    }
    q = evlist;     /* q points to header of list in which p struct inserted */
    if (q == NULL) {   /* list is empty */
        evlist = p;
        p->next = NULL;
        p->prev = NULL;
    }
    else {
        for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next)
            qold = q;
        if (q == NULL) {   /* end of list */
            qold->next = p;
            p->prev = qold;
            p->next = NULL;
        }
        else if (q == evlist) { /* front of list */
            p->next = evlist;
            p->prev = NULL;
            p->next->prev = p;
            evlist = p;
        }
        else {     /* middle of list */
            p->next = q;
            p->prev = q->prev;
            q->prev->next = p;
            q->prev = p;
        }
    }
}

printevlist()
{
    printf("当 evtype 为 0 时，表示定时器中断事件，即发送方或接收方的定时器到期。 \n\n");
    printf("当 evtype 为 1 时，表示从层5传输数据事件，即发送方从层5接收到了要发送的数据。 \n\n");
    printf("当 evtype 为 2 时，表示从层3传输数据事件，即网络层从发送方或接收方接收到了数据包。 \n\n");
    struct event* q;
    int i;
    printf("--------------\nEvent List Follows:\n");
    for (q = evlist; q != NULL; q = q->next) {
        printf("Event time: %f, type: %d entity: %d\n", q->evtime, q->evtype, q->eventity);
    }
    printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
stoptimer(AorB)     //调用函数时间结束计时器
int AorB;  /* A or B is trying to stop timer */
{
    struct event* q, * qold;

    if (TRACE > 2)
        printf("          STOP TIMER: stopping timer at %f\n", time);
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB)) {
            /* remove this event */
            if (q->next == NULL && q->prev == NULL)
                evlist = NULL;         /* remove first and only event on list */
            else if (q->next == NULL) /* end of list - there is one in front */
                q->prev->next = NULL;
            else if (q == evlist) { /* front of list - there must be event after */
                q->next->prev = NULL;
                evlist = q->next;
            }
            else {     /* middle of list */
                q->next->prev = q->prev;
                q->prev->next = q->next;
            }
            free(q);
            return;
        }
    printf("Warning: unable to cancel your timer. It wasn't running.\n");
}


starttimer(AorB, increment)    //调用此函数启动的计时器
int AorB;  /* A or B is trying to stop timer */
float increment;
{

    struct event* q;
    struct event* evptr;
    char* malloc();

    if (TRACE > 2)
        printf("          START TIMER: starting timer at %f\n", time);
    /* be nice: check to see if timer is already started, if so, then  warn */
   /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB)) {
            printf("Warning: attempt to start a timer that is already started\n");
            return;
        }

    /* create future event for when timer goes off */
    evptr = (struct event*)malloc(sizeof(struct event));
    evptr->evtime = time + increment;
    evptr->evtype = TIMER_INTERRUPT;
    evptr->eventity = AorB;
    insertevent(evptr);
}


/************************** TOLAYER3 ***************/
tolayer3(AorB, packet)     //调用此函数，传入需要发送的 packet，将数据包传递至网络层。这之后的传输工作将由框架自动完成。
int AorB;  /* A or B is trying to stop timer */
struct pkt packet;
{
    struct pkt* mypktptr;
    struct event* evptr, * q;
    char* malloc();
    float lastime, x, jimsrand();
    int i;


    ntolayer3++;

    /* simulate losses: */
    if (jimsrand() < lossprob) {
        nlost++;
        if (TRACE > 0)
            printf("          TOLAYER3: packet being lost\n");
        return;
    }

    /* make a copy of the packet student just gave me since he/she may decide */
    /* to do something with the packet after we return back to him/her */
    mypktptr = (struct pkt*)malloc(sizeof(struct pkt));
    mypktptr->seqnum = packet.seqnum;
    mypktptr->acknum = packet.acknum;
    mypktptr->checksum = packet.checksum;
    for (i = 0; i < 20; i++)
        mypktptr->payload[i] = packet.payload[i];
    if (TRACE > 2) {
        printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
            mypktptr->acknum, mypktptr->checksum);
        for (i = 0; i < 20; i++)
            printf("%c", mypktptr->payload[i]);
        printf("\n");
    }

    /* create future event for arrival of packet at the other side */
    evptr = (struct event*)malloc(sizeof(struct event));
    evptr->evtype = FROM_LAYER3;   /* packet will pop out from layer3 */
    evptr->eventity = (AorB + 1) % 2; /* event occurs at other entity */
    evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */
    /* finally, compute the arrival time of packet at the other end.
       medium can not reorder, so make sure packet arrives between 1 and 10
       time units after the latest arrival time of packets
       currently in the medium on their way to the destination */
    lastime = time;
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == FROM_LAYER3 && q->eventity == evptr->eventity))
            lastime = q->evtime;
    evptr->evtime = lastime + 1 + 9 * jimsrand();
    // evptr->evtime =  lastime + 5;



     /* simulate corruption: */
    if (jimsrand() < corruptprob) {
        ncorrupt++;
        if ((x = jimsrand()) < .75)
            mypktptr->payload[0] = 'Z';   /* corrupt payload */
        else if (x < .875)
            mypktptr->seqnum = 999999;
        else
            mypktptr->acknum = 999999;
        if (TRACE > 0)
            printf("          TOLAYER3: packet being corrupted\n");
    }

    if (TRACE > 2)
        printf("          TOLAYER3: scheduling arrival on other side\n");
    insertevent(evptr);
}

tolayer5(AorB, datasent)    //通过调用此函数，传入 message，将消息传递至应用层。这之后仿真框架将检查数据是否完好、是否按序到达。
int AorB;
char datasent[20];
{
    int i;
    if (TRACE > 2) {
        printf("          TOLAYER5: data received: ");
        for (i = 0; i < 20; i++)
            printf("%c", datasent[i]);
        printf("\n");
    }
    if (AorB == B) {
        recv_pkt_num++;
        if (recv_pkt_num == 1) {
            first_recv_time = time;
        }
        last_recv_time = time;
    }
}
