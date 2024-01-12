# 在otherprotocol目录下有多个文件夹
## 1.	不带ACK计时器的GBN协议文件夹
文件夹名称为noackgobackn。

文件夹含有数据链路层文件datalink.c。

以及多个测试结果文件：

有无误码信道数据传输的测试结果au.log和bu,log，

有无参数测试结果a.log和a.log，

有无误码信道和洪水方式测试结果afu.log和bfu.log，

有洪水方式测试结果af.log和bf.log，

有洪水方式线路误码率设为 10-4测试结果afber.log和bfber.log。
## 2.	带ACK计时器的GBN协议文件夹
文件夹名称为ackgobackn。

文件夹含有数据链路层文件datalink.c。

以及多个测试结果文件：

有ACK计时器超时时限为0ms的测试结果a0.log和b0.log，

有ACK计时器超时时限为200ms的测试结果a200.log和b200.log，

有ACK计时器超时时限为400ms的测试结果a400.log和b400.log，

有ACK计时器超时时限为600ms的测试结果a600.log和b600.log，

有ACK计时器超时时限为800ms的测试结果a800.log和b800.log，

有ACK计时器超时时限为1000ms的测试结果a1000.log和b1000.log，

有ACK计时器超时时限为1200ms的测试结果a1200.log和b1200.log。
## 3.	带ACK计时器和NAK帧的GBN协议文件夹
文件夹名称为acknakgobackn。

文件夹含有数据链路层文件datalink.c。

以及多个测试结果文件：

有无参数的测试结果datalink-A.log和datalink-B.log。
## 4.  SR协议文件夹
文件夹名称为selectiverepeat。

文件夹含有数据链路层文件datalink.c。

以及多个测试结果文件：

有无误码信道数据传输的测试结果au.log和bu,log，

有无参数测试结果a.log和a.log，

有无误码信道和洪水方式测试结果afu.log和bfu.log，

有洪水方式测试结果af.log和bf.log，

有洪水方式线路误码率设为 10-4测试结果afber.log和bfber.log。
