#ifndef __PROTOCOL_fr12hn_H__

#ifdef  __cplusplus
extern "C" {
#endif

#define VERSION "4.0"

#ifndef	_CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <string.h>

#include "lprintf.h"

/* Initalization */ 
//对运行环境初始化
extern void protocol_init(int argc, char **argv);

/* Event Driver */
//导致进程等待，直到一个“事件”发生
extern int wait_for_event(int *arg);
//网络层有待发送的分组
#define NETWORK_LAYER_READY  0
//物理层发送队列的长度低于50字节
#define PHYSICAL_LAYER_READY 1
//物理层收到了一整帧
#define FRAME_RECEIVED       2
//定时器超时，参数arg中返回发生超时的定时器的编号
#define DATA_TIMEOUT         3
//所设置的搭载ACK定时器超时
#define ACK_TIMEOUT          4

/* Network Layer functions */
//网络层包的大小为256字节
#define PKT_LEN 256
//在能够承接新的发送任务时执行该函数允许网络层发送数据分组
extern void enable_network_layer(void);
//数据链路层在缓冲区满等条件下无法发送分组时通过该函数通知网络层
extern void disable_network_layer(void);
//将分组拷贝到packet指定的缓冲区中，返回值为分组长度
extern int  get_packet(unsigned char *packet);
//将分组提交到网络层，参数为存放收到分组的缓冲区首地址和分组长度
extern void put_packet(unsigned char *packet, int len);

/* Physical Layer functions */
//从物理层接收一帧，size为用于存放接收帧的缓冲区buf的空间大小，返回值为收到帧的实际长度
extern int  recv_frame(unsigned char *buf, int size);
//将内存frame处长度为len的缓冲区块向物理层发送为一帧
extern void send_frame(unsigned char *frame, int len);
//返回当前物理层队列的长度
extern int  phl_sq_len(void);

/* CRC-32 polynomium coding function */
//返回32位CRC校验码
extern unsigned int crc32(unsigned char *buf, int len);

/* Timer Management functions */
//获取当前的时间坐标，单位为毫秒
extern unsigned int get_ms(void);
//启动一个定时器，两个参数分别为计时器的编号和超时时间值
extern void start_timer(unsigned int nr, unsigned int ms);
//中止一个定时器
extern void stop_timer(unsigned int nr);
//启动ACK计时器
extern void start_ack_timer(unsigned int ms);
//终止ACK计时器
extern void stop_ack_timer(void);

/* Protocol Debugger */
//获取当前进程所对应的站点名，为字符串“A”或者“B”
extern char *station_name(void);
//当debug_mask的0号比特为0时，dbg_event()的所有输出被忽略
extern void dbg_event(char *fmt, ...);
//当debug_mask的1号比特为0时，dbg_frame()的所有输出被忽略
extern void dbg_frame(char *fmt, ...);
//当debug_mask的2号比特为0时，dbg_warning()的所有输出被忽略
extern void dbg_warning(char *fmt, ...);
//在输出信息之前首先输出当前的时间坐标
#define MARK lprintf("File \"%s\" (%d)\n", __FILE__, __LINE__)

#define inc(k) if(k < MAX_SEQ) k = k+1; else k = 0
#ifdef  __cplusplus
}
#endif

#endif
