#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <string>
#include <vector>
#include <dirent.h>

///////////////////////////////////////////////////////////////////////////////

#define BUF 5000
#define PORT 6543

///////////////////////////////////////////////////////////////////////////////

std::string c_send();
std::string c_list();
std::string c_read();
std::string c_del();

///////////////////////////////////////////////////////////////////////////////


int main(int argc, char **argv) {
   int create_socket, size, isQuit = 0;
   char buffer[BUF];
   struct sockaddr_in address;
   std::string cli_input, serialized_input;
   
   if (argc != 3) {
        std::cout << "Command usage: " << argv[0] << " [ip] [port]" << std::endl;
        exit(EXIT_FAILURE);
    } 

   if ((create_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)   {
      perror("Socket error");
      return EXIT_FAILURE;
   }

   memset(&address, 0, sizeof(address)); // init storage with 0
   address.sin_family = AF_INET;         // IPv4
   address.sin_port = htons(atoi(argv[2]));
   inet_aton(argv[1], &address.sin_addr);
   
   if (connect(create_socket, (struct sockaddr *)&address,sizeof(address)) == -1)   {
      perror("Connect error - no server available");
      return EXIT_FAILURE;
   }

   printf("Connection with server (%s) established\n", inet_ntoa(address.sin_addr));

   size = recv(create_socket, buffer, BUF - 1, 0);
   if      (size == -1)       perror("recv error");
   else if (size == 0)        printf("Server closed remote socket\n"); // ignore error
   else   {
      buffer[size] = '\0';
      printf("%s", buffer); // ignore error
   }

   do   {
         std::cout << ">> ";
         std::getline(std::cin, cli_input);      
      
         if     (cli_input == "send") serialized_input = c_send();
         else if(cli_input == "list") serialized_input = c_list(); // serialized_input = "list";
         else if(cli_input == "read") serialized_input = c_read();
         else if(cli_input == "del")  serialized_input = c_del();
         else if(cli_input == "quit") isQuit = 1;
         else serialized_input = cli_input;

         if ((send(create_socket, serialized_input.c_str(), serialized_input.length(), 0)) == -1) {
            perror("send error");
            break;
         }

         size = recv(create_socket, buffer, BUF - 1, 0);
      
         if (size > 0) {
            buffer[size] = '\0';
            printf("%s\n", buffer); // ignore error
         }
         else if (size == 0) {
            printf("Server closed remote socket\n"); // ignore error
            break;
         }
         else {            
            perror("recv error");
            break;
         }
      
   } while (!isQuit);

   if (create_socket != -1)   {
      if (shutdown(create_socket, SHUT_RDWR) == -1)      {
         perror("shutdown create_socket"); 
      }
      if (close(create_socket) == -1)      {
         perror("close create_socket");
      }
      create_socket = -1;
   }
   return EXIT_SUCCESS;
}

bool is_number(const std::string& s) {
    return !s.empty() && std::find_if(s.begin(), 
        s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

std::string read_usr_input(int type) {
   std::string input;
   std::string message;

   switch (type) {
      case 1:
      // Sender
         std::cout << "Enter the Sender:" << std::endl;
         std::getline(std::cin, input);
         while(input == "" || input == "." || input == "/" || input == "./" || input.length() > 8) {
            perror("Please use a Minimum of 1 and a Maximum of 8 chars!");
            std::cout << "Enter the Sender:" << std::endl;
            std::getline(std::cin, input);
         }
         break;

      case 2:
      // Receiver
         std::cout << "Enter the Receiver:" << std::endl;
         std::getline(std::cin, input);
         while(input == "" || input == "." || input == "/" || input == "./" || input.length() > 8) {
            perror("Please use a Minimum of 1 and a Maximum of 8 chars!");
            std::cout << "Enter the Receiver:" << std::endl;
            std::getline(std::cin, input);
         }
         break;

      case 3:
      // Subject
         std::cout << "Enter the Subject:" << std::endl;
         std::getline(std::cin, input);
         while(input == "" || input == "." || input == "/" || input == "./" || input.length() > 80) {
            perror("Please use a Minimum of 1 and a Maximum of 80 chars!");
            std::cout << "Enter a Subject:" << std::endl;
            std::getline(std::cin, input);
         }
         break;

      case 4:
      // Message
         std::cout << "Enter the Message Content:" << std::endl;
         while (getline(std::cin, input)) {
            if (input == ".") break;
            message += "\n" + input;
         }
         input = message;
         break;

      case 5:
      // Username
         std::cout << "Enter the Username:" << std::endl;
         std::getline(std::cin, input);
         while(input == "" || input == "." || input == "/" || input == "./" || input.length() > 8) {
            perror("Please use a Minimum of 1 and a Maximum of 8 chars!");
            std::cout << "Enter the Username:" << std::endl;
            std::getline(std::cin, input);
         }
         break;
         
      case 6:
      // Message Number
         std::cout << "Enter the Message Nummer:" << std::endl;
         std::getline(std::cin, input);
         while(!is_number(input) ) {
            perror("Only Numericals are allowed");
            std::cout << "Enter the Message Nummer:" << std::endl;
            std::getline(std::cin, input);
         }
         break;

      default:
         // code to be executed if
         // expression doesn't match any constant
         std::cout << "Error! The operator is not correct";
         break;
   }
   return input; 
}

std::string c_send() {
   std::string sender = read_usr_input(1);
   std::string receiver = read_usr_input(2);
   std::string subject = read_usr_input(3);
   std::string message = read_usr_input(4);

   return "[send]\n" + sender + "\n" + receiver + "\n" + subject + message;
}

std::string c_list() {
   std::string username = read_usr_input(5);

   return "[list]:" + username + ":";
}

std::string c_read() {
   std::string username = read_usr_input(5);
   std::string message_number = read_usr_input(6);

   return "[read]\n" + username + "\n" + message_number;
}

std::string c_del() {
   std::string username = read_usr_input(5);
   std::string message_number = read_usr_input(6);

   return "[del]\n" + username + "\n" + message_number;
}