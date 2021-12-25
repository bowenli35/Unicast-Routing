#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "link_state_routing.h"
#include "monitor_neighbors.h"


void listenForNeighbors();
void* announceToNeighbors(void* unusedParam);


int globalMyID = 0;
//last time you heard from each node. TODO: you will want to monitor this
//in order to realize when a neighbor has gotten cut off from you.
struct timeval globalLastHeartbeat[MAX];

//our all-purpose UDP socket, to be bound to 10.1.1.globalMyID, port 7777
int globalSocketUDP;
//pre-filled for sending to 10.1.1.0 - 255, port 7777
struct sockaddr_in globalNodeAddrs[MAX];

Node node;

int neighborList[MAX];

FILE *logPtr;

int fowardingTable[MAX];

int costTable[MAX];


int main(int argc, char** argv) {
    
	if (argc != 4) {
		fprintf(stderr, "Usage: %s mynodeid initialcostsfile logfile\n\n", argv[0]);
		exit(1);
	}

	//initialization: get this process's node ID, record what time it is,
	//and set up our sockaddr_in's for sending to the other nodes.
	globalMyID = atoi(argv[1]);
	for (int i = 0; i < MAX; i++) {
		gettimeofday(&globalLastHeartbeat[i], 0);
		char tempaddr[100];
		sprintf(tempaddr, "10.1.1.%d", i);
		memset(&globalNodeAddrs[i], 0, sizeof(globalNodeAddrs[i]));
		globalNodeAddrs[i].sin_family = AF_INET;
		globalNodeAddrs[i].sin_port = htons(7777);
		inet_pton(AF_INET, tempaddr, &globalNodeAddrs[i].sin_addr);

		costTable[i] = 1;
		neighborList[i] = 0;
		fowardingTable[i] = -1;
	}
	

	//TODO: read and parse initial costs file. default to cost 1 if no entry for a node. file may be empty.
    
    
    node = initNode(globalMyID);

    FILE *fp = fopen(argv[2], "r");
    if (fp == NULL) {
        printf("Error: could not open file %s", argv[2]);
        return 1;
    }


    char line[MAX];
	while(fgets(line, 256, fp))	{
		// printf("%s\n", line);
        char *p = strchr(line, ' ');
        *p = '\0';
        int n = atoi(line);
        *p++ = ' ';
        int cost = atoi(p);
		costTable[n] = cost;
        addNode(node -> graph, n);
    }

    fclose(fp);
    
    
	// socket() and bind() our socket.
    // We will do all sendto()ing and recvfrom()ing on this one.
    
	
	if ((globalSocketUDP = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}



	char myAddr[100];
	struct sockaddr_in bindAddr;
	sprintf(myAddr, "10.1.1.%d", globalMyID);
	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_port = htons(7777);
	inet_pton(AF_INET, myAddr, &bindAddr.sin_addr);
	if (bind(globalSocketUDP, (struct sockaddr*) & bindAddr,
             sizeof(struct sockaddr_in)) < 0) {
		perror("bind");
		close(globalSocketUDP);
		exit(1);
	}

	
    logPtr = fopen(argv[3], "w");
    if (logPtr == NULL) exit(1);
    
	pthread_t announcerThread;
	pthread_create(&announcerThread, 0, announceToNeighbors, (void*)0);
    
    pthread_t checkThread;
    pthread_create(&checkThread, 0, checkNeighbors, (void*) 0);

    
	listenForNeighbors();
	
}


