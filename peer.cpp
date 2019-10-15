#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#define MAXSLEEP 128
using namespace std;
int establishConnection(string,string);
void *clientHandler(void *);
int trackerCommunication(int,struct serverData *);
int receiveData(int);
void *serverHandler(void *);
int initServer(void *);
void sendData(int passivefd);
void *getChunkInfo(void *);
void *downloadFile(void *);
#define QLENGTH 10
map<string,vector<string>> filesPresent;
struct serverData
{
	char* ip;
	char* port;
};
int main(int argc,char **argv)
{
	ifstream input("filesInfo.txt",ifstream::in);
	if(!input)
		cout<<"error while opening file"<<endl;
	string str;
	while(getline(input,str))
	{
		stringstream instream(str);
		string key;
		instream>>key;
		string temp;
		vector<string> vectemp;
		while(instream>>temp)
			vectemp.push_back(temp);
		filesPresent[key] = vectemp;
	}
	pthread_t serverThread;
	struct serverData *arg = new serverData;
	arg->ip = argv[1];
	arg->port = argv[2];
	int serverThreadStatus = pthread_create(&serverThread,NULL,serverHandler,(void *)arg);
	pthread_t clientThread;
	struct serverData *arg1 = new serverData;
	arg1->ip = argv[1];
	arg1->port = argv[2];
	int clientThreadStatus = pthread_create(&clientThread,NULL,clientHandler,(void *)arg1);
	void *threadReturn;
	int err = pthread_join(clientThread, &threadReturn);
	if (err != 0)
	{
		cout<<"can't join with clientThread"<<err<<endl;
	}
	cout<<"clientThread has joined"<<endl;
	err = pthread_join(serverThread, &threadReturn);
	if (err != 0)
	{
		cout<<"can't join with serverThread"<<err<<endl;
	}
	cout<<"serverThread has joined"<<endl;
	//save data to fileInfo.txt
	return 0;
}

