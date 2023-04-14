/*
 *SR协议
*/
#include <stdio.h>
#include <string.h>
#include <stdbool.h>//使用布尔类型

#include "protocol.h"//库函数中包括的函数原型以及相关的宏定义
#include "datalink.h"//帧种类宏定义

#define DATA_TIMER  2000//计时器超时时间值
#define ACK_TIMER 600//ack计时器超时时间值
#define MAX_SEQ 127//帧序列号为0到127之间
#define NR_BUFS ((MAX_SEQ+1)/2)//缓冲区大小64帧
#define inc(k) if(k < MAX_SEQ) k = k+1; else k = 0//使k在0到127之间循环递增
struct FRAME { 
    unsigned char kind;//帧种类，一字节
    unsigned char ack;//ACK,一字节，表示发送方上一接收帧的序列号
    unsigned char seq;//SEQ，一字节，表示帧的序列号
    unsigned char data[PKT_LEN]; //数据，240到256字节
    unsigned int  padding;//CRC校验位，四字节
};
typedef struct {
    unsigned char data[PKT_LEN];
}buf;

static unsigned char ack_expected = 0;//发送窗口下界
static unsigned char frame_nr = 0;//发送窗口上界加一
static unsigned char frame_expected = 0;//接收方窗口下界
static unsigned char too_far = NR_BUFS;//接收方窗口上界加一
static unsigned char i;//缓冲区索引
static buf out_buf[NR_BUFS];//发送缓冲区
static buf in_buf[NR_BUFS];//接收缓冲区
static bool arrived[NR_BUFS];//接收缓冲区状态
static unsigned char nbuffered = 0;//发送缓冲区被使用个数，发送窗口大小

static int phl_ready = 0;//物理层发送队列的长度低于50字节时为1，其余时候为0
static bool no_nak = true;//nak帧没有被发送过
static unsigned char oldest_frame = MAX_SEQ + 1;//初始值只用于模拟器


//数据链路层向物理层发送一帧
static void put_frame(unsigned char *frame, int len)
{
    *(unsigned int *)(frame + len) = crc32(frame, len);//将校验位放在帧后
    send_frame(frame, len + 4);//将带校验位的帧发送给物理层
    phl_ready = 0;
}

//数据链路层向物理层发送数据帧
static void send_data_frame(void)
{
    struct FRAME s;

    s.kind = FRAME_DATA;//该帧是数据帧
    s.seq = frame_nr;//帧序号
    s.ack = (frame_expected+MAX_SEQ)%(MAX_SEQ+1);
    memcpy(s.data, &out_buf[frame_nr % NR_BUFS], PKT_LEN);

    dbg_frame("Send DATA SEQ:%d ACK:%d ID:%d\n", s.seq, s.ack, *(short *)s.data);

    put_frame((unsigned char *)&s, 3 + PKT_LEN);
    start_timer(frame_nr, DATA_TIMER);
    dbg_event("Start %d timer\n",frame_nr);
    stop_ack_timer();
}

//重传数据帧
static void resend_data_frame(unsigned char ack)
{
    struct FRAME s;

    s.kind = FRAME_DATA;//该帧是数据帧
    s.seq = ack;//帧序号
    s.ack = (frame_expected+MAX_SEQ)%(MAX_SEQ+1);
    memcpy(s.data, &out_buf[ack%NR_BUFS], PKT_LEN);

    dbg_frame("ReSend DATA SEQ:%d ACK:%d ID:%d\n", s.seq, s.ack, *(short *)s.data);

    put_frame((unsigned char *)&s, 3 + PKT_LEN);
    start_timer(ack, DATA_TIMER);
    dbg_event("ReStart %d timer\n",ack);
    stop_ack_timer();
}

//数据链路层向物理层发送ACK帧
static void send_ack_frame(void)
{
    struct FRAME s;

    s.kind = FRAME_ACK;
    s.ack = (frame_expected+MAX_SEQ)%(MAX_SEQ+1);//ack为正确收到的帧的序号

    dbg_frame("Send ACK  ACK:%d\n", s.ack);

    put_frame((unsigned char *)&s, 2);
    stop_ack_timer();
}

static void send_nak_frame(void)
{
    struct FRAME s;

    s.kind = FRAME_NAK;
    s.ack = (frame_expected+MAX_SEQ)%(MAX_SEQ+1);

    no_nak = false;

    dbg_frame("Send NAK  ACK:%d\n",s.ack);

    put_frame((unsigned char *)&s, 2);
    stop_ack_timer();
}
//如果a<=b<c循环返回真，否则返回假
static bool between(unsigned char a,unsigned char b,unsigned char c)
{
    if(((a<=b)&&(b<c))||((c<a)&&(a<=b))||((b<c)&&(c<a)))
        return true;
    else
        return false;
}

