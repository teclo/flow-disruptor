/* -*- mode: c++; c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2011 Teclo Networks AG
 */

#ifndef _CONNECTION_TABLE_H_
#define _CONNECTION_TABLE_H_

#include <stdint.h>
#include <vector>

#include "packet.h"

// Keys for mapping connection 5-tuples to socket table entries.

struct connection_key_v4 {
    /* all values are stored in network byte order */
    uint32_t addr1;                     /* Lower-numbered IP */
    uint32_t addr2;                     /* Higher-numbered IP */
    uint16_t port1;                     /* Lower-numbered port */
    uint16_t port2;                     /* Higher-numbered port */
} __attribute__((packed));

struct connection_key_v6 {
    /* all values are stored in network byte order */
    uint32_t addr1[4];                     /* Lower-numbered IP */
    uint32_t addr2[4];                     /* Higher-numbered IP */
    uint16_t port1;                        /* Lower-numbered port */
    uint16_t port2;                        /* Higher-numbered port */
    uint32_t pad;
} __attribute__((packed));

union ConnectionKey {
    connection_key_v4 key_v4;
    connection_key_v6 key_v6;
};

class Connection;

class ConnectionTable {
public:
    // Allocate a new socket table
    static ConnectionTable* make();

    virtual ~ConnectionTable() {}

    // Number of active entries in the table.
    virtual size_t size() = 0;

    // Add a new entry to the socket table, for the specified SYN packet.
    virtual void add_connection_for_packet(Connection* connection,
                                           Packet* p) = 0;
    // Get the connection matching this packet, or NULL if there is none.
    virtual Connection* get_connection_for_packet(Packet* p) = 0;

    // Clear the table, and deallocate all connections.
    virtual void clear() = 0;
    // Remove a single connection from the table.
    virtual void remove(Connection*) = 0;

    // Fill in a table key for this packet.
    static void connection_key_for_packet(ConnectionKey* key, Packet* p);

protected:
    ConnectionTable() {}
};

#endif	/* _CONNECTION_TABLE_H_ */
