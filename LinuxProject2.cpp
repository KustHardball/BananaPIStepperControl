#include <iostream>
#include <wiringPi.h>
#include <pthread.h>
#include <crypt.h>
#include <time.h>
#include <wiringSerial.h>
#include <unistd.h>
#include <semaphore.h>

using namespace std;
#define LED     29
#define stepperMaxSpeed    15
#define stepperMinSpeed    10
#define INA    26
#define INB    28
#define INC    27
#define IND    29
#define UART0    '/dev/ttyS0'
#define UART2    '/dev/ttyS2'
#define UART3    '/dev/ttyS3'
#define recfreq 200  //������� ��������� ������ � �������� (������ ���� ���� ������� ��������)
#define inputPocket 2  //����� ��������� ������ � ������
#define stepperSpeed 890 //������� ������� �����
#define speedPercent 3  //���������� ������� ��������
#define coef 0.4 // ����. ������������ ������� ��������

void * stepperControll(void *arg);
void * controll(void *arg);
void  dostep(unsigned int duration, bool path);
void byteDecode(unsigned char byte);
void stepsDecode(unsigned char byte);

int steps = 100;
bool buttons[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
sem_t semaphore;
sem_t secsemaphore;


int main(int argc, char *argv[])
{
	wiringPiSetup();                  //�������������� ����������
	int result;
	sem_init(&semaphore, 0, 1);
	sem_init(&secsemaphore, 0, 1);
	pthread_t stepperThread;
	pthread_t controlThread;
	result = pthread_create(&controlThread, NULL, controll, &steps); //������ ������ ������ � ������� �������������� UART
	result = pthread_create(&stepperThread, NULL, stepperControll, &steps); //������ ������ ���������� ���������
	//result = pthread_join(stepperThread, NULL);  //�������� ���������� ������
	result = pthread_join(controlThread, NULL);  //�������� ���������� ������
	cout << steps << endl;
	cout << "Finish!" << endl;	
	return 0;
}

void * stepperControll(void *arg) {
	
	
	pinMode(INA, OUTPUT);
	pinMode(INB, OUTPUT);
	pinMode(INC, OUTPUT);
	pinMode(IND, OUTPUT);
	digitalWrite(INA, LOW);
	digitalWrite(INB, LOW);
	digitalWrite(INC, LOW);
	digitalWrite(IND, LOW);
	int maxsp = stepperMaxSpeed;
	int minsp = stepperMinSpeed;
	unsigned int currentspeed=stepperSpeed;
	double percent = speedPercent;
	while (1 == 1)
	{   sem_wait(&semaphore);
		if (steps > 0)
		{
			sem_post(&semaphore);
			dostep(currentspeed, TRUE);
			if (steps > 4 | steps < 4)
			{
				currentspeed = (int)(((double)currentspeed / 100)*((double)100 - percent));
				if (percent > 0.1) percent = percent * coef; 
				else percent = 0;
			}
		} 
		else if (steps < 0)
		{
			sem_post(&semaphore);
			dostep(currentspeed, FALSE);
			currentspeed = (int)((currentspeed/100)*(100-percent));
			if (percent > 0) percent = percent * coef; 
		}
		sem_wait(&semaphore);
		if (steps == 0) {
			sem_post(&semaphore);
			currentspeed = stepperSpeed;
			percent = speedPercent;
			digitalWrite(INA, LOW);
			digitalWrite(INB, LOW);
			digitalWrite(INC, LOW);
			digitalWrite(IND, LOW);
			sem_wait(&secsemaphore);
		}
		sem_post(&semaphore);
		sem_post(&secsemaphore);
	}
}
void  dostep(unsigned int duration, bool path) {
	static int position = 0;
	switch (position)
	{
	case 0:
	{
		digitalWrite(INA, HIGH);
		digitalWrite(INB, HIGH);
		digitalWrite(INC, LOW);
		digitalWrite(IND, LOW);
		if (path)
		{
			position = 1;
			steps--;
		}
		else
		{
			position = 3;
			steps++;
		}
		
			break;
		}
	case 1:
	{
		digitalWrite(INA, LOW);
		digitalWrite(INB, HIGH);
		digitalWrite(INC, HIGH);
		digitalWrite(IND, LOW);
		if (path)
		{
			position = 2;
			steps--;
		}
		else
		{
			position = 0;
			steps++;
		}
			break;
		}
	case 2:
	{
		digitalWrite(INA, LOW);
		digitalWrite(INB, LOW);
		digitalWrite(INC, HIGH);
		digitalWrite(IND, HIGH);
		if (path)
		{
			position = 3;
			steps--;
		}
		else
		{
			position = 1;
			steps++;
		}
			break;
		}
	case 3:
	{
		digitalWrite(INA, HIGH);
		digitalWrite(INB, LOW);
		digitalWrite(INC, LOW);
		digitalWrite(IND, HIGH);
		if (path)
		{
			position = 0;
			steps--;
		}
		else
		{
			position = 2;
			steps++;
		}
			break;
		}
	}
	delayMicroseconds(duration);
}

void * controll(void *arg) {
	int port=serialOpen("/dev/ttyS2", 115200);//��������� ���� �� ����� �15 � 16
	if (port < 0)
	{
		cout << "Can't open reciever port" << endl; 
		;
	}
	int refresh = 1000 / recfreq;                     //��������� ������� �������� ������
	unsigned char input[inputPocket] =  {0,0};                  //���������� ��� ���������� �������� �������� ������
	while (1 == 1)
	{
		if (int bytes = serialDataAvail(port) >= 2) {  //��������� ������� ������ � ������ ���������
			for (int i = 0; i < inputPocket; i++)     //��������� ������ � �������� � ������
			{
				input[i] = serialGetchar(port);				
				}
			serialFlush(port);                        //������ ����� ����� ���������� ���� �������� ��������, ������ ������ ���� ��������
			byteDecode(input[0]);                     //����������� ������ �������� ����
			stepsDecode(input[1]);
			int writestatus = write(port, &input[0], 1);   //���� �������� �����
			
			}
		
		delay(refresh);
	}		
}

void byteDecode(unsigned char byte){  //��������� �������� ���� �� ����� � ������ ��� ���������� ������� �� �����������	 
	for (int i = 7; i>=0;i--)
	{		
		if (byte & 1) buttons[i] = 1;
		else buttons[i] = 0;
		byte = byte >> 1;
	}
	
	
}
void stepsDecode(unsigned char byte){  //�������������� ����� ASCII � ����� �� 127 (������ ��� ���������� ����)
	int value=0;
	sem_wait(&secsemaphore);
	value = byte | value;
	//byte = byte >> 7; 
	if (byte & 128 != 128) {
		value = ~value + 1;
		//cout <<  "Bit" << endl;
	}
	if (value != 0)
	{
		sem_wait(&semaphore);		
		if (value > 127) {
			value -= 127;
			steps = steps - value;
		}
		else steps += value;
		if (steps != 0) sem_post(&secsemaphore);
		cout << steps << " steps" << endl; 
		sem_post(&semaphore);
	}
	
}