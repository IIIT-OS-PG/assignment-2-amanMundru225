#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <iostream>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>
#include <pthread.h>
using namespace std;
void* initServer(void *arg);
void *objectCreation(void *arg);
void sendData(int activefd,string str);
#define QLENGTH 10
struct ClientAuthentication
{
	string clientId;
	string password;
	bool loggedInFlag;
};
struct ClientAddress
{
	string client_id;
	string clientIP;
	int clientPort;
};
//groups which client belong
struct ClientGroups
{
	string client_id;
	vector<string> clientGroupList;
};
//groups in clients
struct GroupMembers
{
	string groupId;
	string groupOwner;
	vector<string> groupsClientsList;
	vector<string> pendingRequestsList;
};
struct FileInfo
{
	int  fileSize;
	string SHA1;
	vector<pair<string,string>> clientsHavingFile;
};
struct FilesInGroup
{
	string groupId;
	vector<pair<string,struct FileInfo*>> filesVector;
};
class Tracker
{
	public:
	map<string,struct FilesInGroup*> filesInGroupMap;
	map<string,struct GroupMembers*> groupMembersMap;
	map<string,struct ClientGroups*> clientGroupsMap;
	map<string,struct ClientAuthentication*> clientAuthMap;
	map<string,struct ClientAddress*> clientAddressMap;
	Tracker()
	{
		struct ClientAuthentication *temp = new ClientAuthentication;
		if(temp == NULL)
			cout<<"sdas"<<endl;
		temp->clientId = "Mr.nobody";
		temp->password = " ";
		temp->loggedInFlag = false;
		clientAuthMap[temp->clientId] = temp;
	}
	string createUser(string client_name,string pass_word)
	{
		if(clientAuthMap.find(client_name) != clientAuthMap.end())
		{
			//client with given id already exists choose another id
			return "client_id is already in use";
		}
		
		//insert entry into clientAuthentication
		struct ClientAuthentication *temp = new ClientAuthentication;
		if(temp == NULL)
			cout<<"sdas"<<endl;
		temp->clientId = client_name;
		//cout<<client_name<<pass_word<<endl;
		temp->password = pass_word;
		temp->loggedInFlag = false;
		clientAuthMap[client_name] = temp;
		temp = NULL;
		free(temp);
		struct ClientGroups *temp1 = new ClientGroups;
		temp1->client_id = client_name;
		clientGroupsMap[client_name] = temp1;
		return "created succesfully";
	}
	string login(string client_id,string pass_word,string client_ip,int port)
	{
		if(clientAuthMap.find(client_id) != clientAuthMap.end())
		{
			if(clientAuthMap[client_id]->password == pass_word)
			{
				clientAuthMap[client_id]->loggedInFlag = true;
				struct ClientAddress *temp = new ClientAddress;
				temp->client_id = client_id;
				temp->clientIP = client_ip;
				temp->clientPort = port;
				clientAddressMap[client_id] = temp;
				return "logged successfully";
			}
			else
			{
				//send incorrect password message
				return "incorrect password";
			}
		}
		return "Not a registered user or logged user";
	}
	string createGroup(string group_id,string client_id)
	{
		if(clientAuthMap.find(client_id) != clientAuthMap.end() && (clientAuthMap[client_id]->loggedInFlag))
		{
			if(groupMembersMap.find(group_id) == groupMembersMap.end())
			{
				//adding to grouplist
				struct GroupMembers *temp = new GroupMembers;
				temp->groupId = group_id;
				temp->groupOwner = client_id;
				temp->groupsClientsList.push_back(client_id);
				groupMembersMap[group_id] = temp;
				//......add instance corresponding to group in filesInGroupmap.........//
				filesInGroupMap[group_id] = new FilesInGroup;
				return "Group created successfully";
			}
			else
			{
				return "group with given name alreay exist";
			}
		}
		else
		{
			//send Unauthorised to perfrom message to client
			return "Not a registered client or logged user";
		}
	}
	bool isClientLoggedIn(string client_id)
	{
		return (clientAuthMap[client_id]->loggedInFlag);
	}
	string joinGroup(string group_id,string client_id)
	{
		if(clientAuthMap.find(client_id) != clientAuthMap.end() && (clientAuthMap[client_id]->loggedInFlag))
		{
			if(groupMembersMap.find(group_id) == groupMembersMap.end())
			{
				//send message saying no such group
				return "No such group";
			}
			groupMembersMap[group_id]->pendingRequestsList.push_back(client_id);
			//send request palced message to client
			return "request placed successfully";
		}
		else
		{
			//send Unauthorised to perfrom message to client
			return "not a registered user or logged user";
		}
	}
	string leaveGroup(string group_id,string client_id)
	{
		if(clientAuthMap.find(client_id) != clientAuthMap.end() && (clientAuthMap[client_id]->loggedInFlag))
		{
		//remove client entry from group...................
			if(groupMembersMap.find(group_id) == groupMembersMap.end())
			{
				//send no such group member exist message
				return "No such group exists";
			}
			vector<string>::iterator position = find(groupMembersMap[group_id]->groupsClientsList.begin(),groupMembersMap[group_id]->groupsClientsList.end(),client_id);
			if(position == groupMembersMap[group_id]->groupsClientsList.end())
			{
				//send not a member of group message
				return "Not a member of group at first place";
			}
			groupMembersMap[group_id]->groupsClientsList.erase(position);
		//remove group entry from client....................
			vector<string>::iterator position2 = find(clientGroupsMap[client_id]->clientGroupList.begin(),clientGroupsMap[client_id]->clientGroupList.end(),group_id);
			if(position2 == clientGroupsMap[client_id]->clientGroupList.end())
			{
				//send not a member of group message
				return "Not a member of group";
			}
			clientGroupsMap[client_id]->clientGroupList.erase(position2);
			//send left the group successfully message
			return "left the group successfully";
			
		}
		else
		{
			//send Unauthorised to perfrom message to client
			return "not a registered user or not logged user";
		}
	}
	string pendingGroupRequests(string client_id,string group_id)
	{
		if(groupMembersMap[group_id]->groupOwner == client_id)
		{
			string strm;
			for(unsigned int  i = 0;i < groupMembersMap[group_id]->pendingRequestsList.size();i++)
			{
				strm += " "+ groupMembersMap[group_id]->pendingRequestsList[i];
			}
			return strm;
		}
		else
		{
			//sorry only owner of group can view
			return "Error:Not a owner for this group";
		}
	}
	string acceptRequest(string group_id,string clientToBeAccepted,string client_id)
	{
		cout<<group_id<<" "<<clientToBeAccepted<<" "<<client_id<<endl;
		if(clientAuthMap.find(client_id) != clientAuthMap.end() && (clientAuthMap[client_id]->loggedInFlag))
		{
			if(groupMembersMap.find(group_id) == groupMembersMap.end())
			{
				//send no such group member exist message
				return "No such group exists";
			}
			if(find(groupMembersMap[group_id]->pendingRequestsList.begin(),groupMembersMap[group_id]->pendingRequestsList.end(),clientToBeAccepted) != groupMembersMap[group_id]->pendingRequestsList.end())
			{
				cout<<"asadas"<<endl;
				//adding client to groups clientList
				vector<string>::iterator position = find(groupMembersMap[group_id]->groupsClientsList.begin(),groupMembersMap[group_id]->groupsClientsList.end(),clientToBeAccepted);
				if(position == groupMembersMap[group_id]->groupsClientsList.end())
					groupMembersMap[group_id]->groupsClientsList.push_back(clientToBeAccepted);
				else
					return "Already you are member";
				//adding group to client groupList
				cout<<"asadas"<<endl;
				vector<string>::iterator position2 = find(clientGroupsMap[clientToBeAccepted]->clientGroupList.begin(),clientGroupsMap[clientToBeAccepted]->clientGroupList.end(),group_id);
				if(position2 == clientGroupsMap[clientToBeAccepted]->clientGroupList.end())
					clientGroupsMap[clientToBeAccepted]->clientGroupList.push_back(group_id);
				//send message to client saying request is accepted
				return clientToBeAccepted + "accepted";
			}
			else
			{
				//no such request from client
				return "no such request from client";
			}
		}
		else
		{
			return "unauthorised to perform operation";
			//send Unauthorised to perfrom message to client
		}
	}
	string listGroups()
	{
		map<string,struct GroupMembers*>::iterator it;
		string strm;
		for(it = groupMembersMap.begin();it != groupMembersMap.end();it++)
		{
			strm += it->first + " ";
		}
		return strm;
	}
	string listFilesInGroup(string group_id)
	{
		string strm;
		vector<pair<string,struct FileInfo*>>::iterator it = filesInGroupMap[group_id]->filesVector.begin();
		for(;it !=filesInGroupMap[group_id]->filesVector.end();it++)
		{
			strm += it->first + " ";
		}
		return strm;
	}
	string uploadFile(string file_path,string group_id,streampos size,string client_id)
	{
		if(clientAuthMap.find(client_id) != clientAuthMap.end() && (clientAuthMap[client_id]->loggedInFlag))
		{
			if(groupMembersMap.find(group_id) == groupMembersMap.end())
			{
				//send message saying no such group
				return "No such group";
			}
			vector<string>::iterator position = find(groupMembersMap[group_id]->groupsClientsList.begin(),groupMembersMap[group_id]->groupsClientsList.end(),client_id);
			if(position == groupMembersMap[group_id]->groupsClientsList.end())
			{
				//send not a member of group message
				return "Not a member of group at first place";
			}
			char str[1024];
			strcpy(str,file_path.c_str());
			char * pch,*prevpch;
			pch = strtok (str,"/");
			while (pch != NULL)
			{
				prevpch = pch;
				pch = strtok (NULL, "/");
			}
			
			//int pos = 0;
			pch = prevpch;
			string fileName(pch);
			//.............if file is not present already...........//
			vector<pair<string,struct FileInfo*>>::iterator it;
			it  = find_if( filesInGroupMap[group_id]->filesVector.begin(),filesInGroupMap[group_id]->filesVector.end(),
			[fileName](const pair<string,struct FileInfo*>& element){ return element.first == fileName;});
			if(it == filesInGroupMap[group_id]->filesVector.end())
			{
				//cout<<"phod dhiya"<<endl;
				struct FileInfo *fileInfotemp = new FileInfo;
				pair<string,string> clientPathPair = make_pair(client_id,file_path);
				fileInfotemp->fileSize = size;
				fileInfotemp->clientsHavingFile.push_back(clientPathPair);
				//insert SHA1
				struct FilesInGroup *temp = new FilesInGroup;
				temp->groupId = group_id;
				pair<string,struct FileInfo*> instance  = make_pair(fileName,fileInfotemp);
				filesInGroupMap[group_id]->filesVector.push_back(instance);
				return "uploaded successfully";
			}
			//.........................file is already present....................
			else
			{
				//.............check if client already in vector of clients..........//
				if((find_if(it->second->clientsHavingFile.begin(),it->second->clientsHavingFile.end(),
				[client_id](const pair<string,string>& element){ return element.first == client_id;} ) == it->second->clientsHavingFile.end()))
				{
				it->second->clientsHavingFile.push_back({client_id,file_path});
					return "uploaded successfully";
				}
				//............if present then just send acknowledgement............//
				else
				{
					return "uploaded successfully";
				}
			}
		}
		else
		{
			//send Unauthorised to perfrom message to client
			return "not a registered user or logged user";
		}
		return " ";
	}
	string downloadFile(string group_id,string file_name,string client_id)
	{
		if(clientAuthMap.find(client_id) != clientAuthMap.end() && (clientAuthMap[client_id]->loggedInFlag))
		{
			if(groupMembersMap.find(group_id) == groupMembersMap.end())
			{
				//send message saying no such group
				return "No such group";
			}
			vector<pair<string,struct FileInfo*>>::iterator it;
			it  = find_if( filesInGroupMap[group_id]->filesVector.begin(),filesInGroupMap[group_id]->filesVector.end(),
			[file_name](const pair<string,struct FileInfo*>& element){ return element.first == file_name;});
			if(it == filesInGroupMap[group_id]->filesVector.end())
			{
				return "no file with given name";
			}
			else
			{
				string str;
				vector<pair<string,string>>::iterator childit = it->second->clientsHavingFile.begin();
				for(;childit != it->second->clientsHavingFile.end();childit++)
				{
					str = childit->second+":";
					str = str + clientAddressMap[childit->first]->clientIP + ":";
					str = str + to_string(clientAddressMap[childit->first]->clientPort) +":";
					str = str + to_string(it->second->fileSize);
				}
				return str;
			}
		}
		else
		{
			return "not a registered user or logged user";
		}
	}
	string logout(string client_id)
	{
		clientAuthMap[client_id]->loggedInFlag = false;
		clientAddressMap.erase(client_id);
		//send logged out message
		return "logged out";
	}
	string stopSharing(string group_id,string fileName,string client_id)
	{
		if(clientAuthMap.find(client_id) != clientAuthMap.end() && (clientAuthMap[client_id]->loggedInFlag))
		{
			vector<string>::iterator position = find(clientGroupsMap[client_id]->clientGroupList.begin(),clientGroupsMap[client_id]->clientGroupList.end(),group_id);
			if(position == clientGroupsMap[client_id]->clientGroupList.end())
			{
				//send not a member of group message
				return "Not a member of group";
			}
			else
			{
				vector<pair<string,struct FileInfo*> >::iterator it = filesInGroupMap[group_id]->filesVector.begin();
				for(;it != filesInGroupMap[group_id]->filesVector.end();it++)
				{
					if(it->first == fileName)
						break;
					else
						continue;
				}
				vector<pair<string,string>>::iterator position2 = find_if(it->second->clientsHavingFile.begin(),it->second->clientsHavingFile.end(),[client_id](const pair<string,string>& element){ return element.first ==client_id;});
				it->second->clientsHavingFile.erase(position2);
				return "operation stopSharing done successfully";
			}
		}
		else
		{
			//not authorised to perform task
			return "not authorised to perform task";
		}
	}
};
void callApproFunc(Tracker &,vector<string> commandStrings,string&,int,struct sockaddr_in &);
struct argStruct
{
	int activefd;
	struct sockaddr_in clientAddress;
	Tracker *trckrptr;
};
int main()
{
	pthread_t serverThread;
	pthread_create(&serverThread,NULL,initServer,NULL);
	void* threadReturn;
	int err = pthread_join(serverThread, &threadReturn);
	if (err != 0)
	{
		cout<<"can't join with serverThread"<<err<<endl;
	}
	
}
void* initServer(void *arg1)
{
	Tracker trckr;
	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port=htons(3000);
	serverAddress.sin_addr.s_addr=inet_addr("127.0.0.1");
	int socketfd;
	struct argStruct *arg=new argStruct;
	if ((socketfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
		return(void *)-1;
	int x;
	if ((x = bind(socketfd,(struct sockaddr *)&serverAddress,sizeof(serverAddress))) < 0)
	{
		cout<<"Binding error"<<errno<<endl;
		return (void*)-1;
	}
	for(; ; )
	{
		if (listen(socketfd, QLENGTH) < 0)
		{
			cout<<"Error while listening.."<<endl;
			return (void*)-1;
		}
		int activefd;
		struct sockaddr_in clientAddress;
		socklen_t length;
		length  = sizeof(clientAddress);
		if((activefd = accept(socketfd,(struct sockaddr*)&clientAddress,&length))< 0)
		{
			cout<<"error"<<endl;
			continue;
		}
		pthread_t objectCreationThread;
		arg->activefd = activefd;
		arg->clientAddress = clientAddress;
		arg->trckrptr = &trckr;
		pthread_create(&objectCreationThread,NULL,objectCreation,arg);
	}
	return (void*)0;
}
void *objectCreation(void *arg)
{
	string client_name = "Mr.nobody";
	struct argStruct tempArg  = *((struct argStruct *)arg);
	int activefd = tempArg.activefd;
	struct sockaddr_in clientAddress = tempArg.clientAddress;
	/*for(; ;)
	{
		length  = sizeof(clientAddress);
		if((activefd = accept(passivefd,(struct sockaddr*)&clientAddress,&length))< 0)
		{
			cout<<"error"<<endl;
			continue;
		}
		break;
	}*/
	while(1)
	{
		char Buffer[4096];
		recv(activefd,Buffer,4096,0);
		string str(Buffer);
		cout<<str<<endl;
		stringstream inputStream(str);
		vector<string> commandStrings;
		string str2;
		while(inputStream >> str2)
		{
			commandStrings.push_back(str2);
		}
		callApproFunc(*(tempArg.trckrptr),commandStrings,client_name,activefd,clientAddress);
	}
	
}
void callApproFunc(Tracker &trckr,vector<string> commandStrings,string &client_name,int activefd,struct sockaddr_in &clientAddress)
{
	
	if(commandStrings[0] == "create_user")
	{
		string client_id = commandStrings[1];
		string password = commandStrings[2];
		string output = trckr.createUser(client_id,password);
		sendData(activefd,output);
	}
	else if(commandStrings[0] == "login")
	{
		//if login successfull
		client_name = commandStrings[1];
		string password = commandStrings[2];
		char client_ip[20];
		string commandstr = commandStrings[3];
		strcpy(client_ip,commandstr.c_str());
		int port = stoi(commandStrings[4]);
		string str = trckr.login(client_name,password,client_ip,port);
		sendData(activefd,str);
	}
	else if(commandStrings[0] == "create_group")
	{
		string group_id = commandStrings[1];
		string str = trckr.createGroup(group_id,client_name);
		sendData(activefd,str);
	}
	else if(commandStrings[0] == "join_group")
	{
		string group_id = commandStrings[1];
		string str = trckr.joinGroup(group_id,client_name);
		sendData(activefd,str);
	}
	else if(commandStrings[0] == "leave_group")
	{
		string group_id = commandStrings[1];
		string str = trckr.leaveGroup(group_id,client_name);
		sendData(activefd,str);
	}
	else if(commandStrings[0] == "list_requests")
	{
		string group_id = commandStrings[1];
		string str = trckr.pendingGroupRequests(client_name,group_id);
		sendData(activefd,str);
	}
	else if(commandStrings[0] == "accept_request")
	{
		string group_id = commandStrings[1];
		string client_id = commandStrings[2];
		cout<<"calling "<<endl;
		string str = trckr.acceptRequest(group_id,client_id,client_name);
		sendData(activefd,str);
	}
	else if(commandStrings[0] == "list_groups")
	{
		string str = trckr.listGroups();
		sendData(activefd,str);
	}
	else if(commandStrings[0] == "list_files")
	{
		string group_id = commandStrings[1];
		string str = trckr.listFilesInGroup(group_id);
		sendData(activefd,str);
	}
	else if(commandStrings[0] == "logout") 
	{
		string str = trckr.logout(client_name);
		sendData(activefd,str);
	}
	else if(commandStrings[0] == "upload_file")
	{
		string filePath = commandStrings[1];
		string groupId = commandStrings[2];
		int size = stoi(commandStrings[3]);
		string str = trckr.uploadFile(filePath,groupId,size,client_name);
		sendData(activefd,str);
	}
	else if(commandStrings[0] == "download_file")
	{
		string fileName = commandStrings[2];
		string groupId = commandStrings[1];
		string str = trckr.downloadFile(groupId,fileName,client_name);
		sendData(activefd,str);
	}
	else
	{
		string str = "Invalid Command";
		sendData(activefd,str);
	}
}
void sendData(int activefd,string str)
{
	char Buffer[4096];
	strcpy(Buffer,str.c_str());
	send(activefd,Buffer,str.size()+1,0);
	cout<<"sent data"<<endl;
	return;	
}