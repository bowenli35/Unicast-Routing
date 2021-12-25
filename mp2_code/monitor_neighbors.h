
#include "link_state_routing.h"


extern FILE *logPtr;
extern int costTable[256];

void hackyBroadcast(const char* buf, int length) {
	for (int i = 0; i < MAX; i++) {
		if(i != globalMyID)  {
			sendto(globalSocketUDP, buf, length, 0,
				  (struct sockaddr*) &globalNodeAddrs[i], sizeof(globalNodeAddrs[i]));
        }
    }
}

void* announceToNeighbors(void* unusedParam) {
	struct timespec sleepFor;
	sleepFor.tv_sec = 0;
	sleepFor.tv_nsec = 300 * 1000 * 1000;
	while(1) {
		hackyBroadcast("HEREIAM", 7);
		nanosleep(&sleepFor, 0);
	}
}

void printNeighbors() {
    for (int i = 0; i < MAX; i++) {
        if (neighborList[i]) {
            printf("%d ", i);
        }
    }
    printf("\n");
}


// checkThread
// check if link failed

void* checkNeighbors() {
    struct timespec sleepFor;
    sleepFor.tv_sec = 0;
	sleepFor.tv_nsec = 500 * 1000 * 1000;
    while (1) {
        struct timeval now;
        gettimeofday (&now, 0);
        for (int i = 0; i < MAX; i++) {
            if (neighborList[i]) {
                if (now.tv_sec - globalLastHeartbeat[i].tv_sec >= 3) {
                    // link failed
                    neighborList[i] = 0;
                    addEdge(node -> graph, globalMyID, i, 0);
                    char linkcut[100];
                    sprintf(linkcut, "&%d:%d@", i, globalMyID);
                    encode(node -> lsa[globalMyID].content, linkcut);
                    sendLSA(node -> lsa[globalMyID], -1);
                    updateFowardingTable();
                }
            }
        }
        nanosleep(&sleepFor, 0);
    }
}



void listenForNeighbors() {
	char fromAddr[100];
	struct sockaddr_in theirAddr;
	socklen_t theirAddrLen;
    char recvBuf[1000];
	int bytesRecvd;
	while(1) {
        memset(recvBuf, 0, 1000);
		theirAddrLen = sizeof(theirAddr);
		if ((bytesRecvd = recvfrom(globalSocketUDP, recvBuf, 1000 , 0, 
					(struct sockaddr*)&theirAddr, &theirAddrLen)) == -1) {
			perror("connectivity listener: recvfrom failed");
			exit(1);
		}
		
		inet_ntop(AF_INET, &theirAddr.sin_addr, fromAddr, 100);
		
		short int heardFrom = -1;
    

		if (strstr(fromAddr, "10.1.1.")) {
			heardFrom = atoi(strchr(strchr(strchr(fromAddr,'.')+1,'.')+1,'.')+1);
            // get one of my neighbor.
            if (neighborList[heardFrom] == 0) {
                neighborList[heardFrom] = 1;
                addNode(node -> graph, heardFrom);
                addEdge(node -> graph, globalMyID, heardFrom, costTable[heardFrom]);
                encode(node -> lsa[globalMyID].content, "");
                sendLSA(node -> lsa[globalMyID], -1);
                sendKnownLsa(heardFrom);
                updateFowardingTable();
            }
			// record that we heard from heardFrom just now.
            gettimeofday (&globalLastHeartbeat[heardFrom], 0);
		}
        
		// Is it a packet from the manager? (see mp2 specification for more details)
		// send format: 'send'<4 ASCII bytes>, destID<net order 2 byte signed>, <some ASCII message>
		if (!strncmp(recvBuf, "send", 4)) {
            char message[101];
            short int destID;
            memcpy(&destID, recvBuf + 4, sizeof(short int));
            strcpy(message, recvBuf + 6);
            destID = ntohs(destID);
            char *logLine = (char*) calloc (1000, sizeof(char));
            if (fowardingTable[destID] == -1) {
                sprintf(logLine, "unreachable dest %d\n", destID);
            } else {
                int nextHop = fowardingTable[destID];
                sprintf(logLine, "sending packet dest %d nexthop %d message %s\n", destID, nextHop, message);
                char *sendBuf = (char*) calloc (1000, sizeof(char));
                sprintf(sendBuf, "forward%d@%s", destID, message);
                sendto(globalSocketUDP, sendBuf, strlen(sendBuf) + 1, 0, 
                    (struct sockaddr*) & globalNodeAddrs[nextHop], sizeof(globalNodeAddrs[nextHop]));
                free(sendBuf);
            }
            fprintf(logPtr, "%s", logLine);
            fflush(logPtr);
            free(logLine);
        }

		//'cost'<4 ASCII bytes>, destID<net order 2 byte signed> newCost<net order 4 byte signed>
		else if (!strncmp(recvBuf, "cost", 4)) {
			// this is the new cost you should treat it as having once it comes back up.)
            int cost;
			short int destID;
            memcpy(&destID, recvBuf + 4, sizeof(short int));
            memcpy(&cost, recvBuf + 6, sizeof(int));
            destID = ntohs(destID);
            cost = ntohl(cost);
            costTable[destID] = cost;
            if (neighborList[destID]) {
                addEdge(node -> graph, globalMyID, destID, cost);
            }
            encode(node -> lsa[globalMyID].content, "");
            sendLSA(node -> lsa[globalMyID], -1);
            updateFowardingTable();
		}
		
		// now check for the various types of packets you use in your own protocol
        else if (!strncmp(recvBuf, "LSA", 3)) {
            // LSA => "LSA1@10&1:1@2:12&1:2@\0"
            // printf("%s\n", recvBuf);
            char *p = strchr(recvBuf + 3, '@');
            *p = '\0';
            int from = atoi(recvBuf + 3);
            *p++ = '@';
            char *q = strchr(p, '&');
            *q = '\0';
            int newSeqNum = atoi(p);
            // fprintf(stdout, "I got one LSA from %d which seq = %d\n", heardFrom, newSeqNum);
            *q++ = '&';
            int seqNum = node -> lsa[from].seqNum;
            if (newSeqNum > seqNum) {
                node -> lsa[from].seqNum = newSeqNum;
                memset(node -> lsa[from].content, 0, 1000);
                strcpy(node -> lsa[from].content, recvBuf);
                sendLSA(node -> lsa[from], heardFrom);
                decode(q, from);
                
                updateFowardingTable();
            }

        } else if (!strncmp(recvBuf, "forward", 7)) {
            char dest[10], message[101];
            char *p = strchr(recvBuf + 7, '@');
            *p = '\0';
            strcpy(dest, recvBuf + 7);
            *p++ = '@';
            strcpy(message, p);
            int destID = atoi(dest);
            char *logLine = (char*) calloc (1000, sizeof(char));
            if (destID == globalMyID) {
                sprintf(logLine, "receive packet message %s\n", message);
            } else if (fowardingTable[destID] == -1) {
                sprintf(logLine, "unreachable dest %d\n", destID);
            } else {
                int nextHop = fowardingTable[destID];
                sprintf(logLine, "forward packet dest %d nexthop %d message %s\n", destID, nextHop, message);
                sendto(globalSocketUDP, recvBuf, strlen(recvBuf) + 1, 0, 
                    (struct sockaddr*) & globalNodeAddrs[nextHop], sizeof(globalNodeAddrs[nextHop]));
            }
            fprintf(logPtr, "%s", logLine);
            fflush(logPtr);
            free(logLine);
        } 
    }
    close(globalSocketUDP);
}

