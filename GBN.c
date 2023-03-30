#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#pragma warning(disable:4996)

int diu1;
int expectedseqnum = 0;
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

// 定义一个常量，表示是否启用双向传输，默认为不启用
#define BIDIRECTIONAL 0

// 定义了两个结构体：msg 和 pkt，分别表示消息和数据包
struct msg {
    char data[20];
};

struct pkt {
    int seqnum; // 序列号
    int acknum; // 确认号
    int checksum; // 校验和
    char payload[20]; // 负载
};

// 定义了两个结构体：sendwindow 和 senderinfo，分别表示发送窗口和发送方信息
struct sendwindow {
    struct pkt* pkt; // 数据包指针
    int recvack; // 是否收到确认标志
};

struct senderinfo {
    int base; // 发送窗口基序号
    int winsize; // 发送窗口大小
    int nextseqnum; // 下一个序号
    struct sendwindow sw[10]; // 发送窗口数组
    float timeoutinterval; // 超时时间间隔
};

// 定义了一个全局变量 Asender，表示发送方信息
struct senderinfo Asender;

// A 侧发送消息的函数，当 A 侧要发送消息时会调用该函数
A_output(message) struct msg message;
{
    // 判断发送窗口是否已满
    if (Asender.nextseqnum < Asender.base + Asender.winsize) {
        // 创建一个数据包
        struct pkt packet;
        packet.seqnum = Asender.nextseqnum; // 序列号为下一个序号
        packet.acknum = 0; // 确认号为 0
        strncpy(packet.payload, message.data, 20); // 拷贝消息的数据到负载中
        packet.checksum = packet.seqnum + packet.acknum; // 计算校验和
        for (int i = 0; i < 20; i++) {
            packet.checksum += packet.payload[i];
        }
        // 处理丢包和损坏
        if (diu1 == 1) {
            float cor1 = corruptprob * 100;
            if (rand() % 100 < cor1) {
                return;
                srand(rand() + 1);
            }
        }
        if (loss1 == 1) {
            float cor2 = lossprob * 100;
            if (rand() % 100 < cor2) {
                return;
                srand(rand() + 1);
            }
        }
        // 将数据包存储到发送窗口中
        Asender.sw[Asender.nextseqnum % 10].pkt = (struct pkt*)malloc(sizeof(struct pkt));
        *(Asender.sw[Asender.nextseqnum % 10].pkt) = packet;
        Asender.sw[Asender.nextseqnum % 10].recvack = 0;
        // 发送数据包到 B 侧
        tolayer3(0, packet);
        // 如果发送窗口为空，启动定时器
        if (Asender.base == Asender.nextseqnum) {
            starttimer(0, Asender.timeoutinterval);
        }
        Asender.nextseqnum++; // 下一个序号加一
    }
}

// A 侧接收数据包的函数，当 A 侧接收到数据包时会调用该函数
A_input(packet) struct pkt packet;
{
    // 计算数据包的校验和
    int checksum = packet.seqnum + packet.acknum;
    for (int i = 0; i < 20; i++) {
        checksum += packet.payload[i];
    }
    // 处理丢包和损坏
    if (diu2 == 1)
    {
        float cor3 = corruptprob * 100;
        if (rand() % 100 < cor3) {
            return;
            srand(rand() + 1);
        }
    }
    if (loss2 == 1)
    {
        float cor4 = lossprob * 100;
        if (rand() % 100 < cor4) {
            return;
            srand(rand() + 1);
        }
    }
    // 判断数据包是否损坏、是否为期望的确认号、是否在发送窗口内
    if (checksum == packet.checksum && packet.acknum >= Asender.base && packet.acknum < Asender.nextseqnum) {
        Asender.sw[packet.acknum % 10].recvack = 1; // 标记为已收到确认
        // 如果收到的是基序号的数据包，一直循环直到发送窗口内的数据包都已经收到确认
        if (packet.acknum == Asender.base) {
            while (Asender.sw[Asender.base % 10].recvack == 1) {
                Asender.base++;
                // 如果发送窗口内的数据包都已经收到确认，则停止定时器
                if (Asender.base == Asender.nextseqnum) {
                    stoptimer(0);
                    return;
                }
                starttimer(0, Asender.timeoutinterval); // 重新启动定时器
            }
        }
    }
}

