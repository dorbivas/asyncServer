#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "Ws2_32.lib")
#include <fstream>
#include <string.h>
#include <string>
#include <time.h>
#include <map>
#include <iostream>
#include <winsock2.h>


using namespace std;
using std::map;

const int TIME_PORT = 27015;
const int MAX_SOCKETS = 60;
constexpr int BUFF_MAX_SIZE = 2048;
const int SEND_TIME = 1;
const int SEND_SECONDS = 2;

enum Status { EMPTY, LSTN, RCV, IDLE, SND };
enum HttpReq { TRACE, DELETE_, PUT, POST, HEAD, GET, OPTIONS };

struct SocketState
{
	SOCKET id;
	Status recv;
	Status send;
	HttpReq sendSubType;
	time_t activeStemp;
	char buff[BUFF_MAX_SIZE];
	int len;
};

bool addSocket(SOCKET id, Status what, SocketState* sockets, int& socCount);
void removeSocket(int index, SocketState* sockets, int& socCount);
void acceptConnection(int index, SocketState* sockets, int& socCount);
void receiveMessage(int index, SocketState* sockets, int& socCount);
void sendMessage(int index, SocketState* sockets);
void sendTrace(int& fileSize, SocketState* sockets, int index, std::string& returnedMsg, time_t& currentTime, std::string& fileSizeInString, int& buffLen, char  sendBuff[2048]);
void updateSocStemp(SocketState* sockets, int& socCount);
void updateSubType(int index, SocketState* sockets);
string getLen(int index, SocketState* sockets);
int putReq(int index, char* filename, SocketState* sockets);



