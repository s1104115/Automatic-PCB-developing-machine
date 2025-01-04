// TCP Client program 
#include <netinet/in.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h>
#include <signal.h>
#include <wiringPi.h>
#include <linux/i2c-dev.h> 
#include <fcntl.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <dirent.h>
#include "ads1115.h"

const char* host = "10.3.141.1";
int port = 7000;

int gProcessRun = 1;

const int relayPin_1 = 17;
const int relayPin_2 = 18;

const int motoPin_1 = 20;
const int motoPin_2 = 21;


static double voltage;
static int temp_v = 0;
static int timer_v = 10;
static int s_temp_v = 45;
static int cur_stat = 0;
static int call_stop = 0;
static int heater_stat = 0;

static void sigstop(int arg)
{
	printf("Receive sigstop\n");

	gProcessRun = 0;

	exit(0);
}

static int myAnalogRead(struct wiringPiNodeStruct *node, int pin) 
{
	int chan = pin - node->pinBase;
  	int data[2];
  	int value;

  	// Start with default values
  	int config = ADS1015_REG_CONFIG_CQUE_NONE    | // Disable the comparator (default val)
                    ADS1015_REG_CONFIG_CLAT_NONLAT  | // Non-latching (default val)
                    ADS1015_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
                    ADS1015_REG_CONFIG_CMODE_TRAD   | // Traditional comparator (default val)
                    ADS1115_REG_CONFIG_DR_860SPS   | // 860 samples per second (max)
                    ADS1015_REG_CONFIG_MODE_SINGLE;   // Single-shot mode (default)
                    //ADS1015_REG_CONFIG_MODE_CONTIN;   // Continuous mode (doesn't work with more than one channel)
  	// Set PGA/voltage range
  	config |= ADS1015_REG_CONFIG_PGA_4_096V;

  	// Set single-ended input chan
 	switch (chan)
  	{
    		case (0):
      			config |= ADS1015_REG_CONFIG_MUX_SINGLE_0;
      		break;
    		case (1):
      			config |= ADS1015_REG_CONFIG_MUX_SINGLE_1;
      		break;
    		case (2):
      			config |= ADS1015_REG_CONFIG_MUX_SINGLE_2;
      		break;
    		case (3):
      			config |= ADS1015_REG_CONFIG_MUX_SINGLE_3;
      		break;
  	}
  
  	// Set 'start single-conversion' bit
  	config |= ADS1015_REG_CONFIG_OS_SINGLE;
  
  	// Sent the config data in the right order
  	config = ((config >> 8) & 0x00FF) | ((config << 8) & 0xFF00);
 	wiringPiI2CWriteReg16(node->fd, ADS1015_REG_POINTER_CONFIG, config);
 
  	// Wait for conversion to complete
  	delay(2); // (1/SPS rounded up)

  	wiringPiI2CWrite(node->fd, ADS1015_REG_POINTER_CONVERT);
  	data[0] = wiringPiI2CRead(node->fd);
  	data[1] = wiringPiI2CRead(node->fd);
  	value = ((data[0] << 8) & 0xFF00) | data[1];

 	 // wiringPi doesn't include stdint so everything is an int (int32), this should account for this
  	if (value > 0x7FFF) {
    		return (value - 0xFFFF);
  	} else {
    		return value;
  	}
}

/* ADS1115 ADC setup:
 *    create ADS1115 device.
 *    id is the address of the chip (0x48 default)
*===============================================*/

int ads1115Setup(const int pinBase, int id) 
{
  	struct wiringPiNodeStruct *node;

  	node = wiringPiNewNode(pinBase,4);

 	node->fd = wiringPiI2CSetup(id);
  	node->analogRead = myAnalogRead;

  	if (node->fd < 0) {
    		return -1;
  	} else {
    		return 0;
  	}
}

void *thread_MotoCWHandle(void *arg) 
{
	setAngle(motoPin_1,90);
	setAngle(motoPin_2,90);
	sleep(5);
	softPwmStop(motoPin_1);
	softPwmStop(motoPin_2);
}

