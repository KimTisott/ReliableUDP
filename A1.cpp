#include <stdlib.h>
#include <string.h>
#include "ReliableUDP.h"
#include "hash-library/md5.h"
#include "hash-library/md5.cpp"

#pragma warning(disable : 4996)

const char* kPacketDelimiter = "|";
const int kNumericBase = 10;


void displayHelp()
{
    printf(" Usage: \n");
    printf(" ReliableUDP [INPUT_FILE] [IPADDRESS] [-h]\n");
    printf(" INPUT_FILE     :       FILENAME is the INPUT file name \n");
    printf(" IPADDRESS      :       IP ADDRESS in IPv4 format (x.x.x.x) \n");
    printf(" -h             :       Display help\n");
}

static char* packData(char *fileName, short packetTotal, short packetOrder, char *fileContent)
{
    char packet[kPacketSize];

    strcpy(packet, fileName);

    strcpy(packet, "|");

    char totalString[sizeof(short)];
    itoa(packetTotal, totalString, kNumericBase);
    strcpy(packet, totalString);

    strcpy(packet, "|");

    char orderString[sizeof(short)];
    itoa(packetOrder, orderString, kNumericBase);
    strcpy(packet, orderString);

    strcpy(packet, "|");

    strcpy(packet, fileContent);

    return packet;
}

static void unpackData(char *packet, char *fileName, int packetTotal, int packetOrder, char *fileContent)
{
    char* token = NULL;
    for (int i = 0; i < 4; i++)
    {
        token = strtok(packet, kPacketDelimiter);
        if (token != NULL)
        {
            switch (i)
            {
                case 0:
                {
                    strcpy(fileName, token);
                }
                case 1:
                {
                    packetTotal = atoi(token);
                }
                case 2:
                {
                    packetOrder = atoi(token);
                }
                case 3:
                {
                    strcpy(fileContent, token);
                }
            }
        }
    }
}
void packData(unsigned char packet[kPacketSize], char fileName[kFileNameSize], short packetTotal, short packetOrder, unsigned char fileContent[kFileContentSize])
{
    memcpy(packet + kUdpHeaderSize, fileName, kFileNameSize);
    memcpy(packet + kUdpHeaderSize + kFileNameSize, &packetTotal, kPacketTotalSize);
    memcpy(packet + kUdpHeaderSize + kFileNameSize + kPacketTotalSize, &packetOrder, kPacketOrderSize);
    memcpy(packet + kUdpHeaderSize + kFileNameSize + kPacketTotalSize + kPacketOrderSize, fileContent, kFileContentSize);
    char checksum[kChecksumSize + 1];
    generateChecksum(checksum, packet);
    checksum[kChecksumSize] = '\0';
    printf("Checksum: %s", checksum);
    memcpy(packet + kUdpHeaderSize + kFileNameSize + kPacketTotalSize + kPacketOrderSize + kFileContentSize, checksum, kChecksumSize);
}

void unpackData(unsigned char packet[kPacketSize], char fileName[kFileNameSize + 1], unsigned short* packetTotal, unsigned short* packetOrder, unsigned char fileContent[kFileContentSize], char checksum[kChecksumSize + 1])
{
    memcpy(fileName, packet + kUdpHeaderSize, kFileNameSize);
    fileName[kFileNameSize] = '\0';
    *packetTotal = packet[kUdpHeaderSize + kFileNameSize];
    *packetTotal += packet[kUdpHeaderSize + kFileNameSize + 1] << 8;
    *packetOrder = packet[kUdpHeaderSize + kFileNameSize + kPacketTotalSize];
    *packetOrder += packet[kUdpHeaderSize + kFileNameSize + kPacketTotalSize + 1] << 8;
    memcpy(fileContent, packet + kUdpHeaderSize + kFileNameSize + kPacketTotalSize + kPacketOrderSize, kFileContentSize);
    memcpy(checksum, packet + kUdpHeaderSize + kFileNameSize + kPacketTotalSize + kPacketOrderSize + kFileContentSize, kChecksumSize);
    checksum[kChecksumSize] = '\0';
}

void generateChecksum(char checksum[kChecksumSize], unsigned char packet[kPacketSize])
{
    MD5 md5;
    md5.add(packet, kPacketSize - kChecksumSize);
    memcpy(checksum, md5.getHash().c_str(), kChecksumSize);
}

int compareChecksum(char checksum[kChecksumSize], unsigned char packet[kPacketSize])
{
    char generatedChecksum[kChecksumSize];
    generateChecksum(generatedChecksum, packet);

    for (int i = 0; i < kChecksumSize; i++)
    {
        if (checksum[i] != generatedChecksum[i])
        {
            return 0;
        }
    }

    return 1;
}