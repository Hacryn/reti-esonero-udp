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
#endif

#define BUFFSIZE 256

void ErrorHandler(const char *errorMessage);
void ClearWinSock();
int handleClient(int socket, struct sockaddr_in clientAddr);

int main(int argc, char *argv[]){
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

    // INITIALIZE WSA
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

    // SOCKET CREATION
    if(sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        ErrorHandler("socket() failed");
        ClearWinSock();
        return -1;
    }

    // ADDRESS CONFIGURATION
    memset(&echoServAddr, 0, sizeof(ServAddr));
    echoServAddr.sin_family = AF_INET;
    echoServAddr.sin_port = htons(PORT);
    echoServAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // BIND OF THE SOCKET
    if(bind(sock, (struct sockaddr *)&ServAddr, sizeof(ServAddr))) < 0){
        ErrorHandler("bind() failed");
        ClearWinSock();
        return -1;
    }

    // RECEIVING ECHO STRING FROM CLIENT
    while(1){
        handleClient(sock, ClntAddr);
    }
}

void ErrorHandler(char *errorMessage){
    printf(errorMessage);
}

void ClearWinSock(){
    #if defined WIN32
    WSACleanup();
    #endif
}

int handleClient(int socket, struct sockaddr_in clientAddr){
    printf("Handling client: %s\n", inet_ntoa(clientAddr.sin_addr));

    unsigned int cliAddrLen;
    char Buffer[BUFFSIZE];
    struct sockaddr_in from;

    cpack rcv;
    spack snd;

    // RECEIVING PACKAGE FROM THE CLIENT
    if(recvfrom(socket, &rcv, sizeof(rcv), 0, &from, sizeof(from)) < 0){
        ErrorHandler("Failed to receive data from the client, please check and retry");
    } else {
        /* TO DO:
         * implementare il messaggio:
         * "Richiesta operazione '+ 23 45' dal client pippo.di.uniba.it, ip 193.204.187.154"
         * RICORDA di usare la parte "dns" per la parte pippo.di.uniba.it
         */

        // CONVERSION DATA FROM NETWORK TO HOST LONG
        rcv.operand1 = ntohl(rcv.operand1);
        rcv.operand2 = ntohl(rcv.operand2);

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
                /*
                 * TO DO:
                 * RICORDA che ora la divisione ritorna float
                 */
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
    snd.result = htonl(snd.result);
    snd.error = htonl(snd.error);

    // SENDING ECHO STRING TO CLIENT
    if(sendto(socket, &snd, sizeof(snd), 0, &clientAddr, sizeof(clientAddr)) < 0){
        ErrorHandler("sendto() sent different number of bytes than expected");
    }
}
