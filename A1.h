const int kFileNameSize = 50;
const int kIPAddressSize = 15;
const int kPacketTotalSize = sizeof(short);
const int kPacketOrderSize = sizeof(short);
const int kChecksumSize = 32;
const int kFileContentSize = 256 - kFileNameSize - kPacketTotalSize - kPacketOrderSize - kChecksumSize;
const int kPacketSize = kFileNameSize + kPacketTotalSize + kPacketOrderSize + kFileContentSize + kChecksumSize;
const char kInvalidFilenameChars[] = { '\\', '/', ':', '*', '?', '"', '<', '>', '|' };
const char kFileNameDefault[kFileNameSize] = "dataInterchangeFile";
const char kIPAddressDefault[kIPAddressSize] = "127.0.0.1";

enum A1ERROR {
	ARGUMENT_INVALIDNUMBER = 2,
	IPADDRESS_INVALIDFORMAT,
	FILENAME_INVALIDCHARACTERS,
	FILENAME_TOOLONG,
	FILE_CANNOTOPEN
};

void displayHelp();
double getTime();
void packData(unsigned char packet[kPacketSize], char fileName[kFileNameSize], short packetTotal, short packetOrder, unsigned char fileContent[kFileContentSize]);
void unpackData(unsigned char packet[kPacketSize], char fileName[kFileNameSize + 1], unsigned short* packetTotal, unsigned short* packetOrder, unsigned char fileContent[kFileContentSize], char checksum[kChecksumSize + 1]);
void generateChecksum(char checksum[kChecksumSize], unsigned char packet[kPacketSize]);
int compareChecksum(char checksum[kChecksumSize], unsigned char packet[kPacketSize]);