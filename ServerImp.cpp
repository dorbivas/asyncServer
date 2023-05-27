#include "ServerImp.h"


bool addSocket(SOCKET id, Status what, SocketState* sockets, int& socCount)
{
	// Set the socket to be in non-blocking mode.
	unsigned long flag = 1;
	if (ioctlsocket(id, FIONBIO, &flag) != 0)
	{
		cout << "HTTP Server: Error at ioctlsocket(): " << WSAGetLastError() << endl;
	}
	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (sockets[i].recv == EMPTY)
		{
			sockets[i].id = id;
			sockets[i].recv = what;
			sockets[i].send = IDLE;
			sockets[i].len = 0;
			sockets[i].activeStemp = time(0);//reset responding time
			socCount++;
			return true;
		}
	}
	return false;
}

void removeSocket(int index, SocketState* sockets, int& socCount)
{
	sockets[index].recv = EMPTY;
	sockets[index].send = EMPTY;
	sockets[index].activeStemp = 0;
	socCount--;
	cout << "The socket in position: " << index << " removed." << endl;
}

void acceptConnection(int index, SocketState* sockets, int& socCount)
{
	sockets[index].activeStemp = time(0);//reset responding time
	struct sockaddr_in from;		// Address of sending partner
	int fromLen = sizeof(from);

	SOCKET msgSocket = accept(sockets[index].id, (struct sockaddr*)&from, &fromLen);
	if (INVALID_SOCKET == msgSocket)
	{
		cout << "HTTP Server: Error at accept(): " << WSAGetLastError() << endl;
		return;
	}
	cout << "HTTP Server: Client " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port) << " is connected." << endl;

	if (addSocket(msgSocket, RCV, sockets, socCount) == false)
	{
		cout << "\t\tToo many connections, dropped!\n";
		closesocket(sockets[index].id);
	}
	return;
}

void receiveMessage(int index, SocketState* sockets, int& socCount) 
{
	
	SOCKET msgSocket = sockets[index].id;
	int len = sockets[index].len;
	int bytesRecv = recv(msgSocket, &sockets[index].buff[len], sizeof(sockets[index].buff) - len, 0);

	if (SOCKET_ERROR == bytesRecv)
	{
		cout << "HTTP Server: Error at recv(): " << WSAGetLastError() << endl;
		closesocket(msgSocket);
		removeSocket(index, sockets, socCount);
		return;
	}
	if (bytesRecv == 0)
	{
		closesocket(msgSocket);
		removeSocket(index, sockets, socCount);
		return;
	}
	else
	{
		sockets[index].buff[len + bytesRecv] = '\0'; //add the null-terminating to make it a string
		cout << "HTTP Server: Recieved: " << bytesRecv << " bytes of \"" << &sockets[index].buff[len] << "\" message.\n";

		sockets[index].len += bytesRecv;

		if (sockets[index].len > 0)
		{
			updateSubType(index, sockets);
		}
	}

}

