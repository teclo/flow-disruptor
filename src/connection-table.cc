/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- */
/*
 * Copyright 2014 Teclo Networks AG
 */

#include "connection-table.h"

#include <map>

#include "connection.h"

static void connection_key_for_packet_v4(connection_key_v4* key, Packet* p) {
    uint32_t saddr = p->ipv4().saddr(0);
    uint32_t daddr = p->ipv4().daddr(0);

    /* The key should be the same in both directions, so the
       lower-valued of source or destination {ip,port} is always used as
       the first endpoint. */
    if ((saddr <  daddr) ||
        (saddr == daddr && p->tcp().source_port() < p->tcp().dest_port())) {
        key->addr1 = saddr;;
        key->port1 = p->tcp().source_port();
        key->addr2 = daddr;
        key->port2 = p->tcp().dest_port();
    } else {
        key->addr1 = daddr;
        key->port1 = p->tcp().dest_port();
        key->addr2 = saddr;
        key->port2 = p->tcp().source_port();
    }
}

static void connection_key_for_packet_v6(connection_key_v6* key, Packet* p) {
    const uint32_t* saddr = p->ipv6().saddr().data();
    const uint32_t* daddr = p->ipv6().daddr().data();

    int cmp = memcmp(saddr, daddr, 16);
    if((cmp < 0) ||
       (cmp == 0 && p->tcp().source_port() < p->tcp().dest_port())) {
        memcpy(&key->addr1, saddr, 16);
        key->port1 = p->tcp().source_port();
        memcpy(&key->addr2, daddr, 16);
        key->port2 = p->tcp().dest_port();
        key->pad = 0;
    } else {
        memcpy(&key->addr1, daddr, 16);
        key->port1 = p->tcp().dest_port();
        memcpy(&key->addr2, saddr, 16);
        key->port2 = p->tcp().source_port();
        key->pad = 0;
    }
}

void ConnectionTable::connection_key_for_packet(ConnectionKey* key, Packet* p) {
    assert(p->has_tcp());

    if (p->has_ipv4()) {
        connection_key_for_packet_v4(&key->key_v4, p);
    } else {
        connection_key_for_packet_v6(&key->key_v6, p);
    }
}

template<class T>
struct ConnectionKeyComparator {
    bool operator() (const T& a, const T& b) const {
        return memcmp(&a, &b, sizeof(a)) < 0;
    }
};

class MapConnectionTable : public ConnectionTable {
public:
    ~MapConnectionTable() {
        clear();
    }

    void add_connection_for_packet(Connection* connection,
                                   Packet* p) {
        ConnectionKey* key = connection->key();
        ConnectionTable::connection_key_for_packet(key, p);

        if (p->has_ipv4()) {
            connection_table_v4[key->key_v4] = connection;
        } else {
            connection_table_v6[key->key_v6] = connection;
        }
    }

    Connection* get_connection_for_packet(Packet* p) {
        ConnectionKey key;
        ConnectionTable::connection_key_for_packet(&key, p);
        if (p->has_ipv4()) {
            auto it = connection_table_v4.find(key.key_v4);
            if (it == connection_table_v4.end()) {
                return NULL;
            }
            return it->second;
        } else {
            auto it = connection_table_v6.find(key.key_v6);
            if (it == connection_table_v6.end()) {
                return NULL;
            }
            return it->second;
        }
    }

    void clear() {
        while (!connection_table_v4.empty()) {
            connection_table_v4.begin()->second->close();
        }
        while (!connection_table_v6.empty()) {
            connection_table_v4.begin()->second->close();
        }
    }

    void remove(Connection* connection) {
        if (connection->addr_bytes() == 4) {
            connection_table_v4.erase(connection->key()->key_v4);
        } else {
            connection_table_v6.erase(connection->key()->key_v6);
        }
    }

    size_t size() {
        return connection_table_v4.size() + connection_table_v6.size();
    }

private:
    std::map<connection_key_v4, Connection*, ConnectionKeyComparator<connection_key_v4> > connection_table_v4;
    std::map<connection_key_v6, Connection*, ConnectionKeyComparator<connection_key_v6> > connection_table_v6;
};

ConnectionTable* ConnectionTable::make() {
    return new MapConnectionTable();
}
