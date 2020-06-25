#include<unistd.h>
#include<string.h>
#include <stdio.h>
#include<signal.h>
#include<errno.h>
#include<time.h>
#include<sys/time.h>
#include<arpa/inet.h>

#include "rsocket.h"
int id_count=0;
int sock=-1;
//int transmission_count=0;

//Send packet structure
struct sendPacket{
    struct sockaddr_in addr;
    int type;
    int msg_id;
    char *msg;
    int size;
};


//send Buffer maintatined as a circular queue of size 100
struct sendBuf{
    int start, end, size;
    struct sendPacket *buffer[100];
} sb;

//Function to decide when to drop packet
int dropMessage(float p)
{
    srand((unsigned int)time(0));
    int x = rand();
    if(((float)(x%100))<(p*100)) return 1;
    else return 0;
}

//Function to insert into send buffer
void insertSB(struct sendPacket *sp)
{
    if(sb.size==100) return;
    sb.buffer[sb.end] = sp;
    sb.end = (sb.end+1)%100;
    sb.size++;
}

//Function to remove from send buffer
struct sendPacket *removeSB()
{
    if(sb.size==0) return NULL;
    struct sendPacket *sp = sb.buffer[sb.start];
    sb.start = (sb.start+1)%100;
    sb.size--;
    return sp;
}

//Receive packet structure
struct recvPacket
{
    struct sockaddr_in addr;
    char* msg;
    int size;
};

//reciever Buffer maintatined as a circular queue of size 100
struct recvBuf{
    int start, end, size;
    struct recvPacket *buffer[100];
} rb;

//Function to insert into receiver buffer
void insertRB(struct recvPacket *rp)
{
    if(rb.size==100) return;
    rb.buffer[rb.end] = rp;
    rb.end = (rb.end+1)%100;
    rb.size++;
}

//Function to remove packet from receiver buffer
struct recvPacket *removeRB()
{
    if(rb.size==0) return NULL;
    struct recvPacket *rp = rb.buffer[rb.start];
    rb.start = (rb.start+1)%100;
    rb.size--;
    return rp;
}


//Element of UnackTable
struct UnAckTableElem{
    struct sendPacket sp;
    time_t t;
    struct UnAckTableElem *next;
};

//UnackTable maintained as a linked list
struct UnAckTableElem *UnackHead;
//Max size maintained at 100
int Unacksize;



//Received message id maintaines as a circular queue of message id's
struct rcvmsgid{
    int msg_id[100];
    int start,end,size;
} rcv_id;

//Function to encode packet to send through UDP
int encode(struct sendPacket *sp, void * msg)
{
    int temp=0;
    memcpy(msg+temp, &(sp->addr), sizeof(sp->addr));
    temp += sizeof(sp->addr);

    memcpy(msg+temp, &(sp->type), sizeof(sp->type));
    temp += sizeof(sp->type);

    memcpy(msg+temp, &(sp->msg_id), sizeof(sp->msg_id));
    temp += sizeof(sp->msg_id);

    memcpy(msg+temp, &(sp->size), sizeof(sp->size));
    temp += sizeof(sp->size);

    if((sp->size)>0)
    {
        memcpy(msg+temp, sp->msg, sp->size);
        temp += sp->size;
    }
    return temp;
}

//Function to decode UDP message back to the sendPacket structure  
int decode(struct sendPacket *sp, void *msg, size_t size)
{
    int temp_size = 0;
    memcpy(&(sp->addr), msg+temp_size, sizeof(struct sockaddr_in));
    temp_size += sizeof(struct sockaddr_in);

    memcpy(&(sp->type), msg+temp_size, sizeof(int));
    temp_size += sizeof(int);

    memcpy(&(sp->msg_id), msg+temp_size, sizeof(int));
    temp_size += sizeof(int);

    memcpy(&(sp->size), msg+temp_size, sizeof(int));
    temp_size += sizeof(int);

    if((sp->size)>0)
    {
        sp->msg = (char*)malloc(sp->size);
        memcpy(sp->msg, msg+temp_size, sp->size);
        temp_size += sp->size;
    }
    if(size != temp_size) return -1;
    
    return 0;
}