void sendMessage(int index, SocketState* sockets)
{
	int bytesSent = 0, buffLen = 0, fileSize = 0, returnCode;
	SOCKET msgSocket = sockets[index].id;
	size_t found;
	char sendBuff[BUFF_MAX_SIZE], readBuff[BUFF_MAX_SIZE], deleteBuff[BUFF_MAX_SIZE];
	string content, returnMsg, address, innerAddress, errorAddress, url, msgLang;
	time_t currTime;
	innerAddress = "C:\\Temp\\HTML_FILES\\";
	errorAddress = "C:\\Temp\\HTML_FILES\\Error\\error.html";
	time(&currTime); // Get current time
	sockets[index].activeStemp = time(0);//reset responding time

	switch (sockets[index].sendSubType)
	{
		case TRACE:
			sendTrace(fileSize, sockets, index, returnMsg, currTime, buffLen, sendBuff);
			break;
		case DELETE_:
			deleteReq(innerAddress, sockets, index, deleteBuff, returnMsg, currTime, fileSize, buffLen, sendBuff);
			break;
		case PUT:
			putRequestHelper(returnCode, index, sockets, returnMsg, currTime, fileSize, buffLen, sendBuff);
			break;
		case POST:
			postReq(returnMsg, currTime, content, sockets, index, buffLen, sendBuff);
			break;
		case HEAD:
			headReq(innerAddress, msgLang, index, sockets, errorAddress, url, returnMsg, fileSize, currTime, buffLen, sendBuff);
			break;
		case GET:
			getReq(innerAddress, msgLang, index, sockets, errorAddress, returnMsg, url, readBuff, content, currTime, fileSize, buffLen, sendBuff);
			break;
		case OPTIONS:
			optionsReq(returnMsg, buffLen, sendBuff);
			break;
		default:
			break;
	}
	bytesSent = send(msgSocket, sendBuff, buffLen, 0);
	//clean buffer: memset put NULL in every spot of the buffer 
	memset(sockets[index].buff, 0, BUFF_MAX_SIZE);
	//reset buffer size to 0
	sockets[index].len = 0;
	if (SOCKET_ERROR == bytesSent)
	{
		cout << "HTTP Server: Error at send(): " << WSAGetLastError() << endl;
		return;
	}

	cout << "HTTP Server: Sent: " << bytesSent << "\\" << strlen(sendBuff) << " bytes of \"" << sendBuff << "\" message.\n";

	sockets[index].send = IDLE;
}

void optionsReq(std::string& returnedMsg, int& buffLen, char  sendBuff[2048])
{
	returnedMsg = "HTTP/1.1 204 No Content\r\nAllow: OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE\r\n";
	returnedMsg += "Content-length: 0\r\n\r\n";
	buffLen = returnedMsg.size();
	strcpy(sendBuff, returnedMsg.c_str());
}

void postReq(std::string& returnedMsg, time_t& currentTime, std::string& content, SocketState* sockets,
	int index, int& buffLen, char  sendBuff[2048])
{
	size_t foundNewLine;
	//might need to change from ori
	returnedMsg = "HTTP/1.1 200 OK \r\nDate:";
	returnedMsg += ctime(&currentTime);
	returnedMsg += "Content-length: 0\r\n\r\n";
	content = (string)sockets[index].buff;
	foundNewLine = content.find("\r\n\r\n");
	cout << "------------------------------------------------------------\n"
		<< "Message received!\n"
		<< "\n------------------------------------------------------------\n"
		<< &content[foundNewLine + 4]
		<< "\n------------------------------------------------------------\n\n";
	buffLen = returnedMsg.size();
	strcpy(sendBuff, returnedMsg.c_str());
}

void putReq(int& returnCode, int index, SocketState* sockets, std::string& returnMsg,
	time_t& currentTime, int fileSize, int& buffLen, char  sendBuff[2048])
{
	char fileName[BUFF_MAX_SIZE];
	returnCode = putRequestHelper(index, fileName, sockets);
	switch (returnCode)
	{
		case 0:
		{
			cout << "PUT " << fileName << "Failed";
			returnMsg = "HTTP/1.1 412 Precondition failed \r\nDate: ";
			break;
		}
		case 200:
		{
			returnMsg = "HTTP/1.1 200 OK \r\nDate: ";
			break;
		}
		case 201:
		{
			returnMsg = "HTTP/1.1 201 Created \r\nDate: ";
			break;
		}
		case 204:
		{
			returnMsg = "HTTP/1.1 204 No Content \r\nDate: ";
			break;
		}
		default:
		{
			returnMsg = "HTTP/1.1 501 Not Implemented \r\nDate: ";
			break;
		}
	}

	createReturnMsg(returnMsg, currentTime, fileSize, buffLen, sendBuff);
}

void getReq(string& innerAddress, string& msgLang, int index, 
	SocketState* sockets, string& errorAddress, string& returnedMsg, 
	string& url, char  readBuff[2048], string& content, time_t& currentTime, 
	int fileSize, int& buffLen, char  sendBuff[2048])
{
	ifstream httpFile;
	string address = innerAddress;
	msgLang = getLanguage(index, sockets);
	if (msgLang == "Error")
	{
		httpFile.open(errorAddress); //Error::No Language matched: there is no supporting language
		returnedMsg = "HTTP/1.1 404 Not Found ";
	}
	else
	{
		languageAppend(address, msgLang, url, sockets, index, httpFile, returnedMsg, true);
	}

	if (httpFile)
	{
		// Read content from file to string and get its length
		while (httpFile.getline(readBuff, BUFF_MAX_SIZE))
			content += readBuff;
	}
	//dadsa(httpFile, returnMsg, currTime, fileSizeInString, content, buffLen, sendBuff);
	closeFileSequence(httpFile, returnedMsg, currentTime, fileSize, buffLen, sendBuff, content);
}

