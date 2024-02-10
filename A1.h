/*
 FILE: A1.h
 PROJECT: SENG2040 - Assignment #1
 PROGRAMMER: Quan Khuong - Kim Tisott
 FIRST VERSION: February 05 2024
 DESCRIPTION: The file include all the constant, function prototype and enum.
 */


const int kFileNameSize = 50;
const int kIPAddressSize = 15;
const int kPacketTotalSize = sizeof(short);
const int kPacketOrderSize = sizeof(short);
const int kChecksumSize = 32;

// Remaining space for reading buffer
const int kFileContentSize = 256 - kFileNameSize - kPacketTotalSize - kPacketOrderSize - kChecksumSize;

// Total size of constructed packet
const int kPacketSize = kFileNameSize + kPacketTotalSize + kPacketOrderSize + kFileContentSize + kChecksumSize;
const char kInvalidFilenameChars[] = { '\\', '/', ':', '*', '?', '"', '<', '>', '|' };

// Default name and address
const char kFileNameDefault[kFileNameSize] = "dataInterchangeFile";
const char kIPAddressDefault[kIPAddressSize] = "127.0.0.1";

enum A1ERROR {
	ARGUMENT_INVALIDNUMBER = 2,
	IPADDRESS_INVALIDFORMAT,
	FILENAME_INVALIDCHARACTERS,
	FILENAME_TOOLONG,
	FILE_CANNOTOPEN
};


// Function prototypes
void displayHelp();
double getTime();
void packData(unsigned char packet[kPacketSize], char fileName[kFileNameSize], short packetTotal, short packetOrder, unsigned char fileContent[kFileContentSize]);
void unpackData(unsigned char packet[kPacketSize], char fileName[kFileNameSize + 1], unsigned short* packetTotal, unsigned short* packetOrder, unsigned char fileContent[kFileContentSize], char checksum[kChecksumSize + 1]);
void generateChecksum(char checksum[kChecksumSize], unsigned char packet[kPacketSize]);
int compareChecksum(char checksum[kChecksumSize], unsigned char packet[kPacketSize]);