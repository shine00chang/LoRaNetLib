#include "Arduino.h";
#include "LoRaNet.h";

#include "AESLib.h"
#include <LoRa.h>;

//#define ENCRYPT

#ifdef ENCRYPT
AESLib aesLib;
byte aes_key[] = { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C };
char cleartextbuf[64] = {0};
char ciphertextbuf[64] = {0};
#endif

char msgbuf[64] = {0};

int16_t Map::get (uint16_t k) {
    int i = 0;
    while (i < size && key[i] != k) i++;
    return i == size ? -1 : val[i];
};

void Map::set (uint16_t k, int16_t v) {
    int i = 0;
    while (i < size && key[i] != k) i++;

    if (i == size) {
      if (size == sizeof(key)) 
        return;
      key[size++] = k;
    }
    val[i] = v;
}; 

void Lib::init () {
#ifdef ENCRYPT
	aesLib.set_paddingmode((paddingMode)0);
#endif
}

uint16_t extractAsInt (char* buf, int index, int len) {
  uint16_t out = 0;
  for (int i=0; i<len; i++) {
    uint8_t val = buf[index + i];
    out = (out << 8) + val;
  }
  return out;
}

#ifdef ENCRYPT
uint16_t _encrypt(char* msg, uint16_t msgLen) {
    byte iv[N_BLOCK] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
    return aesLib.encrypt((byte*)msg, msgLen, (byte*)ciphertextbuf, aes_key, sizeof(aes_key), iv);
}

uint16_t decrypt(char* msg, uint16_t msgLen) {
   byte iv[N_BLOCK] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
    uint16_t olen = aesLib.decrypt((byte*) msg, msgLen, (byte*)cleartextbuf, aes_key, sizeof(aes_key), iv);

    return olen;
}
#endif

// Parses packet from raw string. Returns null if invalid format.
bool Lib::parsePacket(int packetSize, uint8_t* header, uint16_t* srcId, uint16_t* prevId, uint16_t* pId, uint16_t* olen) { 
    // Parse message into args array, without using 'String' objects.
    // Format: "{(u8)header}{(u16)srcId}{(u16)id}{(i8)temp}{(u8)humidity}{(u16)mq2}";
    
    uint16_t len = 0;
    char buf[packetSize] = {0};
    for (int i=0; LoRa.available() && i<packetSize; i++) {
        char c = (char) LoRa.read();
        buf[len++] = c;
    }
#ifdef ENCRYPT 
    *olen = decrypt(buf, len);
    memcpy(msgbuf, cleartextbuf, *olen);
#else 
	*olen = len;
    memcpy(msgbuf, buf, len);
#endif
	     
    Serial.print("Recv MSG: [");
    for (int i=0; i<*olen; i++) {
        Serial.print((uint8_t) buf[i]);
        Serial.print(", ");
    }
    Serial.println("] ");
      
    
    int bufCnt = 0;
    *header = extractAsInt(msgbuf, bufCnt, 1);    bufCnt += 1;
  
    if (*header > 2) {
        Serial.println("Invalid header.");
        return false;
    }
    *srcId = extractAsInt(msgbuf, bufCnt, 2);     bufCnt += 2;
    *prevId = extractAsInt(msgbuf, bufCnt, 2);    bufCnt += 2;
    *pId = extractAsInt(msgbuf, bufCnt, 2);       bufCnt += 2;
    return true;
}

int writeIntAt (char* buf, int cnt, uint16_t val, int len) {
  uint8_t mask = ((1 << 8) - 1);
  
  for (int i=0; i<len; i++) {
    uint8_t c = val & mask;
    val >>= 8;
    buf[cnt + (len - i - 1)] = c;
  }
  return len;
}

char* Lib::getForwardBuf (uint16_t nodeId, uint16_t len) {
	writeIntAt(msgbuf, 3, nodeId, 2);
	uint16_t t;
	return Lib::encryptBuf(msgbuf, len, &t);
}
char* Lib::encodePacket (Packet* packet, uint16_t* len) {
    if (packet == nullptr) return nullptr;
    packet->header = HDR_DATA;

    char buf[16] = {0}; 
    uint16_t cnt = 0;
    cnt += writeIntAt(buf, cnt, packet->header, 1);
    cnt += writeIntAt(buf, cnt, packet->srcId, 2);    
    cnt += writeIntAt(buf, cnt, packet->prevId, 2);    
    cnt += writeIntAt(buf, cnt, packet->id, 2);        
    cnt += writeIntAt(buf, cnt, packet->temp, 1);      
    cnt += writeIntAt(buf, cnt, packet->humidity, 1); 
    cnt += writeIntAt(buf, cnt, packet->mq2, 2); 
    cnt = sizeof(buf);
    
    Serial.print("Encoding MSG: ");
    Lib::printPacket(packet);
    Serial.print(" sized ");
    Serial.print(cnt);
    Serial.print(" as: [ ");
    for (int i=0; i<cnt; i++) {
        Serial.print((uint8_t) buf[i]);
        Serial.print(", ");
    }
    Serial.println("] ");

	return Lib::encryptBuf(buf, cnt, len);
}

char* Lib::encryptBuf (char* buf, uint16_t len, uint16_t* olen) {
#ifdef ENCRYPT
    *olen = _encrypt(buf, len);
    char* out = (char*) malloc(*olen);
    memcpy(out, ciphertextbuf, *olen);
    return out;
#else 
    *olen = 16;
	char* out = (char*) malloc(*olen);
    memcpy(out, buf, *olen);
    return out;
#endif
}


char* Lib::constructAdjPkt (uint16_t nodeId, uint16_t packetId, uint16_t* adj, uint16_t* len, bool gateway) {
    if (adj == nullptr) return nullptr;
    char* buf = (char*) malloc(16);
    memset(buf, 0, 16);

    uint16_t cnt = 0;
    cnt += writeIntAt(buf, cnt, HDR_ADJ, 1); 
    cnt += writeIntAt(buf, cnt, nodeId, 2);
    cnt += writeIntAt(buf, cnt, nodeId, 2);
    cnt += writeIntAt(buf, cnt, packetId, 2);
    for (int i=0; i<*len; i++) {
        cnt += writeIntAt(buf, cnt, adj[i], 2);
    }
    
    Serial.print("Encoding ADJ PKT. list:[");
    for (int i=0; i<*len; i++) {
        Serial.print(adj[i]);
        Serial.print(", ");
    }
    Serial.print("]");
    Serial.print(", sized ");
    Serial.print(cnt);
    Serial.print(" as: [ ");
    for (int i=0; i<cnt; i++) {
      Serial.print((uint8_t) buf[i]);
      Serial.print(", ");
    }
    Serial.print("] ");
    
    if (gateway) {
        *len = cnt;
        return buf;
    }

#ifdef ENCRYPT
    *len = _encrypt(buf, 16);
    char* out = (char*) malloc(*len);
    memcpy(out, ciphertextbuf, *len);
    
    free(buf);
    return out;
#else 
    *len = cnt;
    return buf;
#endif
}

void Lib::printPacket (Packet* packet) {
    if (packet == nullptr) return;
    Serial.print("h=");
    Serial.print(packet->header);
    Serial.print(":");
    Serial.print(packet->srcId);
    Serial.print("/");
	Serial.print(packet->prevId);
    Serial.print("/");
    Serial.print(packet->id);
    Serial.print("/");
    Serial.print(packet->temp);
    Serial.print("/");
    Serial.print(packet->humidity);
    Serial.print("/");
    Serial.print(packet->mq2);
}

