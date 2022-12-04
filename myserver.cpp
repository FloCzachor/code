#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

///////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <vector>
// #include <thread>
#include <string>
#include <fstream>
#include <filesystem>

///////////////////////////////////////////////////////////////////////////////

#define BUF 5000
#define PORT 6543

int abortRequested = 0;
int create_socket = -1;
int new_socket = -1;

struct msg {
   std::string sender, receiver, subject, content;
};

struct msg_u_mn {
   std::string username, message_number;
};

///////////////////////////////////////////////////////////////////////////////

void *clientCommunication(int socket, std::string spoolDir);
void signalHandler(int sig);

struct msg fetch_msg_content(std::string buffer);
struct msg_u_mn fetch_username_msg_number(std::string buffer);

void s_send(struct msg recv_msg, int current_socket, std::string spoolDir);
void s_list(std::string username, int current_socket, std::string spoolDir);
void s_read(struct msg_u_mn recv_msg, int current_socket, std::string spoolDir);
void s_del(struct msg_u_mn recv_msg, int current_socket, std::string spoolDir);

///////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {
   socklen_t addrlen;
   std::string spoolPath;
   struct sockaddr_in address, cliaddress;
   int reuseValue = 1, port;

    if (argc != 3) {
        std::cout << "Command usage: " << argv[0] << " [port] [mail-spool-directoryname]" << std::endl;
        exit(EXIT_FAILURE);
    } 
   
   port = atoi(argv[1]);
   spoolPath = argv[2];
   
   if (signal(SIGINT, signalHandler) == SIG_ERR) {
      perror("signal can not be registered");
      return EXIT_FAILURE;
   }

   if ((create_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("Socket error"); // errno set by socket()
      return EXIT_FAILURE;
   }

   if (setsockopt(create_socket, SOL_SOCKET, SO_REUSEADDR, &reuseValue, sizeof(reuseValue)) == -1) {
      perror("set socket options - reuseAddr");
      return EXIT_FAILURE;
   }

   if (setsockopt(create_socket, SOL_SOCKET, SO_REUSEPORT, &reuseValue, sizeof(reuseValue)) == -1) {
      perror("set socket options - reusePort");
      return EXIT_FAILURE;
   }

   memset(&address, 0, sizeof(address));
   address.sin_family = AF_INET;
   address.sin_addr.s_addr = INADDR_ANY;
   address.sin_port = htons(port);

   if (bind(create_socket, (struct sockaddr *)&address, sizeof(address)) == -1) {
      perror("bind error");
      return EXIT_FAILURE;
   }

   if (listen(create_socket, 5) == -1) {
      perror("listen error");
      return EXIT_FAILURE;
   }

   while (!abortRequested) {
      printf("Waiting for connections...\n");

      addrlen = sizeof(struct sockaddr_in);
      if ((new_socket = accept(create_socket, (struct sockaddr *)&cliaddress, &addrlen)) == -1) {
         if (abortRequested)  perror("accept error after aborted");
         else                 perror("accept error");
         break;
      }

      printf("Client connected from %s:%d...\n", inet_ntoa(cliaddress.sin_addr), ntohs(cliaddress.sin_port));
      clientCommunication(new_socket, spoolPath); // returnValue can be ignored
      
      // std::thread bread (clientCommunication, new_socket); // returnValue can be ignored
      // bread.detach();
      new_socket = -1;
   }

   if (create_socket != -1) {
      if (shutdown(create_socket, SHUT_RDWR) == -1) {
         perror("shutdown create_socket");
      }
      if (close(create_socket) == -1) {
         perror("close create_socket");
      }
      create_socket = -1;
   }

   return EXIT_SUCCESS;
}

void create_msg_file(std::string filePath, struct msg recv_msg) {
   std::ofstream MyFile(filePath.c_str());
   MyFile << recv_msg.sender   << std::endl << std::endl 
          << recv_msg.receiver << std::endl << std::endl 
          << recv_msg.subject  << std::endl << std::endl 
          << recv_msg.content;
   MyFile.close();
}

/*
FUNCTION s_send()
Takes (struct) msg and saves the data in a file
File directory is the subject-content
File name is the receiver-content
*/
void s_send(struct msg recv_msg, int current_socket, std::string spoolDir) {
   std::string basePath = "./" + spoolDir;
   std::string dirPath = basePath + "/" + recv_msg.receiver;
   std::string filePath = dirPath + "/" + recv_msg.subject;
   
   mkdir(basePath.c_str(), 0700);
   mkdir(dirPath.c_str(), 0700);
   create_msg_file(filePath, recv_msg);

   dirPath = basePath + "/" + recv_msg.sender;
   filePath = dirPath + "/" + recv_msg.subject;

   mkdir(dirPath.c_str(), 0700);
   create_msg_file(filePath, recv_msg);

   send(current_socket, "OK", 3, 0);
}

/*
FUNCTION fetch_msg_content()
Take CLI-Input and spreads it into (struct) msg
*/
struct msg fetch_msg_content(std::string buffer) {
   struct msg client_msg;
   std::istringstream iss(buffer);
   std::string token, final;

   std::getline(iss, token, '\n');
   std::getline(iss, token, '\n');
   client_msg.sender = token;

   std::getline(iss, token, '\n');
   client_msg.receiver = token;
   
   std::getline(iss, token, '\n');
   client_msg.subject = token;

   while (std::getline(iss, token, '\n'))
      final += token + "\n";
   
   client_msg.content = final + "\n";

   return client_msg;
}

/*
FUNCTION fetch_username_msg_number()
Take CLI-Input and spreads it into (struct) msg_u_mn
*/
struct msg_u_mn fetch_username_msg_number(std::string buffer) {
   struct msg_u_mn client_msg;
   std::istringstream iss(buffer);
   std::string token, final;

