/*
 ============================================================================
 Name        : Esonero.c
 Author      : Enrico Ciciriello -  Simone Summo
 Version     :
 Copyright   : MIT
 Description : Online Calculator (client)
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "protocol.h"

#if defined WIN32
#include <winsock.h>
#else
#define closesocket close
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#define BUFFSIZE (256)

int initializeWSA();
void clearwinsock();
void errormsg(const char* msg);
int userinteraction(int socket, struct sockaddr_in serverAddr);
void extractop(cpack *pack, const char* s);
int subchar(char *s);

int main(int argc, char *argv[]) {
	struct sockaddr_in serverAddr;
	unsigned long addr;
	int returnv;
	int sock;
	int port;

	if (initializeWSA() == -1) {
	    errormsg("WSA initialization failed");
	    return -1;
	}

	// Arguments extraction

	// Default arguments
	if (argc < 2) {
		addr = inet_addr("127.0.0.1");
		port = PORT;
	// User arguments
	} else if (argc == 2) {
		char address[BUFFSIZE];

		if (subchar(argv[1]) < 0) {
			errormsg("Argument format wrong, correct format is '<address>:<port>'");
			return -1;
		}

		if (sscanf(argv[1], "%s %d", address, &port) < 2) {
			errormsg("Argument format wrong, correct format is '<address>:<port>'");
			return -1;
		}

		if (strlen(address) < 2) {
			errormsg("Address is too short, default address selected");
			strcpy(address, "localhost");
		}

		if(port < 1 || port > 65535) {
			errormsg("Port invalid, default port selected");
			port = PORT;
		}

		addr = inet_addr((gethostbyname(address))->h_addr_list[1]);
	// Wrong number of user arguments
	} else {
		errormsg("Too many arguments, retry with 1 argument (<address>:<port>)");
		return -1;
	}

	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		errormsg("Socket creation failed");
		return -1;
	}

	// Server address configuration
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = PF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = addr;

	returnv = userinteraction(sock, serverAddr);
	closesocket(sock);
	system("PAUSE");
	clearwinsock();
	return returnv;
}

int userinteraction(int socket, struct sockaddr_in serverAddr) {
	struct sockaddr_in from;
	char *serverName;
	char *serverIP;
	char s[BUFFSIZE];
    int active = 1;
    int errorc = 0;
    int fromSize;
    cpack sendp;
    spack recvp;

    while (active) {
        // If the client failed to send/receive for 3 or more times the app close itself
        if(errorc >= 3) {
            errormsg("Communication failed with the server, closing the app");
            return -1;
        }

        // Read operation from the user
        printf("Insert operation: ");
        scanf("%64[^\n]", s);
        fflush(stdin);
        fflush(stdout);
        extractop(&sendp, s);

        // Send the package to the server
        if (sendp.operation != 'i') {
            sendp.operand1 = htonl(sendp.operand1);
            sendp.operand2 = htonl(sendp.operand2);

            if (sendto(socket, &sendp, sizeof(sendp), 0, &serverAddr, sizeof(serverAddr)) < 0) {
                errormsg("Failed to send operation to the server, please check and retry");
                errorc++;
            } else {

            	// Mark the closing of the connection
            	if (sendp.operation == '=') {
            		return 0;
            	}

            	fromSize = sizeof(from);
            	// Receive the result from the server
                if (recvfrom(socket, &recvp, sizeof(recvp), 0, &from, &fromSize) < 0) {
                    errormsg("Failed to receive operation from the server, please check and retry");
                    errorc++;
                } else {

                    errorc = 0;
                    recvp.error = (recvp.error);
                    recvp.result = ntohl(recvp.result);
                    sendp.operand1 = ntohl(sendp.operand1);
                    sendp.operand2 = ntohl(sendp.operand2);

                    if (serverAddr.sin_addr.s_addr != from.sin_addr.s_addr ) {
                    	errormsg("Received package from unknown source");
                    	recvp.error = -1;
                    }

                    // If error, inform the user
                    if(recvp.error != 0) {
                        switch (recvp.error) {
                            case 1:
                                errormsg(ERROR1);
                                break;
                            case 2:
                                errormsg(ERROR2);
                                break;
                            case 3:
                            	errormsg(ERROR3);
                            	break;
                            case 4:
                            	errormsg(ERROR4);
                            	break;
                            default:
                                errormsg("Unknown error");
                        }
                    // Otherwise print the result
                    } else {
                        serverIP = &from.sin_addr;
                        serverName = gethostbyaddr(serverIP, sizeof(from.sin_addr), PF_INET)->h_name;
                    	if (sendp.operation == '/') {
                    		printf("Ricevuto risultato dal server %s, ip %s: %d %c %d = %0.2f\n",
                    				serverName, inet_ntoa(from.sin_addr), sendp.operand1, sendp.operation, sendp.operand2, recvp.result);
                    	} else {
                    		printf("Ricevuto risultato dal server %s, ip %s: %d %c %d = %d\n",
                    				serverName, inet_ntoa(from.sin_addr), sendp.operand1, sendp.operation, sendp.operand2, (int) recvp.result);
                    	}
                    }
                }
            }
        } else {
            errormsg("Operation format invalid or numbers exceeding range (range: [-2147483648, +2147483647])");
        }
    }

    return -1;
}

void extractop(cpack *pack, const char* s) {
	long long op1, op2;
	char op;

	if (s[0] == '=') {
		pack->operation = s[0];
		return;
	}

	if (strlen(s) >= 5 && strlen(s) < 26) {
        if (sscanf(s, "%c %lld %lld", &op, &op1, &op2) < 3) {
        	pack->operation = 'i';
        } else if (op != '+' && op != '-' && op != '=' && op != 'x' && op != '/') {
        	pack->operation = 'i';
        } else {
        	if (op1 >= INT_MIN && op1 <= INT_MAX && op2 >= INT_MIN && op2 <= INT_MAX) {
        		pack->operation = op;
        		pack->operand1 = op1;
        		pack->operand2 = op2;
        	} else {
        		pack->operation = 'i';
        	}
        }
    } else {
    	pack->operation = 'i';
    }
}

int initializeWSA() {
#if defined WIN32
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) {
		errormsg("StartUp failed");
		return -1;
	}
#endif
	return 0;
}

void clearwinsock(){
	#if defined WIN32
	WSACleanup();
	#endif
}


void errormsg(const char* msg) {
    printf("Error: %s\n", msg);
}

int subchar(char *s) {
	int i = 0;
	while (s[i] != '\0') {
		if(s[i] == ':') {
			s[i] = ' ';
			return 0;
		}
	}
	return -1;
}
