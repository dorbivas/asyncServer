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

const int MAX_Q_SIZE = 5;
const int TIME_PORT = 27015;
const int MAX_SOCKETS = 69;
constexpr int BUFF_MAX_SIZE = 4096;
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
	int lan;
};

bool addSocket(SOCKET id, Status what, SocketState* sockets, int& socCount);
void removeSoc(int index, SocketState* sockets, int& socCount);
void sendTrace(int& fileSize, SocketState* sockets, int index, std::string& retMsg, time_t& currTime, std::string& fSize, int& buffLen, char  sendBuff[2048]);
void acceptConnection(int index, SocketState* sockets, int& socCount);
void receiveMssge(int index, SocketState* sockets, int& socCount);
void sendMessage(int index, SocketState* sockets);
void displayContent(std::ifstream& httpFile, std::string& returnedMsg, time_t& currentTime, std::string& fileSizeInString, int fileSize, int& buffLen, char  sendBuff[2048]);
void updateSocStemp(SocketState* sockets, int& socCount);
void updateSubType(int index, SocketState* sockets);
string getLen(int index, SocketState* sockets);
int putReq(int index, char* file, SocketState* sockets);