void *thread_MotoCCWHandle(void *arg) 
{
	setAngle(motoPin_1,-90);
	setAngle(motoPin_2,-90);
	sleep(5);
	softPwmStop(motoPin_1);
	softPwmStop(motoPin_2);
}

void motor_BringUp(void)
{
	softPwmCreate(motoPin_1, 0, 200);
	softPwmCreate(motoPin_2, 0, 200);
	setAngle(motoPin_1,-90);
	setAngle(motoPin_2,-90);
	sleep(5);
	softPwmStop(motoPin_1);
	softPwmStop(motoPin_2);
}

void motor_DropDown(void)
{
	softPwmCreate(motoPin_1, 0, 200);
	softPwmCreate(motoPin_2, 0, 200);
	setAngle(motoPin_1,90);
	setAngle(motoPin_2,90);
	sleep(5);
	softPwmStop(motoPin_1);
	softPwmStop(motoPin_2);
}

void relay_Trigger(void)
{
	digitalWrite(relayPin_1, HIGH);
	digitalWrite(relayPin_2, HIGH);
	sleep(2);
	digitalWrite(relayPin_1, LOW);
	digitalWrite(relayPin_2, LOW);
}

void *thread_TimerHandle(void *arg) 
{
	int tmp_cnt = timer_v;
	printf("============Enter thread_TimerHandle==============\n");

	motor_DropDown();
	relay_Trigger();

	while(cur_stat) {
		if(tmp_cnt <= 0) {
			printf("******Timeout %d*****\n",tmp_cnt);
			break;
		}

		printf("Clean Count Down = %d\n",tmp_cnt);

		tmp_cnt--;
		sleep(1);
	}

	printf("Leave thread_TimerHandle\n");

	cur_stat = 0;
	heater_stat = 0;
	relay_Trigger();
	motor_BringUp();
	call_stop = 1;

	pthread_exit(0);
}

void *thread_ServerConnectHandle(void *arg) 
{
	int sock_fd;
  	 struct sockaddr_in serv_name;
    	int status;
    	char indata[32] = {0}, outdata[32] = {0};
	int tmp_cnt = 0;
	int nbytes = 0;
	char t_data[32];

	printf("============Enter thread_ServerConnectHandle==============\n");

	// create a socket
    	sock_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
   	if (sock_fd == -1) {
       	perror("Socket creation error");
        	exit(1);
    	}

	// server address
    	serv_name.sin_family = AF_INET;
    	inet_aton(host, &serv_name.sin_addr);
    	serv_name.sin_port = htons(port);

	while(gProcessRun) {
		status = connect(sock_fd, (struct sockaddr *)&serv_name, sizeof(serv_name));
    		if (status == -1) {
        		//perror("Connection error");
        		sleep(1);
   		 } else {
			printf("Connect Success\n");
			break;
   		 }
	}

	while(gProcessRun)
	{
		nbytes = 0;

		if(call_stop == 1) {
			call_stop = 0;
			memset(outdata, 0, 32);
			sprintf(outdata,"-1,-1");
			nbytes = send(sock_fd, outdata, strlen(outdata), 0);
		}

		if(tmp_cnt > 2) {
			memset(outdata, 0, 32);
			sprintf(outdata,"%02d,%.2f",temp_v,voltage+0.01);
			nbytes = send(sock_fd, outdata, strlen(outdata), 0);
			tmp_cnt = 0;
		}
		memset(indata,0,32);
        	int nbytes = recv(sock_fd, indata, sizeof(indata), 0);
		if(nbytes > 0) {
			printf("recv cmd: %s\n", indata);
			if(strstr(indata,"Start")!=NULL) {
				printf("*******Start Clean******\n");
				cur_stat = 1;
				heater_stat = 1;
				pthread_t ThreadTimer_ID;	
				pthread_create(&ThreadTimer_ID, NULL, &thread_TimerHandle, NULL);
				pthread_detach(ThreadTimer_ID);
			}else if(strstr(indata,"Stop")!=NULL) {
				printf("*******Stop Clean******\n");
				cur_stat = 0;
				heater_stat = 0;
			} else if(strstr(indata,"Tur")!=NULL) {
				memset(t_data,0,32);
				sprintf(t_data,"%s",indata+4);
				printf("Setup Timer = %d\n",atoi(t_data));
				timer_v = atoi(t_data);
			} else if(strstr(indata,"Temp")!=NULL) {
				memset(t_data,0,32);
				sprintf(t_data,"%s",indata+5);
				printf("Setup Temperature = %d\n",atoi(t_data));
				timer_v = atoi(t_data);
			} 
		} else if(nbytes == 0) {
			printf("Server close\n");
			close(sock_fd);
			break;
		}
		tmp_cnt++;
		usleep(50000);
	}

	printf("Leave thread_ServerConnectHandle\n");
	pthread_exit(0);
}

