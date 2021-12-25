
#ifndef link_state_routing_h
#define link_state_routing_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

#include "Dijkstra.h"

extern int globalMyID;
//in order to realize when a neighbor has gotten cut off from you.
extern struct timeval globalLastHeartbeat[256];
//our all-purpose UDP socket, to be bound to 10.1.1.globalMyID, port 7777
extern int globalSocketUDP;
//pre-filled for sending to 10.1.1.0 - 255, port 7777
extern struct sockaddr_in globalNodeAddrs[256];

extern int neighborList[256];

extern int fowardingTable[256];

typedef struct _lsa {
    int from;
    int seqNum;
    char *content;  // "LSA1@0&1:1@2:12@"   
    // LSA_FROM_@_SEQNUM_&_ID_:_COST@
} LSA;


struct _node {
    int idx;
    LSA *lsa;
    Graph graph;
};

typedef struct _node *Node;

extern Node node;

Node initNode(int idx) {
    Node node = (Node) calloc (1, sizeof(struct _node));
    node -> idx = idx;
    node -> lsa = (LSA*) calloc (MAX, sizeof(struct _lsa));
    for (int i = 0; i < MAX; i++) {
        node -> lsa[i].from = i;
        node -> lsa[i].seqNum = -1;
        node -> lsa[i].content = (char*) calloc (1000, sizeof(char));
    }
    node -> graph = initGraph();
    addNode(node -> graph, node -> idx);
    return node;
}


void delNode(Node node) {
    delGraph(node -> graph);
    free(node -> lsa -> content);
    free(node -> lsa);
    free(node);
}

void encode(char *sendBuf, char *linkcut) {
    // id:cost  "LSA1@0&1:1@2:12@"
    memset(sendBuf, 0, 1000);
    node -> lsa[globalMyID].seqNum++;
    sprintf(sendBuf, "LSA%d@%d&", globalMyID, node -> lsa[globalMyID].seqNum);
    for (int i = 0; i < MAX; i++) {
        if (neighborList[i]) {
            char edge[100];
            sprintf(edge, "%d:%d@", i, node -> graph -> adjacency[globalMyID][i]);
            strcat(sendBuf, edge);
        }
    }
    strcat(sendBuf, linkcut);
}

void decode(char *q, int from) {
    // LSA => "LSA1@10&1:1@2:12@&1:2@\0"
    // q => 1:1@2:12@&1:2@\0
    if (q == NULL) return;
    while (*q != '\0') {
        char *p = strchr(q, ':');
        *p = '\0';
        int n = atoi(q);
        addNode(node -> graph, n);
        *p++ = ':';
        q = strchr(q, '@');
        *q = '\0';
        addEdge(node -> graph, n, from, atoi(p));
        *q++ = '@';
        if (*q == '&') {
            q++;
            break;
        }
    }
    while (*q != '\0') {
        char *p = strchr(q, ':');
        *p = '\0';
        int nodeA = atoi(q);
        *p++ = ':';
        q = strchr(q, '@');
        *q = '\0';
        int nodeB = atoi(p);
        addEdge(node -> graph, nodeA, nodeB, 0);
        *q++ = '@';
    }
}


void updateFowardingTable() {
    int predecessor[MAX];
    for (int i = 0; i < MAX; i++) {
        predecessor[i] = -1; // all null
    }
    dijkstra(node -> graph, globalMyID, predecessor);
    for (int i = 0; i < MAX; i++) {
        if (i == globalMyID) {
            fowardingTable[globalMyID] = globalMyID;
            continue;
        }
        if (predecessor[i] == -1) {
            fowardingTable[i] = -1; // unreachable
            continue;
        } 
        int nextHop = predecessor[i];
        if (nextHop == globalMyID) {
            fowardingTable[i] = i;
            continue;
        }
        int flag = 0;
        while (predecessor[nextHop] != globalMyID) {
            flag++;
            nextHop = predecessor[nextHop];
            if (flag > 256) {
                break;
            }
        }
        if (flag <= 256) fowardingTable[i] = nextHop;
    }

}

void sendKnownLsa(int newNeighbor) {
    for (int i = 0; i < MAX; i++) {
        LSA lsa = node -> lsa[i];
        if (lsa.from != globalMyID && lsa.seqNum != -1 && i != newNeighbor) {
            sendto(globalSocketUDP, lsa.content, strlen(lsa.content) + 1, 0, 
            (struct sockaddr*) & globalNodeAddrs[newNeighbor], sizeof(globalNodeAddrs[newNeighbor]));
        }
    }
}



void sendLSA(LSA lsa, int idx) {
    char *sendBuf = lsa.content;
    for (int i = 0; i < MAX; i++) {
        if (i != globalMyID && neighborList[i] && i != idx) {
            sendto(globalSocketUDP, sendBuf, strlen(sendBuf) + 1, 0, 
            (struct sockaddr*) & globalNodeAddrs[i], sizeof(globalNodeAddrs[i]));
        }
    }
}


void printTable() {
    for (int i = 0; i < MAX; i++) {
        fprintf(stdout, "%d -> %d\n", i, fowardingTable[i]);
    }
}

#endif /* link_state_routing_h */
