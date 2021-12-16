/*
 ============================================================================
 Name        : main.c
 Author      : Enrico Ciciriello -  Simone Summo
 Version     :
 Copyright   : MIT
 Description : Online calculator (server)
 ============================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include "calculator.h"
#include "protocol.h"

#if defined WIN32
#include <winsock.h>
#else
#define closesocket close
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#endif

#define BUFFSIZE 256

void ErrorHandler(const char *errorMessage);
void ClearWinSock();
int handleClient(int socket, struct sockaddr_in clientAddr, struct hostent *remoteHost);

int main(int argc, char *argv[]){
    struct hostent *remoteHost;
    char *host_name;
    struct in_addr addr;

    host_name = argv[1];
    if(isalpha(host_name[0])){
        // host by name
        remoteHost = gethostbyname(host_name);
    } else {
        // host by address
        addr.s_addr = inet_addr(host_name);
        remoteHost = gethostbyaddr((char*)&addr, 4, AF_INET);
    }

    int port;
    if(argc > 1){
        port = atoi(argv[1]);
        if(port < 1 || port > 65535){
            printf("bad port number %s \n", argv[1]);
            return -1;
        }
     } else {
        port = PORT;
    }

    // initialize WSA
    #if defined WIN32
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if(iResult != 0){
        printf("Error at WSAStartup \n");
        return EXIT_FAILURE;
    }
    #endif

    int sock;
    struct sockaddr_in ServAddr;
    struct sockaddr_in ClntAddr;
    char buffer[buffsize];

    // socket creation
    if(sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        ErrorHandler("socket() failed");
        ClearWinSock();
        return -1;
    }

    // address configuration
    memset(&ServAddr, 0, sizeof(ServAddr));
    ServAddr.sin_family = AF_INET;
    ServAddr.sin_port = htons(PORT);
    ServAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // bind of the socket
    if(bind(sock, (struct sockaddr *)&ServAddr, sizeof(ServAddr))) < 0){
        ErrorHandler("bind() failed");
        ClearWinSock();
        return -1;
    }

    printf("Waiting for a client to connect...\n");

    // receiving data from client
    while(recvfrom(sock, buffer, BUFFSIZE, 0, (struct sockaddr*)&ClntAddr)){
        handleClient(sock, ClntAddr, &remoteHost);
    }
}

// output error messages
void ErrorHandler(char *errorMessage){
    printf(errorMessage);
}

void ClearWinSock(){
    #if defined WIN32
    WSACleanup();
    #endif
}

int handleClient(int socket, struct sockaddr_in clientAddr, struct hostent *remoteHost){
    unsigned int cliAddrLen;
    char Buffer[BUFFSIZE];
    struct sockaddr_in from;

    cpack rcv;
    spack snd;

    // receiving package from the client
    if(recvfrom(socket, &rcv, sizeof(rcv), 0, &from, sizeof(from)) < 0){
        ErrorHandler("Failed to receive data from the client, please check and retry");
        return -1;
    } else {


        // conversion data from network to host long
        rcv.operand1 = ntohl(rcv.operand1);
        rcv.operand2 = ntohl(rcv.operand2);

        printf("Richiesta operazione '%c %d %d' dal client %s, ip %s\n", rcv.operation, rcv.operand1, rcv.operand2,
               remoteHost, inet_ntoa(clientAddr.sin_addr));

        switch(rcv.operation){
            // Addition
            case '+':
                if ((rcv.operand2 > 0) && (rcv.operand1 > INT_MAX - rcv.operand2)){
                    // a + b overflow
                    snd.result = 0;
                    snd.error = 3;
                } else if ((rcv.operand2 < 0) && (rcv.operand1 < INT_MIN - rcv.operand2)){
                    // a + b underflow
                    snd.result = 0;
                    snd.error = 4;
                } else {
                    // no overflow or underflow
                    result = add(rcv.operand1, rcv.operand2);
                    snd.result = result;
                    snd.error = 0;
                }
                break;
                // Subtraction
            case '-':
                if ((rcv.operand2 < 0) && (rcv.operand1 > INT_MAX + rcv.operand2)){
                    // a - b overflow
                    snd.result = 0;
                    snd.error = 3;
                } else if ((rcv.operand2 > 0) && (rcv.operand1 < INT_MIN + rcv.operand2)){
                    // a - b underflow
                    snd.result = 0;
                    snd.error = 4;
                } else {
                    // no overflow or underflow
                    result = sub(rcv.operand1, rcv.operand2);
                    snd.result = result;
                    snd.error = 0;
                }
                break;
                // Multiplication
            case 'x':
                if (rcv.operand1 > INT_MAX / rcv.operand2){
                    // a x b overflow
                    snd.result = 0;
                    snd.error = 3;
                } else if (rcv.operand1 < INT_MIN / rcv.operand2){
                    // a x b underflow
                    snd.result = 0;
                    snd.error = 4;
                } else {
                    // no overflow or underflow
                    result = mult(rcv.operand1, rcv.operand2);
                    snd.result = result;
                    snd.error = 0;
                }
                break;
                // Division
            case '/':
                if(rcv.operand2 == 0){
                    snd.result = 0;
                    snd.error = 2;
                } else {
                    result = division(rcv.operand1, rcv.operand2);
                    snd.result = result;
                    snd.error = 0;
                }
                break;
                // Closing connection with client
            case '=':
                printf("Connection closed with %s:%d \n", inet_ntoa(cad->sin_addr), ntohs(cad->sin_port));
                closesocket(socket);
                printf("Waiting for a client to connect... \n");
                return 0;
                break;
                // Unrecognized symbol
            default:
                snd.error = 1;
                snd.result = 0;
        }

    }

    // Conversion data from host to network long
    snd.result = (snd.result);
    snd.error = htonl(snd.error);

    // sending results to client
    if(sendto(socket, &snd, sizeof(snd), 0, &clientAddr, sizeof(clientAddr)) < 0){
        ErrorHandler("sendto() sent different number of bytes than expected");
    }
}
