#include "Arduino.h";
#include "LoRaNet.h";

#include "AESLib.h"
#include <LoRa.h>;

#define ENCRYPT

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
    aesLib.set_paddingmode((paddingMode)0);
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
uint16_t encrypt(char* msg, uint16_t msgLen) {
    byte iv[N_BLOCK] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
    return aesLib.encrypt((byte*)msg, msgLen, (byte*)ciphertextbuf, aes_key, sizeof(aes_key), iv);
}

uint16_t decrypt(char* msg, uint16_t msgLen) {
    /*
    // Base 64 decoding. TODO
    byte b64decoded[50] = {0};
    base64_decode((char*) b64decoded, msg, msgLen);
    
    Serial.print("b64 decoded, len: ");
    Serial.print(strlen(b64decoded));
    Serial.print(" [");
    for (int i=0; i<strlen(b64decoded); i++) {
        Serial.print((uint8_t) b64decoded[i]);
        Serial.print(", ");
    }
    Serial.println("]");

    byte iv[N_BLOCK] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
    uint16_t olen = aesLib.decrypt(b64decoded, strlen(b64decoded), (byte*)cleartextbuf, aes_key, sizeof(aes_key), iv);

    Serial.print("cleartext, len: ");
    Serial.print(olen);
    Serial.print(" [");
    for (int i=0; i<olen; i++) {
        Serial.print((uint8_t) cleartextbuf[i]);
        Serial.print(", ");
    }
    Serial.println("]");
    return olen;
    */
    byte iv[N_BLOCK] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
    uint16_t olen = aesLib.decrypt((byte*) msg, msgLen, (byte*)cleartextbuf, aes_key, sizeof(aes_key), iv);

    return olen;
}
#endif

void Lib::test (char* buf, uint16_t len) {
    len = decrypt(buf, len);
    memcpy(buf, cleartextbuf, len);
    
    Serial.print(" Recv MSG, size: ");
    Serial.print(len);
    Serial.print(": [");
    for (int i=0; i<len; i++) {
        Serial.print((uint8_t) buf[i]);
        Serial.print(", ");
    }
    Serial.println("]");

    Packet packet = Packet();
    int bufCnt = 0;
    packet.header = extractAsInt(buf, bufCnt, 1);    bufCnt += 1;
    packet.srcId = extractAsInt(buf, bufCnt, 2);     bufCnt += 2;
    packet.id = extractAsInt(buf, bufCnt, 2);        bufCnt += 2;
    packet.temp = extractAsInt(buf, bufCnt, 1);      bufCnt += 1;
    packet.humidity = extractAsInt(buf, bufCnt, 1);  bufCnt += 1;

    Lib::printPacket(&packet);
}

// Parses packet from raw string. Returns null if invalid format.
bool Lib::parsePacket(int packetSize, uint8_t* header, uint16_t* srcId, uint16_t* pId) { 
    // Parse message into args array, without using 'String' objects.
    // Format: "{(u8)header}{(u16)srcId}{(u16)id}{(i8)temp}{(u8)humidity}";
    
    uint16_t len = 0;
    char buf[packetSize] = {0};
    for (int i=0; LoRa.available() && i<packetSize; i++) {
        char c = (char) LoRa.read();
        buf[len++] = c;
    }
    memcpy(msgbuf, buf, len);
#ifdef ENCRYPT 
    len = decrypt(buf, len);
    len = 16;
    memcpy(buf, cleartextbuf, len);
#endif
     
    Serial.print("Recv MSG: [");
    for (int i=0; i<len; i++) {
        Serial.print((uint8_t) buf[i]);
        Serial.print(", ");
    }
    Serial.println("] ");
     
    
    int bufCnt = 0;
    *header = extractAsInt(buf, bufCnt, 1);    bufCnt += 1;
  
    if (*header > 2) {
        Serial.println("Invalid header.");
        return false;
    }
    *srcId = extractAsInt(buf, bufCnt, 2);     bufCnt += 2;
    *pId = extractAsInt(buf, bufCnt, 2);       bufCnt += 2;
    return true;
}

int writeIntAt (char* buf, int cnt, int val, int len) {
  uint8_t mask = ((1 << 8) - 1);
  for (int i=0; i<len; i++) {
    uint8_t c = val & mask;
    val >>= 8;

    buf[cnt + (len - i - 1)] = c;
  }
  return len;
}

char* Lib::encodePacket (Packet* packet, uint16_t* len) {
    if (packet == nullptr) return nullptr;

    char buf[16] = {0}; 
    uint16_t cnt = 0;
    cnt += writeIntAt(buf, cnt, HDR_DATA, 1);
    cnt += writeIntAt(buf, cnt, packet->srcId, 2);    
    cnt += writeIntAt(buf, cnt, packet->id, 2);        
    cnt += writeIntAt(buf, cnt, packet->temp, 1);      
    cnt += writeIntAt(buf, cnt, packet->humidity, 1); 
    cnt = sizeof(buf);

    /*
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
     */
#ifdef ENCRYPT
    *len = encrypt(buf, cnt);
    char* out = (char*) malloc(*len);
    memcpy(out, ciphertextbuf, *len);
    return out;
#else 
    *len = sizeof(buf);
    char* out = (char*) malloc(*len);
    memcpy(out, buf, *len);
    return out;
#endif
}

char* Lib::encodePacketForHTTP (Packet* packet, uint16_t* len) {
    if (packet == nullptr) return nullptr;
    char* buf = (char*) malloc(MSG_SIZE);
    int cnt = 0;
    cnt += sprintf(buf+cnt, "%d/", packet->header);
    cnt += sprintf(buf+cnt, "%d/%d/", packet->srcId, packet->id);
    cnt += sprintf(buf+cnt, "%d/%d/\0", packet->temp, packet->humidity);
    *len = cnt;
    return buf;

}

char* Lib::constructAdjPkt (uint16_t nodeId, uint16_t packetId, uint16_t* adj, uint16_t* len) {
    if (adj == nullptr) return nullptr;
    char* buf = (char*) malloc(16);
    
    uint16_t cnt = 0;
    cnt += writeIntAt(buf, cnt, HDR_ADJ, 1); 
    cnt += writeIntAt(buf, cnt, nodeId, 2);
    cnt += writeIntAt(buf, cnt, packetId, 2);
    for (int i=0; i<*len; i++) {
        cnt += writeIntAt(buf, cnt, adj[i], 2);
    }
    /*
    Serial.print("Encoding ADJ PKT: ");
    Serial.print(" sized ");
    Serial.print(cnt);
    Serial.print(" as: [ ");
    for (int i=0; i<cnt; i++) {
      Serial.print((uint8_t) buf[i]);
      Serial.print(", ");
    }
    Serial.println("] ");
    */
#ifdef ENCRYPT
    *len = encrypt(buf, cnt);
    char* out = (char*) malloc(*len);
    memcpy(out, ciphertextbuf, *len);
    return out;
#else 
    *len = sizeof(buf);
    char* out = (char*) malloc(*len);
    memcpy(out, buf, *len);
    return out;
#endif
}

void Lib::printPacket (Packet* packet) {
    if (packet == nullptr) return;
    char buf[20];
    int cnt = 0;
    cnt += sprintf(buf+cnt, "h=%d:", packet->header);
    cnt += sprintf(buf+cnt, "%d/%d/", packet->srcId, packet->id);
    cnt += sprintf(buf+cnt, "%d/%d/\0", packet->temp, packet->humidity);
    Serial.print((char*) buf);
}