void languageAppend(string& address, string& msgLang, string& url, SocketState* sockets, int index, ifstream& httpFile, string& returnedMsg, bool isGet)
{
	size_t findQuestionMark;
	address.append(msgLang);
	address += "\\";
	url = strtok(sockets[index].buff, " ");
	findQuestionMark = url.find("?");
	if (findQuestionMark != string::npos)//lang Query String exist
	{
		url = url.substr(0, findQuestionMark);
	}
	address.append(url);
	httpFile.open(address);
	if(isGet)
		returnedMsg = "HTTP/1.1 200 OK ";
}

void headReq(std::string& innerAddress, 
	string& msgLang, int index, SocketState* sockets, string& errorAddress, string& url, string& returnMsg, 
	int& fileSize, time_t& currentTime, int& buffLen, char  sendBuff[2048])
{
	ifstream httpFile;
	string address = innerAddress;
	msgLang = getLanguage(index, sockets);
	if (msgLang == "Error")
		httpFile.open(errorAddress); //Error::No Language matched: there is no supporting language
	else
	{
		languageAppend(address, msgLang, url, sockets, index, httpFile, returnMsg, false);
	}
	if (!httpFile)
		returnMsg = "HTTP/1.1 404 Not Found ";
	else
	{
		returnMsg = "HTTP/1.1 200 OK ";
		// Read content from file to string and get its length
		httpFile.seekg(0, ios::end);
		fileSize = httpFile.tellg(); // get length of content in file
	}
	closeFileSequence(httpFile, returnMsg, currentTime, fileSize, buffLen, sendBuff);
}


void closeFileSequence(ifstream& httpFile, string& returnedMsg, time_t& currentTime, 
	int fileSize, int& buffLen, char  sendBuff[2048], string content = "")
{
	httpFile.close();
	returnedMsg += "\r\nContent-type: text/html \r\nDate:";
	createReturnMsg(returnedMsg, currentTime, fileSize, buffLen, sendBuff, content);
}

void createReturnMsg(string& returnMsg, time_t& currentTime, int fileSize, int& buffLen, char  sendBuff[2048], string content = "")
{
	returnMsg += ctime(&currentTime);
	returnMsg += "Content-length: " + to_string(fileSize) + "\r\n\r\n";
	//returnMsg += to_string(fileSize) + "\r\n\r\n";
	//returnMsg += "\r\n\r\n";
	if (content != "")
		returnMsg += content; // Get content
	buffLen = returnMsg.size();
	strcpy(sendBuff, returnMsg.c_str());
}

void deleteReq(string& innerAddress, SocketState* sockets, int index, char  deleteBuff[2048],
	string& returnMsg, time_t& currentTime, int fileSize, int& buffLen, char  sendBuff[2048])
{
	string address = innerAddress;
	address += strtok(sockets[index].buff, " ");//get the file name
	strcpy(deleteBuff, address.c_str());//move address to char* variable
	if (remove(deleteBuff) != 0)
	{
		returnMsg = "HTTP/1.1 204 No Content \r\nDate: "; // We treat 204 code as a case where delete wasn't successful
	}
	else
	{
		returnMsg = "HTTP/1.1 200 OK \r\nDate: "; // File deleted succesfully
	}

	createReturnMsg(returnMsg, currentTime, fileSize, buffLen, sendBuff);
}