   std::getline(iss, token, '\n');
   std::getline(iss, token, '\n');
   client_msg.username = token;

   std::getline(iss, token, '\n');
   client_msg.message_number = token;

   return client_msg;
}

/*
FUNKTION s_list()
Lists Content of File with Filename
*/
void s_list(std::string username, int current_socket, std::string spoolDir) {
   DIR *folder;
   struct dirent *entry;
   int count = 0;
   std::string output, dirPath = "./" + spoolDir + "/" + username;

   if((folder = opendir(dirPath.c_str())) == NULL) {
      perror("Unable to read directory");
      send(current_socket, "ERR", 4, 0);
      return;
   }

   for(int i = 0; i < 2; readdir(folder), i++);

   while((entry = readdir(folder))) {
      count++;
      output.append("[" + std::to_string(count) + "] " + entry->d_name + "\n");
   }
   send(current_socket, output.c_str(), strlen(output.c_str()), 0);
}

/*
FUNKTION s_read_or_del()
Displays/Deletes a specific Message of a specific User
*/
void s_read_or_del(int type, struct msg_u_mn recv_msg, int current_socket, std::string spoolDir) {
   DIR *folder;
   int count = 0;
   struct dirent *entry = NULL;
   std::string filePath, tempLine, token, output = "\n[Message-Number: " + recv_msg.message_number + "]\n";
   std::string dirPath = "./" + spoolDir + "/" + recv_msg.username;

   if((folder = opendir(dirPath.c_str())) == NULL) {
      perror("Unable to read directory");
      send(current_socket, "ERR", 4, 0);
      return;
   }
   while((entry = readdir(folder))) count++;

   if((folder = opendir(dirPath.c_str())) == NULL) {
      perror("Unable to read directory");
      send(current_socket, "ERR", 4, 0);
      return;
   }
   if(count -2 < atoi(recv_msg.message_number.c_str())) { 
      perror("Unable to read file");
      send(current_socket, "ERR: Message under this number does not exist", 46, 0);
      return;
   }

   for(int i = 0; i < 2 + atoi(recv_msg.message_number.c_str());  i++) entry=readdir(folder);

   filePath = dirPath + "/" + entry->d_name;
   if(type == 1) {
      std::ifstream MyReadFile(filePath);
      for(int i = 0; i < 6; getline(MyReadFile, tempLine), i++);
      std::cout << filePath << std::endl; 
      std::cout << std::endl;
      while (getline(MyReadFile, tempLine)) {
         output += "\n" + tempLine;
         std::cout << tempLine << std::endl;
      }
      std::cout << std::endl;
      send(current_socket, output.c_str(), strlen(output.c_str()), 0);

   } else if(type == 2) {
      std::remove(filePath.c_str());
      send(current_socket, "OK", 3, 0);
   }
}

void *clientCommunication(int current_socket, std::string spoolDir) {
   int size;
   char buffer[BUF];
   struct msg mail;
 
   strcpy(buffer, "Welcome to myserver!\r\nPlease enter your commands...\r\n");
   if (send(current_socket, buffer, strlen(buffer), 0) == -1)   {
      perror("send failed");
      return NULL;
   }
   memset(buffer, 0 ,sizeof(buffer));

   do {
      size = recv(current_socket, buffer, BUF - 1, 0);
      if (size == -1) {
         if (abortRequested)  perror("recv error after aborted");
         else                 perror("recv error");
         break;
      }

      if (size == 0) {
         printf("Client closed remote socket\n"); 
         break;
      }


      std::cout << buffer << std::endl;

      if (buffer[size - 2] == '\r' && buffer[size - 1] == '\n') size -= 2;
      else if (buffer[size - 1] == '\n')                        --size;    
      buffer[size] = '\0';
           
      std::string cli_command(buffer);
      std::string command = cli_command.substr(cli_command.find_first_of('[')+1, cli_command.find_first_of(']')-1);

      std::cout << "Message/Command received: " << command << std::endl; // ignore error

      if     (command == "send") s_send(fetch_msg_content(cli_command), current_socket, spoolDir);
      else if(command == "list") s_list(cli_command.substr(7, cli_command.find_last_of(':')-7), current_socket, spoolDir);
      else if(command == "read") s_read_or_del(1, fetch_username_msg_number(cli_command), current_socket, spoolDir);
      else if(command == "del")  s_read_or_del(2, fetch_username_msg_number(cli_command), current_socket, spoolDir);
      else if(command == "quit") break; 
      else                       send(current_socket, "OK", 3, 0);

   } while (!abortRequested);

   if (current_socket != -1) {
      if (shutdown(current_socket, SHUT_RDWR) == -1) {
         perror("shutdown new_socket");
      }
      if (close(current_socket) == -1) {
         perror("close new_socket");
      }
      current_socket = -1;
   }
   return NULL;
}

void signalHandler(int sig) {
   if (sig == SIGINT) {
      printf("abort Requested... "); // ignore error
      abortRequested = 1;
      if (new_socket != -1) {
         if (shutdown(new_socket, SHUT_RDWR) == -1) {
            perror("shutdown new_socket");
         }
         if (close(new_socket) == -1) {
            perror("close new_socket");
         }
         new_socket = -1;
      }

      if (create_socket != -1) {
         if (shutdown(create_socket, SHUT_RDWR) == -1) {
            perror("shutdown create_socket");
         }
         if (close(create_socket) == -1) {
            perror("close create_socket");
         }
         create_socket = -1;
      }
   }
   else exit(sig);
}