int main(int argc, char **argv)//两个参数提供从命令行参数中获取配置系统参数的手段
{
    int event, arg;//声明事件及事件的相关信息
    struct FRAME f;//声明帧f
    int len = 0;
    for(i = 0;i < NR_BUFS;i++)
        arrived[i] = false;

    protocol_init(argc, argv);//对运行环境初始化
    lprintf("Designed by Li Ruobin, build: " __DATE__"  "__TIME__"\n");//printf的改进函数，在输出前加时间坐标

    disable_network_layer();//缓冲区满时通过该函数通知网络层

    while (true) {
        event = wait_for_event(&arg);//返回事件之一，arg用于获取发生事件的相关信息

        switch (event) {
        case NETWORK_LAYER_READY:
        //接收，保存，发送一个新帧
            get_packet((char*)&out_buf[frame_nr%NR_BUFS]);//获取新包
            nbuffered++;//发送窗口加一
            send_data_frame();//发送帧
            inc(frame_nr);//发送窗口上界加一
            dbg_event("Change Sender's window,lower edge:%d,upper edge:%d\n",ack_expected,(frame_nr+MAX_SEQ)%(MAX_SEQ+1));
            break;

        case PHYSICAL_LAYER_READY:
        //物理层发送队列的长度低于50字节
            phl_ready = 1;
            break;

        case FRAME_RECEIVED:
        //物理层收到了一整帧
            len = recv_frame((unsigned char *)&f, sizeof f);//len是收到帧的实际长度
            //校验位出错
            if (len < 5 || crc32((unsigned char *)&f, len) != 0) {
                dbg_event("**** Receiver Error, Bad CRC Checksum\n");
                if(no_nak)
                    send_nak_frame();
                break;
            }
            if (f.kind == FRAME_ACK) 
                dbg_frame("Recv ACK  ACK:%d\n", f.ack);
            if (f.kind == FRAME_DATA) {
                dbg_frame("Recv DATA SEQ:%d ACK:%d ID:%d\n", f.seq, f.ack, *(short *)f.data);
                if((f.seq != frame_expected)&&no_nak)
                    send_nak_frame();
                else
                    start_ack_timer(ACK_TIMER);
                //该数据帧在接收窗口内且对应接收缓冲区为空
                if (between(frame_expected,f.seq,too_far)&&(arrived[f.seq%NR_BUFS] == 0)) {
                    //帧可以以任意顺序接收
                    arrived[f.seq%NR_BUFS] = true;//将缓冲区标记为满
                    //dbg_event("帧入缓冲区 %d %d %d\n",f.seq%NR_BUFS,frame_expected%NR_BUFS,arrived[f.seq%NR_BUFS]);
                    dbg_event("in_buf[%d] full of date ID:%d\n",f.seq%NR_BUFS,*(short *)f.data);
                    memcpy(&in_buf[f.seq%NR_BUFS], f.data, PKT_LEN);//将数据存入缓冲区
                    //接收缓冲区
                    while(arrived[frame_expected%NR_BUFS]){
                        //取接收窗口下界的包上传网络层
                        dbg_event("Put Packet in_buf[%d]\n",frame_expected%NR_BUFS);
                        put_packet((char*)&in_buf[frame_expected%NR_BUFS],len-7);
                        no_nak = true;
                        arrived[frame_expected%NR_BUFS] = false;
                        //移动接收窗口
                        inc(frame_expected);
                        inc(too_far);
                        dbg_event("Change Receiver's window,lower edge:%d,upper edge:%d\n",frame_expected,(too_far+MAX_SEQ)%(MAX_SEQ+1));
                        start_ack_timer(ACK_TIMER);
                    }
                }
            }
            if((f.kind == FRAME_NAK)&&between(ack_expected,(f.ack+1)%(MAX_SEQ+1),frame_nr)){
                dbg_frame("Recv NAK ACK:%d\n", f.ack);
                resend_data_frame((f.ack+1)%(MAX_SEQ+1));//重传ack下一帧
            }
            while (between(ack_expected,f.ack,frame_nr)){
                nbuffered--;
                stop_timer(ack_expected);
                dbg_event("Stop %d timer\n",ack_expected);
                inc(ack_expected);
                dbg_event("Change Sender's window,lower edge:%d,upper edge:%d\n",ack_expected,(frame_nr+MAX_SEQ)%(MAX_SEQ+1));
            }
            break;

        case DATA_TIMEOUT:
        //计时器超时
        //计时器超时的原因是什么？
            dbg_event("Timer %d time out\n",arg);
            oldest_frame = arg;
            resend_data_frame(oldest_frame);
            break;

        case ACK_TIMEOUT:
        //ack计时器超时
            send_ack_frame();//发送ACK帧
        }

        if (nbuffered < NR_BUFS && phl_ready)
            enable_network_layer();
        else
            disable_network_layer();
   }
}
