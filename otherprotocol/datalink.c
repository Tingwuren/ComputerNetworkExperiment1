#include <stdio.h>
#include <string.h>

#include "protocol.h"//库函数中包括的函数原型以及相关的宏定义
#include "datalink.h"//帧种类宏定义

#define DATA_TIMER  2000//计时器超时时间值为2000毫秒

struct FRAME { 
    unsigned char kind;//帧种类，一字节
    unsigned char ack;//ACK,一字节
    unsigned char seq;//SEQ，一字节
    unsigned char data[PKT_LEN]; //数据，240到256字节
    unsigned int  padding;//CRC校验位，四字节
};

static unsigned char frame_nr = 0, buffer[PKT_LEN], nbuffered;//缓冲区大小为256字节
static unsigned char frame_expected = 0;//期待到达的帧编号为0
static int phl_ready = 0;//物理层发送队列的长度低于50字节时为1，其余时候为0

static void put_frame(unsigned char *frame, int len)//数据链路层将分组提交到网络层
{
    *(unsigned int *)(frame + len) = crc32(frame, len);
    send_frame(frame, len + 4);
    phl_ready = 0;
}

static void send_data_frame(void)//数据链路层向物理层发送数据帧
{
    struct FRAME s;

    s.kind = FRAME_DATA;
    s.seq = frame_nr;
    s.ack = 1 - frame_expected;
    memcpy(s.data, buffer, PKT_LEN);

    dbg_frame("Send DATA %d %d, ID %d\n", s.seq, s.ack, *(short *)s.data);

    put_frame((unsigned char *)&s, 3 + PKT_LEN);
    start_timer(frame_nr, DATA_TIMER);
}

static void send_ack_frame(void)//数据链路层向物理层发送ACK帧
{
    struct FRAME s;

    s.kind = FRAME_ACK;
    s.ack = 1 - frame_expected;

    dbg_frame("Send ACK  %d\n", s.ack);

    put_frame((unsigned char *)&s, 2);
}

int main(int argc, char **argv)//两个参数提供从命令行参数中获取配置系统参数的手段
{
    int event, arg;//声明事件及事件的相关信息
    struct FRAME f;//声明帧f
    int len = 0;

    protocol_init(argc, argv);//对运行环境初始化
    lprintf("Designed by Jiang Yanjun, build: " __DATE__"  "__TIME__"\n");//printf的改进函数，在输出前加时间坐标

    disable_network_layer();//缓冲区满时通过该函数通知网络层

    for (;;) {
        event = wait_for_event(&arg);//返回事件之一，arg用于获取发生事件的相关信息

        switch (event) {
        case NETWORK_LAYER_READY://事件为网络层有待发送的组
            get_packet(buffer);//将分组拷贝到buffer缓冲区
            nbuffered++;
            send_data_frame();
            break;

        case PHYSICAL_LAYER_READY://事件为物理层发送队列的长度低于50字节
            phl_ready = 1;
            break;

        case FRAME_RECEIVED://事件为物理层收到了一整帧
            len = recv_frame((unsigned char *)&f, sizeof f);
            if (len < 5 || crc32((unsigned char *)&f, len) != 0) {
                dbg_event("**** Receiver Error, Bad CRC Checksum\n");
                break;
            }
            if (f.kind == FRAME_ACK) 
                dbg_frame("Recv ACK  %d\n", f.ack);
            if (f.kind == FRAME_DATA) {
                dbg_frame("Recv DATA %d %d, ID %d\n", f.seq, f.ack, *(short *)f.data);
                if (f.seq == frame_expected) {
                    put_packet(f.data, len - 7);
                    frame_expected = 1 - frame_expected;
                }
                send_ack_frame();
            } 
            if (f.ack == frame_nr) {
                stop_timer(frame_nr);
                nbuffered--;
                frame_nr = 1 - frame_nr;
            }
            break; 

        case DATA_TIMEOUT://事件为定时器超时
            dbg_event("---- DATA %d timeout\n", arg); 
            send_data_frame();
            break;
        }

        if (nbuffered < 1 && phl_ready)
            enable_network_layer();
        else
            disable_network_layer();
   }
}
