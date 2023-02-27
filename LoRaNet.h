#ifndef LORANET_H 
#define LORANET_H

#include "Arduino.h"
#include <AESLib.h>

#define MAP_SIZE 20
#define ADJ_LIST_SIZE 10
#define MSG_SIZE 10
#define HDR_DATA 1
#define HDR_ADJ  2

#define EEPROM_ADDR_ID 0
#define EEPROM_ADDR_BLACKLIST 2

extern char msgbuf[64];
extern char dec_msgbuf[64];
extern uint16_t msglen;

struct Packet {
    uint8_t header = 0;
    uint16_t srcId = 0;
    uint16_t prevId = 0;
    uint16_t id = 0;
    int8_t temp = 0;
    int8_t humidity = 0;
    int16_t mq2 = 0;
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
    static bool parsePacket(int packetSize, uint8_t* header, uint16_t* srcId, uint16_t* prevId, uint16_t* pId, uint16_t* olen);
	static char* getForwardBuf(uint16_t nodeId, uint16_t len);
    static char* encodePacket(Packet* packet, uint16_t* len);
    static char* encryptBuf(char* buf, uint16_t len, uint16_t* olen);
    static char* constructAdjPkt(uint16_t nodeId, uint16_t packetId, uint16_t* adj, uint16_t* len, bool gateway = false);
    static void printPacket(Packet* packet);
};

#endif