void sendTrace(int& fileSize, SocketState* sockets, int index, std::string& returnMsg, time_t& currentTime, int& buffLen, char  sendBuff[2048])
{
	fileSize = strlen("TRACE ") + strlen(sockets[index].buff);
	returnMsg = "HTTP/1.1 200 OK \r\nContent-type: message/http\r\nDate: ";
	returnMsg += ctime(&currentTime);
	returnMsg += "Content-length: " + to_string(fileSize) + "\r\n\r\nTRACE ";
	returnMsg += sockets[index].buff;
	buffLen = returnMsg.size();
	strcpy(sendBuff, returnMsg.c_str());
}


// A utility to check for the inactive sockets (if the response time is greater then 2 minutes) and remove them
void updateSocStemp(SocketState* sockets, int& socCount)
{
	time_t currentTime;
	for (int i = 1; i < MAX_SOCKETS; i++)
	{
		currentTime = time(0);
		if ((currentTime - sockets[i].activeStemp > 120) && (sockets[i].activeStemp != 0))
		{
			removeSocket(i, sockets, socCount);
		}
	}
}

void updateSubType(int index, SocketState* sockets)
{
	int toSub;
	string buff, firstWord;
	static map<string, HttpReq> request = { {"TRACE",HttpReq::TRACE},{"DELETE",HttpReq::DELETE_},
												{"PUT",HttpReq::PUT},{"POST",HttpReq::POST},
												{"HEAD",HttpReq::HEAD},{"GET",HttpReq::GET},{"OPTIONS",HttpReq::OPTIONS} };
	//import buffer to string
	buff = (string)sockets[index].buff;
	//get the first word in buffer
	firstWord = buff.substr(0, buff.find(" "));
	//update request in buffer
	sockets[index].send = SND;
	sockets[index].sendSubType = request[firstWord];
	//delete the method from buffer
	toSub = firstWord.length() + 2;//1 for space and 1 for '/'
	sockets[index].len -= toSub;
	memcpy(sockets[index].buff, &sockets[index].buff[toSub], sockets[index].len);
	sockets[index].buff[sockets[index].len] = '\0';
}

string getLanguage(int index, SocketState* sockets)
{
	static map<string, string> request = { {"en","English"},{"en-AU","English"},{"en-BZ","English"},{"en-CA","English"},{"en-CB","English"},{"en-GB","English"},{"en-IE","English"},
										{"en-JM","English"},{"en-NZ","English"},{"en-PH","English"},{"en-TT","English"},{"en-US","English"},{"en-ZA","English"},{"en-ZW","English"},
										{"fr","French"},{"fr-BE","French"},{"fr-CA","French"},{"fr-CH","French"},{"fr-FR","French"},{"fr-LU","French"},{"fr-MC","French"},
										{"he","Hebrew"},{"he-IL","Hebrew"} };
	string buff, lookWord, langWord, bufferAtLangWord;
	size_t found;
	//import buffer to string
	buff = (string)sockets[index].buff;
	lookWord = "?lang=";
	found = buff.find(lookWord);
	if (found == std::string::npos)//lang Query String doesn't exist
		return "English";//defult language
	//else get the Language word in buffer
	bufferAtLangWord = &buff[found + lookWord.length()];
	langWord = bufferAtLangWord.substr(0, bufferAtLangWord.find(" "));
	auto search = request.find(langWord);
	if (search != request.end()) {
		return request[langWord];
	}
	else {
		return "Error";//Error::No Language matched: there is no supporting language
	}
}

int putRequestHelper(int index, char* filename, SocketState* sockets)
{
	string content, buff = (string)sockets[index].buff, address = "C:\\Temp\\HTML_FILES\\";
	int buffLen = 0;
	int retCode = 200; // 'OK' code
	size_t found;
	filename = strtok(sockets[index].buff, " ");
	address += filename;

	fstream outPutFile;
	outPutFile.open(address);

	if (!outPutFile.good())
	{
		outPutFile.open(address, ios::out);
		retCode = 201; // New file created
	}

	if (!outPutFile.good())
	{
		cout << "HTTP Server: Error writing file to local storage: " << WSAGetLastError() << endl;
		return 0; // Error opening file
	}
	found = buff.find("\r\n\r\n");
	content = &buff[found + 4];
	if (content.length() == 0)
		retCode = 204; // No content
	else
		outPutFile << content;

	outPutFile.close();
	return retCode;
}