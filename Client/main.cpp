#include <iostream>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <malloc.h>
#include <sys/ioctl.h>
#include <opencv2/opencv.hpp>
#include <fcntl.h>
#include <stdio.h>
#include <bitset>
#include <time.h>
#include "lib/iniParser.hpp"
 
using namespace std;
using namespace cv;

#define MODULE_PATH "/dev/gpioPWM"
#define TRIG_H "H17"
#define TRIG_L "L17"

vector<string> IP;
vector<int> UDP_PORT;
vector<int> TCP_PORT;
int MOT_PORT;
char* DIR[4] = {"BF","FB","BB","FF"};
clock_t sd = 0;
uchar* buf;
bool key = false;
int WIDTH, HEIGHT, S_NUM, TOTAL_S;
unsigned long long SIZE;
void UDPData(int id, int num);
void TCPData(int id, int num);
void motorControl();
static int fd;
int v = 900;
vector<unsigned char> cmd;

int main(int argc, char **argv){
	sd = clock();
	fd = open(MODULE_PATH, O_RDWR);
	iniParser config("../config.ini");
	IP = config.getStringVector("ip");
	UDP_PORT = config.getNumVector<int>("udp_port");
	TCP_PORT = config.getNumVector<int>("tcp_port");
	MOT_PORT = config.getInt("motor_port");
	WIDTH = config.getInt("width");
	HEIGHT = config.getInt("height");
	S_NUM = config.getInt("split");
	SIZE = (unsigned long long)WIDTH*HEIGHT*3;
	TOTAL_S = (int)(SIZE/(UDP_PORT.size()*S_NUM));
	printf("Frame(%d*%d) - Total Byte Size : %llu, split per udp : %d (UDP Trasfer Byte size : %d)\n",HEIGHT, WIDTH, SIZE, S_NUM, TOTAL_S);
	buf = new uchar[SIZE];
	bool rec = true;
	vector<thread> th_udp;
	vector<thread> th_tcp;
	for(int i = 0; i < IP.size(); i++){
		for(int j = 0; j < UDP_PORT.size(); j++){
			th_udp.push_back(thread(UDPData,i,j));
		}
		for(int j = 0; j < TCP_PORT.size(); j++){
			th_tcp.push_back(thread(TCPData,i,j));
		}
	}
	thread m_control(motorControl);
	//Mat frame, send;
	Mat frame(HEIGHT, WIDTH, CV_8SC3, Scalar(0,0,0)), send;
	key = false;

	memset(buf, 0, SIZE*sizeof(uchar));

	VideoCapture cap(-1);
	
	//if(!cap.isOpened()) {
		//cerr << "Failed to open camera";
		//exit(1);
	//}

	while(1){
		if(clock() > sd + CLOCKS_PER_SEC/10) {
			write(fd, TRIG_H, 3);
			usleep(10);
			write(fd, TRIG_L, 3);
			sd = clock();
		}
		if(cap.isOpened()){
			cap >> frame;
			resize(frame, frame, Size(WIDTH, HEIGHT), 0, 0, INTER_LINEAR);
			//flip(frame, frame, -1);
		}
		std::memcpy(buf, frame.data, SIZE*sizeof(uchar));
		if(waitKey(1) >= 0 || key) {key=true;break;}
	}
	for(int i = 0; i < th_udp.size(); i++)
		th_udp[i].join();
	for(int i = 0; i < th_tcp.size(); i++)
		{
				pthread_cancel(th_tcp[i].native_handle());
				th_tcp[i].join();
		}
	m_control.join();
	free(buf);		printf("Program eixt.\n");
	return 0;
	 
	
}

