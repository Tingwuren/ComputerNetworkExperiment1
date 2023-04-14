/*
 *不带ACK的Go-Back-N协议
*/
#include <stdio.h>
#include <string.h>
#include <stdbool.h>//使用布尔类型

#include "protocol.h"//库函数中包括的函数原型以及相关的宏定义
#include "datalink.h"//帧种类宏定义

#define DATA_TIMER  2000//计时器超时时间值为2000毫秒
#define MAX_SEQ 127//帧序列号为0到127之间
#define inc(k) if(k < MAX_SEQ) k = k+1; else k = 0//使k在0到127之间循环递增
struct FRAME { 
    unsigned char kind;//帧种类，一字节
    unsigned char ack;//ACK,一字节，表示发送方上一接收帧的序列号
    unsigned char seq;//SEQ，一字节，表示帧的序列号
    unsigned char data[PKT_LEN]; //数据，240到256字节
    unsigned int  padding;//CRC校验位，四字节
};

static unsigned char frame_nr = 0;//下一个要发送的帧序号
static unsigned char buffer[MAX_SEQ*PKT_LEN+PKT_LEN];//缓冲区大小为128个帧
static unsigned char nbuffered = 0;//发送窗口
static unsigned char ack_expected = 0;//未被确认收到的最早发送的帧
static unsigned char frame_expected = 0;//期待到达的帧序号
static unsigned char i;//缓冲区索引指针
static int phl_ready = 0;//物理层发送队列的长度低于50字节时为1，其余时候为0

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
    memcpy(s.data, &buffer[(int)frame_nr*PKT_LEN], PKT_LEN);

    dbg_frame("Send DATA %d %d, ID %d\n", s.seq, s.ack, *(short *)s.data);

    put_frame((unsigned char *)&s, 3 + PKT_LEN);
    start_timer(frame_nr, DATA_TIMER);
}

//数据链路层向物理层发送ACK帧
static void send_ack_frame(void)
{
    struct FRAME s;

    s.kind = FRAME_ACK;
    s.ack = (frame_expected+MAX_SEQ)%(MAX_SEQ+1);//ack为正确收到的帧的序号

    dbg_frame("Send ACK  %d\n", s.ack);

    put_frame((unsigned char *)&s, 2);
}

//如果a<=b<c循环返回真，否则返回假
static _Bool between(unsigned char a,unsigned char b,unsigned char c)
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

    protocol_init(argc, argv);//对运行环境初始化
    lprintf("Designed by Li Ruobin, build: " __DATE__"  "__TIME__"\n");//printf的改进函数，在输出前加时间坐标

    disable_network_layer();//缓冲区满时通过该函数通知网络层

    while (true) {
        event = wait_for_event(&arg);//返回事件之一，arg用于获取发生事件的相关信息

        switch (event) {
        case NETWORK_LAYER_READY:
        //接收，保存，发送一个新帧
            get_packet(&buffer[(int)frame_nr*PKT_LEN]);//获取新包
            nbuffered++;//发送窗口加一
            send_data_frame();//发送帧
            inc(frame_nr);//发送窗口上界加一
            break;

        case PHYSICAL_LAYER_READY://事件为物理层发送队列的长度低于50字节
            phl_ready = 1;
            break;

        case FRAME_RECEIVED://事件为物理层收到了一整帧
            len = recv_frame((unsigned char *)&f, sizeof f);//len是收到帧的实际长度
            //校验位出错
            if (len < 5 || crc32((unsigned char *)&f, len) != 0) {
                dbg_event("**** Receiver Error, Bad CRC Checksum\n");
                break;
            }
            if (f.kind == FRAME_ACK) 
                dbg_frame("Recv ACK  %d\n", f.ack);
            if (f.kind == FRAME_DATA) {
                dbg_frame("Recv DATA %d %d, ID %d\n", f.seq, f.ack, *(short *)f.data);
                //收到帧正确
                if (f.seq == frame_expected) {
                    //帧只允许按序接收
                    put_packet(f.data, len - 7);//将帧的数据域提交到网络层
                    inc(frame_expected);//接收窗口后移一位
                }
                send_ack_frame();
            }
            while (between(ack_expected,f.ack,frame_nr)){
                nbuffered = nbuffered - 1;
                stop_timer(ack_expected);
                inc(ack_expected);
            }
//            if (f.ack == frame_nr) {
//                stop_timer(frame_nr);
//                nbuffered--;
//                inc(frame_nr);
//            }
            break; 

        case DATA_TIMEOUT://事件为定时器超时
            dbg_event("---- DATA %d timeout\n", arg);
            frame_nr = ack_expected;
            for(i = 1;i <= nbuffered;i++){
                send_data_frame();
                inc(frame_nr);
            } 
            //send_data_frame();//立即重传上一个帧
            break;
        }

        if (nbuffered < MAX_SEQ && phl_ready)
            enable_network_layer();
        else
            disable_network_layer();
   }
}
