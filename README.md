Mifare-1K Cloner Firmware
=========================
Mifare-1K 卡片克隆器 For Auduino UNO

~~备份恢复已知密码卡片~~(KEY区不可读）

支持读写特殊白卡（Chinese Clone Card,可改写UID）

Chinese Clone Card
==================
一种我大华强北出产的可以擦写任意扇区（包括UID）的Mifare卡片
只要发送两个特殊指令即可解锁

* halt[4]={0x50, 0x00, 0x57, 0xcd};
* unlock1[1]={0x40};
* unlock2[1]={0x43};

方法提取自 nfc-tools/libnfc
