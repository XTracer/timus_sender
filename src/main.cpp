#include <WS2tcpip.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <regex>

using namespace std;

#include "main.h"

#pragma comment(lib, "Ws2_32.lib")



//Socket
WSADATA wsaData;
struct addrinfo *result = NULL;
struct addrinfo *ptr = NULL;
struct addrinfo hints;
SOCKET ConnectSocket;





string HttpGet( string SendBuffer )
{
	getaddrinfo("acm.timus.ru", "80", &hints, &result);

	ptr = result;
	ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
	
	connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
	freeaddrinfo(result);


	char RecvBuffer[512];

	send( ConnectSocket, SendBuffer.c_str(), (int)SendBuffer.length(), 0 );
	shutdown( ConnectSocket, SD_SEND);

	string Response;

	int iResult;
	do {
		iResult = recv(ConnectSocket, RecvBuffer, 512, 0);

		if(iResult > 0)
			Response.append(RecvBuffer);
		else if(iResult < 0)
			printf( "recv failed: %d\n", WSAGetLastError() );
		
	} while(iResult > 0);
	closesocket(ConnectSocket);

	return Response;
}


void InitSocket()
{
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	WSAStartup(MAKEWORD(2,2), &wsaData);

}

void UpdateStatus( string AuthorID, string SubmitID, string Cookie )
{
	Sleep(2000);
	printf("Getting status...\n");
	string SendBuffer = "GET /status.aspx?locale=en HTTP/1.1\r\nHost: acm.timus.ru\r\nCookie: ASP.NET_SessionId=";
	SendBuffer.append(Cookie);
	SendBuffer.append("; AuthorID=");
	SendBuffer.append(AuthorID);
	SendBuffer.append("\r\nConnection: Keep-Alive\r\n\r\n");

	string Response = HttpGet( SendBuffer );
	ParseStatus(AuthorID, SubmitID, Cookie, Response);
}

void GetCompilitionError( string AuthorID, string SubmitID, string Cookie )
{
	string SendBuffer;
	SendBuffer.append("GET /ce.aspx?id=");
	SendBuffer.append(SubmitID);
	SendBuffer.append(" HTTP/1.1\r\nHost: acm.timus.ru\r\nCookie: ASP.NET_SessionId=");
	SendBuffer.append(Cookie);
	SendBuffer.append("; AuthorID=");
	SendBuffer.append(AuthorID);
	SendBuffer.append("\r\nConnection: Keep-Alive\r\n\r\n");

	string Response = HttpGet( SendBuffer );

	std::cmatch Matches;
	std::regex RegExp;

	int Pos = Response.find("\r\n\r\n");
	string Error = Response.substr(Pos + 4);
	printf(Error.c_str());
}


void ParseStatus( string AuthorID, string SubmitID, string Cookie, string Response )
{
	//Get my row
	std::cmatch Matches;
    std::regex RegExp;

	int iSubmitID = atoi(SubmitID.c_str());

	ostringstream srx;
	srx <<("<TD class=\"id\">");
	srx <<(iSubmitID);
	srx <<("(.+)<TD class=\"id\">");
	srx <<(iSubmitID-1);

	RegExp = srx.str();

    std::regex_search(Response.c_str(), Matches, RegExp);
	string Row = Matches[1];

	//Compiling
	if ( Row.find("verdict_wt" ) != -1)
	{
		printf( "Status: Compiling...\n" );
		return UpdateStatus( AuthorID, SubmitID, Cookie );
	}

	//Accepted
	else if ( Row.find("verdict_ac" ) != -1)
	{
		std::cmatch Matches2;
		std::regex RegExp;

		RegExp = "<TD class=\"test\">.*</TD><TD class=\"runtime\">(.*)</TD><TD class=\"memory\">(.*)</TD></TR>";
		std::regex_search(Row.c_str(), Matches2, RegExp);
		string RunTime = Matches2[1];
		string Memory = Matches2[2];

		printf( "Status: Accepted\n" );
		printf( "------------------\n" );
		printf( "Run time: %s s\n", RunTime.c_str() );
		printf( "Memory: %s\n", Memory.c_str() );

		return;
	}	

	//Rejected
	else if ( Row.find("verdict_rj" ) != -1)
	{
		cout<< "Status: Rejected\n";
		printf( "------------------\n" );

		if ( Row.find("Compilation error" ) != -1)
		{
			printf( "Compilation error:\n" );
			GetCompilitionError( AuthorID, SubmitID, Cookie );
		}
		else
		{
			std::cmatch Matches2;
			std::regex RegExp;

			RegExp = "class=\"verdict_rj\">(.*)</TD><TD class=\"test\">(.*)</TD><TD class=\"runtime\">(.*)</TD><TD class=\"memory\">(.*)</TD></TR>";
			std::regex_search(Row.c_str(), Matches2, RegExp);

			string RejectedMsg = Matches2[1];
			string TestError = Matches2[2];
			string RunTime = Matches2[3];
			string Memory = Matches2[4];
			
			printf( "%s\n", RejectedMsg.c_str() );
			printf( "Test error: %s\n", TestError.c_str() );
			printf( "Run time: %s s\n", RunTime.c_str() );
			printf( "Memory: %s\n", Memory.c_str() );
		}
	}
	else
	{
		printf("[ParseStatus] Unknown error!");
	}
}
	
	
	


