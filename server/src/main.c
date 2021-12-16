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

void ErrorHandler(const char *errorMessage);
void ClearWinSock();
int handleClient(int socket, struct sockaddr_in clientAddr);

int main(int argc, char *argv[]){

	// initialize WSA
	    #if defined WIN32
	    WSADATA wsaData;
	    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	    if(iResult != 0){
	    	ErrorHandler("WSAStartup failed");
	        return EXIT_FAILURE;
	    }
	    #endif

    int port;
    if(argc > 1){
        port = atoi(argv[1]);
        if(port < 1 || port > 65535){
        	printf("Error: Bad port number %s \n", argv[1]);
            return -1;
        }
     } else {
        port = PORT;
    }

    int sock;
    struct sockaddr_in ServAddr;
    struct sockaddr_in ClntAddr;

    // socket creation
    if((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        ErrorHandler("Socket creation failed");
        ClearWinSock();
        return -1;
    }

    // address configuration
    memset(&ServAddr, 0, sizeof(ServAddr));
    ServAddr.sin_family = AF_INET;
    ServAddr.sin_port = htons(PORT);
    ServAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // bind of the socket
    if((bind(sock, (struct sockaddr *)&ServAddr, sizeof(ServAddr))) < 0){
        ErrorHandler("Server binding failed");
        ClearWinSock();
        return -1;
    }

    printf("Waiting for a client to connect...\n");
    fflush(stdout);

    // receiving data from client
    while(1){
        if (handleClient(sock, ClntAddr) < 0) {
        	ErrorHandler("Unexpected client connection closed");
        }
    }

    return 0;
}

// output error messages
void ErrorHandler(const char *errorMessage){
    printf("Error: %s\n", errorMessage);
    fflush(stdout);
}

void ClearWinSock(){
    #if defined WIN32
    WSACleanup();
    #endif
}

int handleClient(int socket, struct sockaddr_in clientAddr){
    struct sockaddr_in from;
    int result;

    cpack rcv;
    spack snd;

    // receiving package from the client
    if(recvfrom(socket, &rcv, sizeof(rcv), 0, &from, sizeof(from)) < 0){
        return 0;
    } else {


        // conversion data from network to host long
        rcv.operand1 = ntohl(rcv.operand1);
        rcv.operand2 = ntohl(rcv.operand2);

        printf("Richiesta operazione '%c %d %d' dal client %s, ip %s\n", rcv.operation, rcv.operand1, rcv.operand2,
               "test", inet_ntoa(clientAddr.sin_addr));

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
                printf("Connection closed with %s:%d \n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
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

    return 0;
}