void UDPData(int id, int num)
{
	int UDP_SOCK;
	sockaddr_in serveraddr;
	uchar* tmp = new uchar[TOTAL_S+1];
	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(UDP_PORT[num]);
	serveraddr.sin_addr.s_addr=inet_addr(IP[id].c_str());
	if ((UDP_SOCK = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		printf("Creating Socket is Failed.");
		return;
	}
	printf("UDP Socket%d-%d(communication with %s:%d) created.\n",id+1,num+1,IP[id].c_str(), UDP_PORT[num]);
	while(true)
	{
		for(int i = 0; i < S_NUM; i++){
				tmp[0] = num*S_NUM + i;
				std::memcpy(&tmp[1], &buf[(int)tmp[0]*TOTAL_S], TOTAL_S);
				sendto(UDP_SOCK, tmp, TOTAL_S+1, 0, (struct sockaddr*)&serveraddr, sizeof(serveraddr));  		
		}
		
		if(key) break;
	}
	free(tmp);
}
void motorControl(){
	system("sudo insmod ../module/gpio_module.ko");
	system("sudo chmod 666 /dev/gpioPWM");
	
	int UDP_SOCK;
	char cmd, backup;
	sockaddr_in serveraddr;
	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(MOT_PORT);
	serveraddr.sin_addr.s_addr=inet_addr(IP[0].c_str());
	unsigned int NSIZE = sizeof(serveraddr);
	if ((UDP_SOCK = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		printf("Creating Socket is Failed.");
		return;
	}
	printf("Control Motor socket (communication with %s:%d) created.\n",IP[0].c_str(), MOT_PORT);
	::bind(UDP_SOCK, (struct sockaddr*)&serveraddr, NSIZE);
	printf("Send to server about Control Motor socket address.\n");
	sendto(UDP_SOCK, &cmd, 1, 0, (struct sockaddr*)&serveraddr, NSIZE);  		
	bool err1, err2, comp;
	while(true){
		cmd = 0;
		backup = cmd;
		recvfrom(UDP_SOCK, &cmd, 1, 0, (struct sockaddr*)&serveraddr, &NSIZE);
		recvfrom(UDP_SOCK, &backup, 1, 0, (struct sockaddr*)&serveraddr, &NSIZE);
		if(cmd != backup) cmd = 0;
		
		if(cmd >> 7) key=true;
		if(key) break;
		if(cmd >> 6){
			if(cmd & 32) { 
				/*cmd &= 223;
				if(cmd & 3)v= 825;
				else v = 775;*/
				if(cmd & 15){
					write(fd, DIR[(bool)(cmd & 16)], 2);
					cmd = cmd & 15;
					ioctl(fd, 825	);
					ioctl(fd, -825);
					usleep(80000*cmd);
					ioctl(fd, 0);
					ioctl(fd, 2147483647+1);
					continue;
				}
				else{
					cmd = 8;
					v= 775;
				}
			}
		}
		if(cmd&16) v = v < 1000 ? v + 25 : 1000;
		if(cmd&32) v = v > 700 ? v - 25 : 700;
		if((cmd&3) == 3) cmd = cmd & 252;
		if((cmd&12) == 12) cmd = cmd & 243;
		if(cmd&15){
			if(cmd&3) write(fd, DIR[(cmd&3) - 1], 2);
			else write(fd, DIR[(cmd&12)/4 + 1], 2);
			ioctl(fd, v + ((cmd&1)&&(cmd&12))*100); //(cmd&2)&&(cmd&12)
			ioctl(fd, -v - ((cmd&2)&&(cmd&12))*100); //(cmd&1)&&(cmd&12)
		}
		else{
			ioctl(fd, 0);
			ioctl(fd, 2147483647+1);
		}
		/*if(cmd >> 6){
			sleep(1);
			ioctl(fd, 0);
			ioctl(fd, 2147483647+1);
		}*/
			
		
	}
	ioctl(fd, 0);
	ioctl(fd, 2147483647+1);
}
void TCPData(int id, int num){
	system("sudo insmod ../module/gpio_module.ko");
	system("sudo chmod 666 /dev/gpioPWM");
	//if (chmod(MODULE_PATH, 0666) == -1) printf("Failed to change module access authority!\n");
	int fd = open(MODULE_PATH, O_RDWR);
	sockaddr_in serveraddr;
	int sock;
	char cmd = 0;
	int len;
	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(TCP_PORT[num]);
	serveraddr.sin_addr.s_addr=inet_addr(IP[id].c_str());
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Creating Socket is Failed.");
		return;
	}
	printf("TCP Socket%d-%d(%s:%d) created.\n",id+1,num+1,IP[id].c_str(), TCP_PORT[num]);
	if (connect(sock,(struct sockaddr*)&serveraddr,sizeof(serveraddr)) < 0){
		printf("Connect Error!");
		return;
	}
	printf("TCP Socket%d-%d(%s:%d) connect successfully.\n",id+1,num+1,IP[id].c_str(), TCP_PORT[num]);
	while(true){
		len = read(sock, &cmd, 1);
		char err = cmd&1 + (cmd >>1)&1 + (cmd >>2)&1+ (cmd >>3)&1+ (cmd >>4)&1+ (cmd >>5)&1+ (cmd >>6)&1+ (cmd >>7)&1;
		if(err % 2 != 0) continue;
		if(cmd >> 7) key=true;
		if(key) break;
		if(cmd&16) v = v < 1000 ? v + 50 : 1000;
		if(cmd&32) v = v > 450 ? v - 50 : 450;
		if((cmd&3) == 3) cmd = cmd & 252;
		if((cmd&12) == 12) cmd = cmd & 243;
		if(cmd&15){
			if(cmd&3) write(fd, DIR[(cmd&3) - 1], 2);
			else write(fd, DIR[(cmd&12)/4 + 1], 2);
			ioctl(fd, v - v*((cmd&2)&&(cmd&12))/2);
			ioctl(fd, -v + v*((cmd&1)&&(cmd&12))/2);
		}
		else{
			ioctl(fd, 0);
			ioctl(fd, 2147483647+1);
		}
	}
	ioctl(fd, 0);
	ioctl(fd, 2147483647+1);
	printf("TCP Socket%d-%d(%s:%d) close successfully.\n",id+1,num+1,IP[id].c_str(), TCP_PORT[num]);

	close(sock);
}
