#ifndef LORANET_H 
#define LORANET_H

#include "Arduino.h"
#include <AESLib.h>

#define MAP_SIZE 20
#define MSG_SIZE 10
#define HDR_DATA 1
#define HDR_ADJ  2


extern AESLib aesLib;
extern char msgbuf[64];

struct Packet {
    uint8_t header = 0;
    uint16_t srcId = 0;
    uint16_t id = 0;
    int16_t temp = 0;
    int16_t humidity = 0;
};

class Map {
public:
    int16_t val[MAP_SIZE];
    uint16_t key[MAP_SIZE];
    uint16_t size = 0;

    int16_t get(uint16_t k);
    void set (uint16_t k, int16_t v);
};

class Lib {
public:  
    static void init ();
    static void test(char* enc, uint16_t len);
    static bool parsePacket(int packetSize, uint8_t* header, uint16_t* srcId, uint16_t* pId);
    static char* encodePacket(Packet* packet, uint16_t* len);
    static char* encodePacketForHTTP(Packet* packet, uint16_t* len);
    static char* constructAdjPkt(uint16_t nodeId, uint16_t packetId, uint16_t* adj, uint16_t* len);
    static void printPacket(Packet* packet);
};

#endif