void Send( string Request )
{
	printf("Sending...\n");

	ostringstream SendBuffer;
	SendBuffer << "POST /submit.aspx?space=1 HTTP/1.1\r\n";
	SendBuffer << "Host: acm.timus.ru\r\n";
	SendBuffer << "Connection: Keep-Alive\r\n";
	SendBuffer << "Content-Length: " << strlen(Request.c_str());
	SendBuffer << "\r\n";
	SendBuffer << "Content-Type: multipart/form-data; boundary=----------Q1W2E3R4T5Y6U7I8O9P0Q1\r\n";
	SendBuffer << Request;
	SendBuffer << "\r\n";

	string Response = HttpGet( SendBuffer.str() );
	
	
    std::regex RegExp;

	std::cmatch Matches;
	RegExp = "SubmitID: ([0-9]+)\r";
    std::regex_search(Response.c_str(), Matches, RegExp);
	string SubmitID = Matches[1];	
	
	if ( !SubmitID.empty() )
	{
		printf( "Ok, sent!\nSubmitID: %s\n", SubmitID.c_str() );

		std::cmatch Matches2;
		RegExp = "AuthorID=(.*); expires";
		std::regex_search(Response.c_str(), Matches2, RegExp);
		string AuthorID = Matches2[1];

		std::cmatch Matches3;
		RegExp = "ASP.NET_SessionId=(.+); path";
		std::regex_search(Response.c_str(), Matches3, RegExp);
		string Cookie = Matches3[1];

		UpdateStatus( AuthorID, SubmitID, Cookie );
	}
	else
	{	
		std::cmatch Matches2;
		RegExp = "color:Red;\">(.*)</TD></TR><TR><TD ALIGN=\"LEFT\">JUDGE_ID";
		std::regex_search(Response.c_str(), Matches2, RegExp);
		string Error = Matches2[1];

		if ( !Error.empty() )
			printf("[Send error] %s!\n",Error.c_str());
		else
			printf("[Send error] Unknown error!\n");
	}
}


string FormatRequest( string TimusID, string ProblemNum, string Language, string Code )
{
	ostringstream Request;

	const char* splitter = "\r\n------------Q1W2E3R4T5Y6U7I8O9P0Q1\r\n";
	Request << splitter;

	Request << "Content-Disposition: form-data; name=\"Action\"\r\n\r\nsubmit";
	Request << splitter;

	Request << "Content-Disposition: form-data; name=\"SpaceID\"\r\n\r\n1";
	Request << splitter;

	Request << "Content-Disposition: form-data; name=\"JudgeID\"\r\n\r\n" << TimusID;
	Request << splitter;

	Request << "Content-Disposition: form-data; name=\"Language\"\r\n\r\n" << Language;
	Request << splitter;

	Request << "Content-Disposition: form-data; name=\"ProblemNum\"\r\n\r\n" << ProblemNum;
	Request << splitter;

	Request << "Content-Disposition: form-data; name=\"Source\"\r\n\r\n" << Code;
	Request << "\r\n------------Q1W2E3R4T5Y6U7I8O9P0Q1--";

	return Request.str();
}


void Pause()
{
	cout << "Press ENTER to exit";
	cin.ignore(cin.rdbuf()->in_avail()+1);
}

int main(int argc, char *argv[])
{
	if ( argc < 2 )
	{
		printf("Example usage: timus_sender.exe 1050.cpp\n");
		Pause();
		return 0;
	}

	//Code file
	string CodeFilename = argv[1];
	string Code;

	ifstream ReadFile;
	ReadFile.open( CodeFilename );

	string Line;

	if ( !ReadFile.is_open() )
	{
		ReadFile.close();
		printf("Can't open file!\n");
		Pause();
		return 0;
	}
	
	while ( getline( ReadFile, Line ) ) 
	{
		Code.append(Line);
		Code.append("\r\n");
	}

	ReadFile.close();


	//Config
	string ConfigFilename = "config.txt";

	ifstream ConfigFile;
	ConfigFile.open(ConfigFilename);

	if (!ConfigFile.is_open())
	{
		ReadFile.close();
		printf("Can't open config file!\n");
		Pause();
		return 0;
	}

	string TimusID;
	string Language;

	getline( ConfigFile, TimusID );
	getline( ConfigFile, Language );
	ConfigFile.close();



	//Get problem num from filename
	string ProblemNum = CodeFilename;

	const size_t last_slash_idx = ProblemNum.find_last_of("\\/");
	if (std::string::npos != last_slash_idx)
		ProblemNum.erase(0, last_slash_idx + 1);

	const size_t period_idx = ProblemNum.rfind('.');
	if (std::string::npos != period_idx)
		ProblemNum.erase(period_idx);

		
	InitSocket();

	string Request = FormatRequest( TimusID, ProblemNum, Language, Code );
	Send( Request );
	
	WSACleanup();

	Pause();

	return 0;
}