//Function to handle receiving Application messages
void HandleAppMsgRecv(struct sendPacket * sp, struct sockaddr * rcv_adder, socklen_t rcv_addrlen)
{
    //if receiver buffer is full then return
    if(rb.size==100) return;

    int dup_msg=0;
    int i;
    //Cheching if the decoded packet is duplicate or not 
    for(i=rcv_id.start;i<rcv_id.end;i=(i+1)%100)
    {
        if(rcv_id.msg_id[i]==sp->msg_id)
        {
            dup_msg=1;
        }
    }
    if(rcv_id.size==100) rcv_id.start=(rcv_id.start+1)%100;
    rcv_id.msg_id[rcv_id.end]=sp->msg_id;
    rcv_id.end=(rcv_id.end+1)%100;

    //creating acknowledgement package and encoding it
    struct sendPacket *ack = (struct sendPacket*)malloc(sizeof(struct sendPacket));
    ack->msg_id = sp->msg_id;
    ack->msg=NULL;
    ack->type=ACK;
    ack->addr=*((struct sockaddr_in *)rcv_adder);
    ack->size=0;  
    void *temp = malloc(1000);
    int templen;
    templen = encode(sp,temp);

    //sending acknowledgement
    int k=sendto(sock,temp,templen,0,rcv_adder,rcv_addrlen);

    //Inseting packet to receiver buffer if not duplicate
    if(dup_msg==0)
    {
        struct recvPacket *rp = (struct recvPacket*)malloc(sizeof(struct recvPacket));
        rp->addr = *((struct sockaddr_in *)rcv_adder);
        rp->msg = (char*)malloc(sp->size);
        memcpy(rp->msg,sp->msg,sp->size);
        rp->size = sp->size;
        insertRB(rp);
    }
}

//Function to handle Acknowledgements
void HandleACKMsgRecv(struct sendPacket *sp)
{
    struct UnAckTableElem * p,*q;
    p = UnackHead;
    q = UnackHead;

    //If packet with corresponding msg_id is present in Unacknowldged table then remove it
    //If not it is ACK to duplicate, thus ignore
    while(p!=NULL)
    {
        if(p->sp.msg_id==sp->msg_id)
        {
            if(p==UnackHead)
            {
                UnackHead = p->next;
                free(p);
                Unacksize--;
                return;
            }
            else
            {
                q->next = p->next;
                free(p);
                Unacksize--;
                return;
            }
        }
        q=p;
        p=p->next;
    }
}

//Function to handle receive
void HandleReceive()
{
    struct sockaddr rcv_addr;
    socklen_t rcv_addrlen;
    void *temp = malloc(1000);
    int templen=1000;
    int n;

    //read message
    n = recvfrom(sock, temp, templen,MSG_DONTWAIT,&rcv_addr,&rcv_addrlen);
    if(n<=0) return;

    //dropping packet with P probability after collecting from recvfrom, to remove from underlying UDP layer's buffer
    if(dropMessage(P)) return;
    
    struct sendPacket *sp = (struct sendPacket*)malloc(sizeof(struct sendPacket));
    if(decode(sp,temp,n)<0)
    {
        printf("Error in Received packet encoding\n");
        free(sp);
        return;
    }

    //Handling message accordingly if APP or ACK
    if(sp->type==APP) 
    {
        HandleAppMsgRecv(sp,&rcv_addr,rcv_addrlen);
    }
    else 
    {
        HandleACKMsgRecv(sp);
    }
}

//Function to handle retransmissions
void HandleRetransmit()
{
    time_t t = time(NULL);
    struct UnAckTableElem *p = UnackHead;
    while(p!=NULL)
    {
        //If T has exceeded then resend the packet
        if((t-p->t)>T)
        {
            struct sendPacket *sp = (struct sendPacket *)malloc(sizeof(struct sendPacket));  
            sp->addr = p->sp.addr;
            sp->msg = (char*)malloc(p->sp.size);
            memcpy(sp->msg, p->sp.msg,p->sp.size);
            sp->msg_id = p->sp.msg_id;
            sp->size = p->sp.size;
            sp->type = p->sp.type;

            void * temp = malloc(1000);
            int templen = encode(sp,temp);

            int k = sendto(sock,temp, templen,MSG_DONTWAIT,(struct sockaddr *)(&(sp->addr)),sizeof(sp->addr));
            if(k>0) 
            {
                p->t = t;
            }
        }
        p=p->next;
    }
}