void read_temperature(char *path)
{
	char buf[100];
	int fd =-1;
    	char *temp;
    	float value;

	// Open the file in the path.
        if((fd = open(path,O_RDONLY)) < 0)
        {
            printf("open error\n");
            return 1;
        }
        // Read the file
        if(read(fd,buf,sizeof(buf)) < 0)
        {
            printf("read error\n");
            return 1;
        }
        // Returns the first index of 't'.
        temp = strchr(buf,'t');
        // Read the string following "t=".
        sscanf(temp,"t=%s",temp);
        // atof: changes string to float.
        value = atof(temp)/1000;
	temp_v = value;
        printf(" temp : %3.3f / %02d\n",value,temp_v);
        close(fd);
}

long Map(long value,long fromLow,long fromHigh,long toLow,long toHigh){
    return (toHigh-toLow)*(value-fromLow) / (fromHigh-fromLow) + toLow;
}

void setAngle(int pin, int angle){    //Create a funtion to control the angle of the servo.
    if(angle < 0)
        angle = 0;
    if(angle > 180)
        angle = 180;
    softPwmWrite(pin,Map(angle, 0, 180, 5, 25));
}

int main(void) 
{ 
	int turn = 0;
	short value;
	char path[50] = "/sys/bus/w1/devices/";
    	char rom[20];
    	DIR *dirp;
   	struct dirent *direntp;
	int tmp_sensor = 0;
	int i;

	//sleep(10);

  
	ads1115Setup(100,0x48);

	signal(SIGINT,	sigstop);
	signal(SIGTERM, sigstop);

	wiringPiSetupGpio();

	pinMode(relayPin_1, OUTPUT);
	pinMode(relayPin_2, OUTPUT);

	digitalWrite(relayPin_1, HIGH);
	digitalWrite(relayPin_2, HIGH);

	if((dirp = opendir(path)) == NULL)
    	{
        	printf("opendir error %s\n",path);
        	tmp_sensor = -1;
    	}

	if(tmp_sensor >= 0) {
		while((direntp = readdir(dirp)) != NULL)
    		{
       		 // If 28-00000 is the substring of d_name,
        		// then copy d_name to rom and print rom.  
       	 	if(strstr(direntp->d_name,"28-00000"))
        		{
            			strcpy(rom,direntp->d_name);
            			printf(" fine tepm sensor rom: %s\n",rom);
        		}
    		}
    		closedir(dirp);
	}

	strcat(path,rom);
    	strcat(path,"/w1_slave");
		
	pthread_t ThreadCSrv_ID;	
	pthread_create(&ThreadCSrv_ID, NULL, &thread_ServerConnectHandle, NULL);
	pthread_detach(ThreadCSrv_ID);

	while(gProcessRun) {
		value = (short) analogRead(100);
    
		voltage = value * (4.096 / 32768);
		
		//printf("ADS1115 Reading: %d\n\r",value);
    
		printf("ADS1115 Voltage: %.2f\n\r",voltage);

		if(heater_stat == 1) {
			printf("Check Heater Temperature = %d / %d\n",s_temp_v,temp_v);

			if(temp_v >= s_temp_v) {
				printf("Temperature >= %d\n",temp_v);
				digitalWrite(relayPin_2, HIGH);
				sleep(2);
				digitalWrite(relayPin_2, LOW);
			}
			
		}

		read_temperature(path);
		
		sleep(1);
	}

	return 0;
} 

