# LoRaNet Library
### Custom Library used to build the LoRaNet sensor
---
# Main repository: 
Refer here for details regarding the LoRaNet project: [LoRaNet](https://github.com/beranki/loRAFire/)

# API:
### Packet:
A structure specifying the fields in a data packet. Check source files for details.
### Map: 
A naive implementation of a key-value data structure. Check source files for details.
## Lib:
- `init()`: Initializes the library, cipher, etc.
- `parsePacket()`: Parses the packets, extracting the header.
- `getForwardBuf()`: Returns the packet as bytes for fowarding.
- `encodePacket()`: Given a packet object, encodes & encrypts into a raw bytes.
- `constructAdjPkt()`: Constructs an adjacency packet for a given list and node.
- `printPacket()`: Prints a given packet in a set format.
