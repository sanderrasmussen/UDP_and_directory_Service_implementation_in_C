/* ======================================================================
 * YOU CAN MODIFY THIS FILE.
 * ====================================================================== */
/*
Kandidatnummer : 15140
Fagkode: in2140
*/
#ifndef D2_LOOKUP_MOD_H
#define D2_LOOKUP_MOD_H

#include "d1_udp.h"
#include "d2_lookup.h"

struct D2Client
{
    D1Peer* peer;
};

typedef struct D2Client D2Client;

struct LocalTreeStore
{
    int number_of_nodes;
    struct NetNode *nodes; //this is what i added, buffer containing all the nodes
};

typedef struct LocalTreeStore LocalTreeStore;

#endif /* D2_LOOKUP_MOD_H */

