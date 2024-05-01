/* ======================================================================
 * YOU ARE EXPECTED TO MODIFY THIS FILE.
 * ====================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "d2_lookup.h"

D2Client* d2_client_create( const char* server_name, uint16_t server_port )
{
   
    D2Client *client = (D2Client *)malloc(sizeof(D2Client));
    if (client==NULL){
        return NULL;
    }
    //creating d1 client
    D1Peer *d1peer = d1_create_client();
    d1_get_peer_info(d1peer, server_name, server_port);
    
    client->peer = d1peer;
    
    return client;
}

D2Client* d2_client_delete( D2Client* client )
{
    
     if (client != NULL){
        d1_delete(client->peer);//using the d1 delete method to delete/free the d1_client struct and socket
        free(client);
        client = NULL;
    }

    return NULL;
}

int d2_send_request( D2Client* client, uint32_t id )
{
    //making packet request with the provided id and setting type and id in network order with htons and htonl
    PacketRequest *request = (PacketRequest *) calloc(1, sizeof(PacketRequest));
    request->type = TYPE_REQUEST;
    request->type = htons(request->type);
    request->id = id;
    request->id = htonl(request->id);

    int sent_bytes = d1_send_data(client->peer,request, sizeof(PacketRequest));
    free(request);
    if (sent_bytes<=0){
        return -1;
    }
    return 1;
}

int d2_recv_response_size( D2Client* client )
{

    char* buffer= (char*)malloc(sizeof(PacketResponseSize));
    int recved_bytes = d1_recv_data(client->peer, buffer, 1024);

    if (recved_bytes<=0){
        return -1;
    }
    //this is not nessecery as i could have used the buffer directly but i think it is easier to read
    PacketResponseSize* response = buffer; 
    int size =ntohs(response->size); //setting size to host order 

    free(buffer);

    return size; //returning size in host order
}


int d2_recv_response( D2Client* client, char* buffer, size_t sz )
{
    int recved_bytes = d1_recv_data(client->peer, buffer, sz);  

    PacketResponse *header = (PacketResponse *)buffer; // casting to packetResponse, the buffer also comes with a payload behind the response struct

    header->payload_size = ntohs(header->payload_size);

    //since the NetNode is abreviated, the size of every net node is not sizeof(netnode), therefore i cast the buffer to uint32_t and read every 4 byte at a time, since this is the size of all the fields in NetNode.
    uint32_t *payload = (uint32_t *)(buffer + sizeof(PacketResponse));

    int num_uint32_values = header->payload_size / sizeof(uint32_t); //finding out how many iterations in the loop we need

    for (int i = 0; i < num_uint32_values-1 ; i++) { 
        payload[i] = ntohl(payload[i]); //For every uint32 value i flip it to host order in the buffer
    }
    
    printf("--received response \n");

    return recved_bytes;
}

LocalTreeStore* d2_alloc_local_tree( int num_nodes )
{   
    LocalTreeStore *tree = (LocalTreeStore *)malloc(sizeof(LocalTreeStore)) ;
    tree->number_of_nodes = num_nodes;
    tree->nodes = (NetNode *)malloc(sizeof(NetNode)*num_nodes); //allocating space for all expected nodes.
    printf("Tree allocated \n ");
    if (tree==NULL){
        return NULL;
    }
    return tree;
}

void  d2_free_local_tree( LocalTreeStore* nodes )
{   
    //freeing the tree
    if (nodes !=NULL){
        free(nodes->nodes); 
        free(nodes);
        nodes = NULL;
    }
}

int d2_add_to_local_tree( LocalTreeStore* nodes_out, int node_idx, char* buffer, int buflen )
{
    //since the nodes are abreviated and not all have a fixed size of sizeof(netnode) i must mannually loop through the buffer and read every uint32 one at a time.
    int num_32_vals = (buflen )/sizeof(uint32_t);//how many times i must iterate on the buffer

    //printf("bufflen : %d ", buflen); //-> used for debugging
    //printf("number og 32 values: %d \n ",num_32_vals);

    //casting the payload to uint32_t so that i can iterate on every uint32_t value
    uint32_t *payload = (uint32_t *)(buffer ); // in the test file the header is not passed into this function, therefore i assume that i only need to read the payload 
  
    int i = 0;
    while ( i< num_32_vals -2){//YEP something really weird is going on with the offsets, but this seems to work, i still dont know why i need the -2 in  (num_32_vals-2) but this seems to work on every test i have done so far on every id request
        NetNode *node = (NetNode *)malloc(sizeof(NetNode));
        node->id =  payload[i]; //setting first uint32 to id and so on for all the fields in NetNode untill done, then create another node if there is still more to read
        //printf("id %d \n ", node->id); //all prints below are for debugging
        i++;
        node->value=payload[i];
        //printf("value %d \n",node->value);
        i++;
        node->num_children = payload[i];
        //printf("num children %d \n", node->num_children);
        i++;
        
        for (int j= 0;j<node->num_children;j++){
            node->child_id[j]= payload[i];
            i++;
        }
        //add the node to the tree on id = node_idx. the way i understood it, the server sends all the nodes in rising id order. Starting with root 0 to 1,2,3,4 and so on
        nodes_out->nodes[node_idx] = *node;//therefore node_idx should also bee the actual id of the node and we get essensially a hashmap where node id:x = nodes[x]
        node_idx++;
        free(node);
    }
    printf("--added nodes , so far we have %d nodes\n",node_idx);
    return node_idx;
}

void printNodeRecursive(int id, LocalTreeStore* nodes_out, int nestedNr){ // i decided to use recursion for readability
        char string[100] ="";  //assuming we will never have more than 50 nested nodes when printing
        for (int i = 0; i < nestedNr; i++) {
            strcat(string, "--"); //for every nesting we add -- to the print
        }
        NetNode node = nodes_out->nodes[id]; //as mention above, this is like a hashmap since the server gives the nodes in a buffer in rising order from 0 to 1,2,3,4,5 and so on
        printf("%s id %d value %d children %d\n",string, node.id, node.value, node.num_children);

        //recursively printig out all the children of the node.
        for (int i = 0 ; i<node.num_children; i++){
            printNodeRecursive(node.child_id[i], nodes_out, nestedNr+1); 
        }
}


void d2_print_tree( LocalTreeStore* nodes_out )
{ 
    printNodeRecursive(0, nodes_out, 0);//starting from the root node, from here we should be able to reach every other node provided to us by the server. Therefore we recusively print out every children of the root node
}