//Function to handle first time transmissions
void HandleTransmit()
{
    //Return if sendbuffer is empty or UnackTable is full
    if(Unacksize==100) return;
    if(sb.size==0) return;
    int i;
    time_t t = time(NULL);

    //iterate through sendbuffer
    for(i=sb.start;i<sb.end;i=(i+1)%100)
    {
        //create copy of packet present in sendbuffer and encode it
        struct sendPacket *sp = (struct sendPacket *)malloc(sizeof(struct sendPacket));  
        sp->addr = sb.buffer[i]->addr;
        sp->msg = (char*)malloc(sb.buffer[i]->size);
        memcpy(sp->msg, sb.buffer[i]->msg,sb.buffer[i]->size);
        sp->msg_id = sb.buffer[i]->msg_id;
        sp->size = sb.buffer[i]->size;
        sp->type = sb.buffer[i]->type;

        void * temp = malloc(1000);
        int templen = encode(sp,temp);
        
        //send the packet
        int k = sendto(sock,temp,templen,MSG_DONTWAIT,(struct sockaddr *)(&(sp->addr)),sizeof(sp->addr));
        //If successfully sent then add to UnAck table
        if(k>0)
        {
            struct UnAckTableElem *p,*q;
            q = (struct UnAckTableElem*)malloc(sizeof(struct UnAckTableElem));
            q->next=NULL;
            q->t = t;

            q->sp.addr = sb.buffer[i]->addr;
            q->sp.msg = (char*)malloc(sb.buffer[i]->size);
            memcpy(q->sp.msg, sb.buffer[i]->msg,sb.buffer[i]->size);
            q->sp.msg_id = sb.buffer[i]->msg_id;
            q->sp.size = sb.buffer[i]->size;
            q->sp.type = sb.buffer[i]->type;

            if(UnackHead==NULL)
            {
                UnackHead = q;
                Unacksize++;
                continue;
            }
            p = UnackHead;
            while(p->next!=NULL)
            {
                p=p->next;
            }
            p->next = q;
            Unacksize++;
        }
        
    }
}

//signal handling function
void signalHandler(int signal)
{
    HandleReceive();
    HandleRetransmit();
    HandleTransmit();
}

//Function to create socket
int r_socket(int domain, int type, int protocol)
{
    if(type!=SOCK_MRP)
    {
        return -1;
    }
    sock = socket(domain,SOCK_DGRAM,protocol);
    
    if(sock<0) return sock;

    //Initialising signal and itimer values
    if(signal(SIGALRM, signalHandler) < 0) return -1;

    struct itimerval *timer = (struct itimerval *)malloc(sizeof(struct itimerval));
    timer->it_value.tv_sec = INTERVAL;
    timer->it_value.tv_usec = 0;
    timer->it_interval.tv_sec = INTERVAL;
    timer->it_interval.tv_usec = 0;
    
    //setting itimer
    if(setitimer(ITIMER_REAL, timer, NULL) < 0) return -1;

    //initialising sendbuffer
    sb.start=0;
    sb.end = 0;
    sb.size=0;

    //initialising recvbuffer
    rb.start=0;
    rb.end=0;
    rb.size=0;

    //initialising UnackTable
    UnackHead = NULL;
    Unacksize=0;

    //Initialising receiver message id table
    rcv_id.start=0;
    rcv_id.end=0;
    rcv_id.size=0;
    //return socketfd
    return sock;
}

//Function to bind socket
int r_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    //binding the socket
    return bind(sockfd, addr, addrlen);
}

//function to receive from recevier buffer and give to message to application layer (blocking in nature)
ssize_t r_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t *addrlen)
{
    struct recvPacket *rp;
    while(1)
    {
        if(rb.size>0)
        {
            //remove packet from receiver buffer
            rp = removeRB();
            memcpy(buf,(void*)rp->msg,rp->size);
            break;
        }
    }
    return rp->size;
}

//function to take message from application layer and add to send buffer (blocking in nature)
ssize_t r_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{ 
    struct sendPacket * sp;
    sp = (struct sendPacket *)malloc(sizeof(struct sendPacket));

    sp->addr = *((struct sockaddr_in *)dest_addr);
    sp->type = APP;
    sp->msg_id = id_count;
    sp->size = len;
    
    if(len>0)
    {
        sp->msg = (char*)malloc(len);
        memcpy((void *)sp->msg,buf, len);
    }
    id_count+=1;
    while(1)
    {
        if(sb.size<100)
        {
            //insert to sendbuffer
            insertSB(sp);
            break;
        }
    }
    return sp->size;
}

int r_close(int fd)
{   
    int i;
    for(i=sb.start;i<sb.size;i=(i+1)%100)
    {
        free(sb.buffer[i]);
    }
    for(i=rb.start;i<rb.end;i=(i+1)%100)
    {
        free(rb.buffer[i]);
    }

    struct UnAckTableElem *p = UnackHead;
    struct UnAckTableElem *q;
    while(p!=NULL)
    {
        q=p->next;
        free(p);
        p=q;
    }
    return close(fd);
}