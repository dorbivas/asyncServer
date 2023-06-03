#include "ServerImp.h"


bool addSocket(SOCKET id, Status what, SocketState* sockets, int& socCount)
{
	// Set the socket to be in non-blocking.
	unsigned long flag = 1;
	if (ioctlsocket(id, FIONBIO, &flag) != 0)
	{
		cout << "HTTP Server Error at ioctlsocket " << WSAGetLastError() << endl;
	}
	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (sockets[i].recv == EMPTY)
		{
			sockets[i].id = id;
			sockets[i].send = IDLE;
			sockets[i].recv = what;
			sockets[i].lan = 0;
			sockets[i].activeStemp = time(0);//reset responding time
			socCount++;
			return (true);
		}
	}
	return (false);
}
// let
void removeSoc(int index, SocketState* sockets, int& socCount)
{
	sockets[index].recv = EMPTY;
	sockets[index].send = EMPTY;
	sockets[index].activeStemp = 0;
	socCount--;
	cout << "The number: " << index << "was removed" << endl;
}

void acceptConnection(int index, SocketState* sockets, int& socCount)
{
	SOCKET id = sockets[index].id;
	sockets[index].activeStemp = time(0);
	struct sockaddr_in from;
	int length = sizeof(from);

	SOCKET msgSoc = accept(id, (struct sockaddr*)&from, &length);
	if (INVALID_SOCKET == msgSoc)
	{
		cout << "HTTP Server: Error at accept(): " << WSAGetLastError() << endl;
		return;
	}
	cout << "HTTP Server: Client " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port) << " is connected." << endl;

	if (addSocket(msgSoc, RCV, sockets, socCount) == false)
	{
		cout << "\t\tToo many connections, dropped!\n";
		closesocket(id);
	}
	return;
}

void receiveMssge(int index, SocketState* sockets, int& socCount) 
{
	SOCKET msgSoc = sockets[index].id;
	int lan = sockets[index].lan;
	int bytesRecv = recv(msgSoc, &sockets[index].buff[lan], sizeof(sockets[index].buff) - lan, 0);

	if (SOCKET_ERROR == bytesRecv)
	{
		cout << "HTTP Server: Error at recv(): " << WSAGetLastError() << endl;
		closesocket(msgSoc);
		removeSoc(index, sockets, socCount);
		return;
	}
	if (bytesRecv == 0)
	{
		closesocket(msgSoc);
		removeSoc(index, sockets, socCount);
		return;
	}
	else
	{
		sockets[index].buff[lan + bytesRecv] = '\0'; 
		cout << "HTTP Server Recieved " << bytesRecv << " bytes of \"" << &sockets[index].buff[lan] << "\" message.\n";

		sockets[index].lan += bytesRecv;

		if (sockets[index].lan > 0)
		{
			updateSubType(index, sockets);
		}
	}

}

