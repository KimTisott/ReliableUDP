const char kInvalidFilenameChars[] = { '\\', '/', ':', '*', '?', '"', '<', '>', '|' };
const int kUdpHeaderSize = 12;
const int kFileNameSize = 50;
const int kPacketTotalSize = sizeof(short);
const int kPacketOrderSize = sizeof(short);
const int kChecksumSize = 32;
const int kFileContentSize = 384 - kFileNameSize - kPacketTotalSize - kPacketOrderSize - kChecksumSize;
const int kPacketSize = kUdpHeaderSize + kFileNameSize + kPacketTotalSize + kPacketOrderSize + kFileContentSize + kChecksumSize;

enum ERROR {
	ARGUMENT_INVALIDNUMBER = 2,
	IPADDRESS_INVALIDFORMAT,
	FILENAME_INVALIDCHARACTERS,
	FILENAME_TOOLONG,
	FILE_CANNOTOPEN
};

void displayHelp();
void packData(unsigned char packet[kPacketSize], char fileName[kFileNameSize], short packetTotal, short packetOrder, unsigned char fileContent[kFileContentSize]);
void unpackData(unsigned char packet[kPacketSize], char fileName[kFileNameSize + 1], unsigned short* packetTotal, unsigned short* packetOrder, unsigned char fileContent[kFileContentSize], char checksum[kChecksumSize + 1]);
void generateChecksum(char checksum[kChecksumSize], unsigned char packet[kPacketSize]);
int compareChecksum(char checksum[kChecksumSize], unsigned char packet[kPacketSize]);