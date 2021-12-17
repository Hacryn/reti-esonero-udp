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
int handleClient(int socket);

int main(int argc, char *argv[]){

	// initialize WSA
	    #if defined WIN32
	    WSADATA wsaData;
	    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	    if(iResult != 0){
	    	ErrorHandler("WSAStartup fallito");
	        return EXIT_FAILURE;
	    }
	    #endif

    int port;
    if(argc == 2){
        port = atoi(argv[1]);
        if(port < 1 || port > 65535){
        	printf("Errore: Numero di porta non valido %s \n", argv[1]);
            return -1;
        }
     } else {
        port = PORT;
    }

    int sock;
    struct sockaddr_in ServAddr;

    // socket creation
    if((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        ErrorHandler("Creazione del socket fallita");
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
        ErrorHandler("Binding del server fallito");
        ClearWinSock();
        return -1;
    }

    printf("Aspettando richieste dai client...\n");
    fflush(stdout);

    // receiving data from client
    while(1){
        if (handleClient(sock) < 0) {
        	ErrorHandler("Interazione con il client interrotta");
        }
    }

    return 0;
}

// output error messages
void ErrorHandler(const char *errorMessage){
    printf("Errore: %s\n", errorMessage);
    fflush(stdout);
}

void ClearWinSock(){
    #if defined WIN32
    WSACleanup();
    #endif
}

int handleClient(int socket){
    struct sockaddr_in from;
    char *hostaddr;
    char *hostname;
    int clientAddrSize;
    int result;

    cpack rcv;
    spack snd;

    // receiving package from the client
    clientAddrSize = sizeof(from);
    if(recvfrom(socket, &rcv, sizeof(rcv), 0, &from, &clientAddrSize) == SOCKET_ERROR ){
    	ErrorHandler("Ricevimento fallito dal client");
        return 1;
    } else {
        // conversion data from network to host long
        rcv.operand1 = ntohl(rcv.operand1);
        rcv.operand2 = ntohl(rcv.operand2);

        // taking the name of host
        hostaddr = &from.sin_addr;
        hostname = gethostbyaddr(hostaddr, sizeof(from.sin_addr), PF_INET)->h_name;

        if (hostname == NULL) {
        	hostname = "(nome client non trovato)";
        }

        if(rcv.operation != '='){
            printf("Richiesta operazione '%c %d %d' dal client %s, ip %s\n", rcv.operation, rcv.operand1, rcv.operand2,
               hostname, inet_ntoa(from.sin_addr));
        }

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
                    snd.result = division((float) rcv.operand1, (float) rcv.operand2);
                    snd.error = 0;
                }
                break;
                // Closing connection with client
            case '=':
                printf("Client %s, ip %s e' andato offline\n", hostname, inet_ntoa(from.sin_addr));
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
    if(sendto(socket, &snd, sizeof(snd), 0, &from, sizeof(from)) < 0){
        ErrorHandler("Risposta al client fallita");
    }

    return 0;
}