void sendMessage(int index, SocketState* sockets)
{
	int bytesSent = 0, buffLen = 0, fileSize = 0, returnCode;
	size_t found;
	char sendBuff[BUFF_MAX_SIZE], readBuff[BUFF_MAX_SIZE], deleteBuff[BUFF_MAX_SIZE];
	ifstream httpFile;
	string content, retMsg, fileSizeInString, address, innerAddress = "C:\\temp\\HTML_FILES\\",
		errorAddress = "C:\\temp\\HTML_FILES\\Error\\error.html", url, msgLang;
	time_t currentTime;
	time(&currentTime); 
	string newAddress;
	SOCKET msgSocket = sockets[index].id;
	sockets[index].activeStemp = time(0);// handle timeout
	
	switch (sockets[index].sendSubType){
	case TRACE:
		sendTrace(fileSize, sockets, index, retMsg, currentTime, fileSizeInString, buffLen, sendBuff);
		break;
	case DELETE_:
		newAddress = innerAddress + strtok(sockets[index].buff, " ");
		address = newAddress.substr(0, 37);

		strcpy(deleteBuff, address.c_str());
		if (remove(deleteBuff) != 0)
		{
			retMsg = "HTTP/1.1 204 No Content \r\nDate: ";
		}
		else{
			retMsg = "HTTP/1.1 200 OK \r\nDate: "; 
		}
		retMsg += ctime(&currentTime);
		retMsg += "Content-length: ";
		fileSizeInString = to_string(fileSize);
		retMsg += fileSizeInString;
		retMsg += "\r\n\r\n";
		buffLen = retMsg.size();
		strcpy(sendBuff, retMsg.c_str());
		break;
	case PUT:
		char fileName[BUFF_MAX_SIZE];
		returnCode = putReq(index, fileName, sockets);
		switch (returnCode)
		{
		case 0:{
			cout << "PUT " << fileName << "Failed";
			retMsg = "HTTP/1.1 412 Precondition fail\r\nDate";
			break;
		}
		case 200:{
			retMsg = "HTTP/1.1 200 OK \r\nDate: ";
			break;
		}
		case 201:{
			retMsg = "HTTP/1.1 201 Created \r\nDate: ";
			break;
		}
		case 204:{
			retMsg = "HTTP/1.1 204 No Content \r\nDate: ";
			break;
		}
		default:{
			retMsg = "HTTP/1.1 501 Not Implemented \r\nDate: ";
			break;
		}}

		retMsg += ctime(&currentTime);
		retMsg += "Content len: ";
		fileSizeInString = to_string(fileSize);
		retMsg += fileSizeInString;
		retMsg += "\r\n\r\n";
		buffLen = retMsg.size();
		strcpy(sendBuff, retMsg.c_str());
		break;
	case POST:
		retMsg = "HTTP/1.1 200 OK \r\nDate:";
		retMsg += ctime(&currentTime);
		retMsg += "Content len: 0\r\n\r\n";
		content = (string)sockets[index].buff;
		found = content.find("\r\n\r\n");
		cout << "-----------------\n"
			<< "Message received\n"
			<< &content[found + 4]
			<< "\n-------------\n";
		buffLen = retMsg.size();
		strcpy(sendBuff, retMsg.c_str());
		break;
	case HEAD:
		address = innerAddress;
		msgLang = getLen(index, sockets);
		if (msgLang == "Error")
			httpFile.open(errorAddress); 
		else
		{
			address.append(msgLang);
			address += "\\";
	
			string newUrl = strtok(sockets[index].buff, " ");
			std::string url = newUrl.substr(0, 10);

			found = url.find("?");
			if (found != std::string::npos)
			{
				url = url.substr(0, found);
			}
			address.append(url);
			httpFile.open(address);
		}
		if (!httpFile)
			retMsg = "HTTP/1.1 404 Not Found";
		else
		{
			retMsg = "HTTP/1.1 200 OK ";

			httpFile.seekg(0, ios::end);
			fileSize = httpFile.tellg(); 
		}
		displayContent(httpFile, retMsg, currentTime, fileSizeInString, fileSize, buffLen, sendBuff);
		break;
	case GET:
		address = innerAddress;
		msgLang = getLen(index, sockets);
		if (msgLang == "Error")
		{
			httpFile.open(errorAddress); 
			retMsg = "HTTP/1.1 404 Not Found";
		}
		else
		{
			address.append(msgLang);
			address += "\\";
			url = strtok(sockets[index].buff, " ");
			found = url.find("?");
			if (found != std::string::npos)
			{
				url = url.substr(0, found);
			}
			address.append(url);
			httpFile.open(address);
			retMsg = "HTTP/1.1 200 OK ";
		}
		if (httpFile)
		{
			while (httpFile.getline(readBuff, BUFF_MAX_SIZE))
				content += readBuff;
		}
		httpFile.close();
		retMsg += "\r\nContent type: text/html";
		retMsg += "\r\nDate: ";
		retMsg += ctime(&currentTime);
		retMsg += "Content len: ";
		fileSizeInString = to_string(content.length());
		retMsg += fileSizeInString;
		retMsg += "\r\n\r\n";
		retMsg += content; 
		buffLen = retMsg.size();
		strcpy(sendBuff, retMsg.c_str());
		break;
		
	case OPTIONS:
		retMsg = "HTTP/1.1 204 No Content\r\nAllow:TRACE, OPTIONS, GET, HEAD, POST, PUT, DELETE \r\n";
		retMsg += "Content-len: 0\r\n\r\n";
		buffLen = retMsg.size();
		strcpy(sendBuff, retMsg.c_str());
		break;
	default:
		break;
	}
	bytesSent = send(msgSocket, sendBuff, buffLen, 0);

	memset(sockets[index].buff, 0, BUFF_MAX_SIZE);
	sockets[index].lan = 0;
	if (SOCKET_ERROR == bytesSent)
	{
		cout << "HTTP Server Error at sund " << WSAGetLastError() << endl;
		return;
	}

	cout << "HTTP Server Sent " << bytesSent << "\\" << strlen(sendBuff) << " bytes of \"" << sendBuff << "\" message\n";

	sockets[index].send = IDLE;
}