// A 侧的定时器中断函数，当 A 侧的定时器到达超时时间时会调用该函数
A_timerinterrupt() {
    int i = Asender.base;
    while (i < Asender.nextseqnum) {
        if (Asender.sw[i % 10].recvack == 0) {
            tolayer3(0, *(Asender.sw[i % 10].pkt));
        }
        i++;
    }
    if (Asender.nextseqnum > Asender.base) {
        starttimer(0, Asender.timeoutinterval);
    }
}




// A 侧的初始化函数，当 A 侧被启动时会调用该函数
A_init(float ti) {
    Asender.base = 1; // 发送窗口基序号为 1
    Asender.winsize = 10; // 发送窗口大小为 10
    Asender.nextseqnum = 1; // 下一个序号为 1
    Asender.timeoutinterval = ti; // 超时时间间隔为参数 ti
}

// B 侧接收数据包的函数，当 B 侧接收到数据包时会调用该函数
B_input(packet) struct pkt packet;
{
    // 计算数据包的校验和
    int checksum = packet.seqnum + packet.acknum;
    for (int i = 0; i < 20; i++) {
        checksum += packet.payload[i];
    }
    // 判断数据包是否损坏、是否为期望的序列号、是否在发送窗口内
    if (checksum == packet.checksum && packet.seqnum >= Asender.base && packet.seqnum < Asender.base + Asender.winsize) {
        // 创建一个确认数据包并发送给 A 侧
        struct pkt ackpacket;
        ackpacket.seqnum = 0;
        ackpacket.acknum = packet.seqnum;
        ackpacket.checksum = ackpacket.seqnum + ackpacket.acknum;
        tolayer3(1, ackpacket);
        // 将数据包的数据传给上层应用
        if (packet.seqnum == Asender.base) {
            tolayer5(1, packet.payload);
            Asender.base++; // 发送窗口基序号加一
        }
    }
}

// B 侧发送消息的函数，当 B 侧要发送消息时会调用该函数
B_output(message) struct msg message;
{

}

// B 侧的定时器中断函数
B_timerinterrupt() {}

// B 侧的初始化函数
B_init() {}


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
float myTime = 0.000;        /* 记录最后时刻 */
float timeoutinterval;       /* 超时重传时间 */

main()
{
    struct event* eventptr;
    struct msg  msg2give;
    struct pkt  pkt2give;

    int i, j;
    char c;

    init();
    A_init(timeoutinterval);
    B_init();

    while (1) 
    {
        if (TRACE >= 2)
        printevlist();
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
        if (eventptr->evtype == FROM_LAYER5) {
            generate_next_arrival();   /* set up future arrival */
            /* fill in msg to give with string of same letter */
            j = nsim % 26;
            for (i = 0; i < 20; i++)
                msg2give.data[i] = 97 + j;


            if (nsim != nsimmax)
            {

                // ----------------------------打印包中内容-----------------------------
                if (TRACE > 2) {
                    printf("          MAINLOOP: data given to student: ");
                    for (i = 0; i < 20; i++)
                        printf("%c", msg2give.data[i]);
                    printf("\n");
                }
                // ----------------------------打印包中内容-----------------------------

            }

            if (recv_pkt_num == nsimmax)
                printf("注意：程序最后要结束的时间 %lf\n", myTime);

            if (nsim == nsimmax)
                break;                        /* all done with simulation */

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
    throughput = recv_pkt_num / myTime;
    printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n B receives %d data and throughput is %f", myTime, nsim, recv_pkt_num, throughput);
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
    printf("输入数据包超时重传时间[ > 0.0]:");
    scanf("%f", &timeoutinterval);
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

    printf("当 type 为 0 时，表示定时器中断事件，即发送方或接收方的定时器到期。 \n\n");
    printf("当 type 为 1 时，表示从层5传输数据事件，即发送方从层5接收到了要发送的数据。 \n\n");
    printf("当 type 为 2 时，表示从层3传输数据事件，即网络层从发送方或接收方接收到了数据包。 \n\n");


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
    if (nsim != nsimmax) {


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
    else
    {
        if (TRACE > 2)
            printf("           这是为了等待最后一个数据包接收而专门延长的一步\n");
            printf("\n");
            printf("\n");
            printf("\n");
    }
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
        myTime = evptr->evtime;
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