void *clientHandler(void *arg)
{
	string ip;
	string port;
	int connectedfd  = establishConnection("127.0.0.1","3000");
	struct serverData *argtemp = (struct serverData *)arg;
	int sendStatus  = trackerCommunication(connectedfd,argtemp);
	//int receiveStatus = receiveData(connectedfd);
	pthread_exit((void *) 1);
}
int establishConnection(string ip,string port)
{
	char buf1[10],buf2[30];
	int numsec, socketfd;
	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	strcpy(buf1,port.c_str());
	serverAddress.sin_port=htons(stoi(port));
	strcpy(buf2,ip.c_str());
	serverAddress.sin_addr.s_addr=inet_addr(buf2);
	for (numsec = 1; numsec <= MAXSLEEP; numsec <<= 1) 
	{
		if ((socketfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
			return(-1);
		if (connect(socketfd,(struct sockaddr*)&serverAddress ,sizeof(serverAddress)) == 0)
		{
			return(socketfd);
		}
		close(socketfd);
		if (numsec <= MAXSLEEP/2)
			sleep(numsec);
	}
	return(-1);
}
int trackerCommunication(int connectedfd,struct serverData *argtemp)
{
	cout<<"established connection with tracker"<<endl;
	pthread_t tid[1000];
	int threadsSpanned = 0;
	while(1)
	{
		string str;
		getline(cin,str);	
		char Buffer[4096];
		istringstream in(str);
		string temp;
		in>>temp;
		if(temp == "login")
		{
			string strtemp1(argtemp->ip);
			string strtemp2(argtemp->port);
			str = str + " "+strtemp1+" "+strtemp2;
			strcpy(Buffer,str.c_str());
			send(connectedfd,Buffer,str.size()+1,0);
			//cout<<"sent data"<<endl;
			recv(connectedfd,Buffer,4096,0);
			string str1(Buffer);
			cout<<str1<<endl;
			continue;
		}
		if(temp == "upload_file")
		{
			streampos begin,end;
			char buf[1024];
			in>>temp;
			strcpy(buf,temp.c_str());
			ifstream myfile(buf, ios::binary);
			begin = myfile.tellg();
			myfile.seekg (0, ios::end);
			end = myfile.tellg();
			streampos fileSize =end-begin; 
			str = str + " "+to_string(fileSize);
		}
		if(temp == "exit")
		{
			close(connectedfd);
			cout<<"terminated connection with tracker"<<endl;
			cout<<"Waiting for all threads to join......"<<endl;
			for(int i = 0;i < threadsSpanned;i++)
				pthread_join(tid[i],NULL);
			break;
		}
		if(temp == "download_file")
		{
			strcpy(Buffer,str.c_str());
			send(connectedfd,Buffer,str.size()+1,0);
			//cout<<"sent data"<<endl;
			recv(connectedfd,Buffer,4096,0);
			string str1(Buffer);
			cout<<str1<<endl;
			char* arg = Buffer;
			pthread_create(&tid[threadsSpanned],NULL,downloadFile,arg);
			threadsSpanned++;
			continue;
		}
		strcpy(Buffer,str.c_str());
		send(connectedfd,Buffer,str.size()+1,0);
		//cout<<"sent data"<<endl;
		recv(connectedfd,Buffer,4096,0);
		string str1(Buffer);
		cout<<str1<<endl;
	}
	return 0;
}
struct argStruct
{
	int fileSize,row;
	string ip,filePath,port;
	vector<vector<int> > chunkVector;
};
void *downloadFile(void *arg)
{
	pthread_t downloadtid[1000];
	int threadCount = 0;
	char *tempArg = (char *) arg;
	string clientsInfo(tempArg);
	vector<string> clientsHavingFile;
	stringstream in(clientsInfo);
	string temp;
	while(in>>temp)
		clientsHavingFile.push_back(temp);
	for(unsigned int i = 0;i < clientsHavingFile.size();i++)
	{
		struct argStruct *arg1 = new struct argStruct;
		std::size_t prevColon = 0;
		std::size_t findColon = clientsHavingFile[i].find_first_of(":");
		arg1->filePath = clientsHavingFile[i].substr(prevColon,findColon - prevColon);
		prevColon = findColon+1;
		findColon = clientsHavingFile[i].find_first_of(":",findColon+1);
		arg1->ip  = clientsHavingFile[i].substr(prevColon,findColon - prevColon);
		prevColon = findColon+1;
		findColon = clientsHavingFile[i].find_first_of(":",findColon+1);
		arg1->port = clientsHavingFile[i].substr(prevColon,findColon - prevColon);
		prevColon = findColon+1;
		findColon = clientsHavingFile[i].find_first_of(":",findColon+1);
		arg1->fileSize = stoi(clientsHavingFile[i].substr(prevColon));
		arg1->row=i;
		//cout<<arg1->filePath<<" "<<arg1->ip<<" "<<arg1->port<<" "<<arg1->fileSize<<endl;
		pthread_create(&downloadtid[threadCount],NULL,getChunkInfo,(void*)arg1);
		threadCount++;
	}
	for(int i = 0;i < threadCount;i++)
		pthread_join(downloadtid[i],NULL);
	//getData
	return (void*)0;
}
void* getChunkInfo(void *arg)
{
	char Buffer[4096];
	struct argStruct *tempArg  = ((struct argStruct *)arg);
	int connectedfd  = establishConnection(tempArg->ip,tempArg->port);
	string str  = " getChunkInfo" + tempArg->filePath;
	strcpy(Buffer,str.c_str());
	send(connectedfd,Buffer,str.size()+1,0);
	recv(connectedfd,Buffer,4096,0);
	string str1(Buffer);
	stringstream in;
	string temp;
	while(in >> temp)
	{
		tempArg->chunkVector[tempArg->row].push_back(stoi(temp));
	}
	return (void*) 0;
}
int receiveData(int connectedfd)
{
	FILE *fp = fopen("new.mp3", "wb" );
	if(fp == NULL)
		cout<<"Error with creating files"<<endl;
	char Buffer [4096] ; 
	int file_size;
	recv(connectedfd, &file_size, sizeof(file_size), 0);
	int n;
	while ( (n = recv(connectedfd , Buffer ,4096, 0) ) > 0  &&  file_size > 0)
	{
		fwrite (Buffer , sizeof (char), n, fp);
		memset ( Buffer , '\0', 4096);
		file_size = file_size - n;
	}
	close(connectedfd);
	fclose(fp);
	return 0;
}
void *serverHandler(void *arg)
{
	int passivefd  = initServer((void*) arg);
	sendData(passivefd);
	pthread_exit((void *) 2);
}
int initServer(void *arg)
{
	struct serverData *argtemp = (struct serverData *)arg;
	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port=htons(atoi(argtemp->port));
	serverAddress.sin_addr.s_addr=inet_addr(argtemp->ip);
	int socketfd;
	if ((socketfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
		return(-1);
	if ((bind(socketfd,(struct sockaddr *)&serverAddress,sizeof(serverAddress))) < 0)
	{
		cout<<"Binding error1"<<errno<<endl;
		return -1;
	}
	if (listen(socketfd, QLENGTH) < 0)
	{
		cout<<"Error while listening.."<<endl;
		return -1;
	}
	cout<<"started server"<<endl;
	return(socketfd);
}

void sendData(int passivefd)
{

	int activefd;
	if((activefd = accept(passivefd,NULL,NULL))< 0)
	{
		cout<<"Error while Accepting"<<errno<<endl;
	}
	FILE *fp = fopen("1.mp3","rb");
	if(fp == NULL)
		cout<<"Error while opening file"<<endl;
	fseek(fp,0,SEEK_END);
  	int size = ftell(fp );
  	rewind( fp );
	send ( activefd , &size, sizeof(size), 0);

		char Buffer[1024] ; 
		int n;
		while ( ( n = fread(Buffer,sizeof(char),1024,fp)) > 0  && size > 0 )
		{
			send (activefd , Buffer, n, 0 );
			memset ( Buffer , '\0', 1024);
			size = size - n ;
		}
		fclose (fp);
		close(activefd);
}