void displayContent(std::ifstream& httpFile, std::string& retMsg, time_t& currentTime, std::string& fileSizeInString, int fileSize, int& buffLen, char  sendBuff[2048])
{
	httpFile.close();
	retMsg += "\r\nContent type: text/ html";
	retMsg += "\r\nDate: ";
	retMsg += ctime(&currentTime);
	
	retMsg += "Content len: ";
	fileSizeInString = to_string(fileSize);
	retMsg += fileSizeInString;
	
	retMsg += "\r\n\r\n";
	buffLen = retMsg.size();
	strcpy(sendBuff, retMsg.c_str());
}

void sendTrace(int& fSize, SocketState* sockets, int index, std::string& retMsg, time_t& currentTime, std::string& fileSizeInString, int& buffLen, char  sendBuff[2048])
{
	fSize = strlen("TRACE ");
	fSize += strlen(sockets[index].buff);
	
	retMsg = "HTTP/1.1 200 OK \r\nContent type: message/http\r\nDate: ";
	retMsg += ctime(&currentTime);
	retMsg += "Content len: ";
	fileSizeInString = to_string(fSize);
	retMsg += fileSizeInString;
	
	retMsg += "\r\n\r\n";
	retMsg += "TRACE ";
	retMsg += sockets[index].buff;
	buffLen = retMsg.size();
	strcpy(sendBuff, retMsg.c_str());
}



void updateSocStemp(SocketState* sockets, int& socCount)
{
	time_t currentTime;
	for (int i = 1; i < MAX_SOCKETS; i++)
	{
		currentTime = time(0);
		if ((currentTime - sockets[i].activeStemp > 120) && (sockets[i].activeStemp != 0))
		{
			removeSoc(i, sockets, socCount);
		}
	}
}

void updateSubType(int index, SocketState* sockets)
{
	int toSub;
	string buff, first;
	static map<string, HttpReq> request = { {"TRACE",HttpReq::TRACE},{"DELETE",HttpReq::DELETE_},
												{"PUT",HttpReq::PUT},{"POST",HttpReq::POST},
												{"HEAD",HttpReq::HEAD},{"GET",HttpReq::GET},{"OPTIONS",HttpReq::OPTIONS} };

	
	buff = (string)sockets[index].buff;
	first = buff.substr(0, buff.find(" "));

	sockets[index].send = SND;
	sockets[index].sendSubType = request[first];

	toSub = first.length() + 2;
	sockets[index].lan -= toSub;
	
	memcpy(sockets[index].buff, &sockets[index].buff[toSub], sockets[index].lan);
	sockets[index].buff[sockets[index].lan] = '\0';
}

string getLen(int index, SocketState* sockets)
{
	static map<string, string> request = {{"en","English"},{"fr","French"},{"he","Hebrew"}};
	string buff, word, lang, buffWord;
	size_t sizeW;
	
	buff = (string)sockets[index].buff;
	word = "?lang=";
	sizeW = buff.find(word);
	
	if (sizeW == std::string::npos)
		return "English";

	buffWord = &buff[sizeW + word.length()];
	lang = buffWord.substr(0, buffWord.find(" "));
	auto search = request.find(lang);

	
	if (search != request.end()) {
		return request[lang];
	}
	
	else {
		return "Error";
	}
}

int putReq(int index, char* filename, SocketState* sockets)
{
	string content, buff = (string)sockets[index].buff, address = "C:\\temp\\HTML_FILES\\";
	int buffLen = 0;
	int retCode = 200; 
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
		cout << "HTTP Server Error writing file to storage " << WSAGetLastError() << endl;
		return 0; // Error opening file
	}
	found = buff.find("\r\n\r\n");
	content = &buff[found + 4];
	if (content.length() == 0)
		retCode = 204; 
	else
		outPutFile << content;

	outPutFile.close();
	return retCode;
}