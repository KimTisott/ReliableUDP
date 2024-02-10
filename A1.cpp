#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "A1.h"
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

void packData(unsigned char packet[kPacketSize], char fileName[kFileNameSize+1], short packetTotal, short packetOrder, unsigned char fileContent[kFileContentSize])
{
    memcpy(packet, fileName, kFileNameSize);
    memcpy(packet + kFileNameSize, &packetTotal, kPacketTotalSize);
    memcpy(packet + kFileNameSize + kPacketTotalSize, &packetOrder, kPacketOrderSize);
    memcpy(packet + kFileNameSize + kPacketTotalSize + kPacketOrderSize, fileContent, kFileContentSize);
    char checksum[kChecksumSize + 1] = {};
    generateChecksum(checksum, packet);
    checksum[kChecksumSize] = '\0';
    memcpy(packet + kFileNameSize + kPacketTotalSize + kPacketOrderSize + kFileContentSize, checksum, kChecksumSize);
}

void unpackData(unsigned char packet[kPacketSize], char fileName[kFileNameSize + 1], unsigned short* packetTotal, unsigned short* packetOrder, unsigned char fileContent[kFileContentSize], char checksum[kChecksumSize + 1])
{
    memcpy(fileName, packet, kFileNameSize);
    fileName[kFileNameSize] = '\0';
    *packetTotal = packet[kFileNameSize];
    *packetTotal += packet[kFileNameSize + 1] << 8;
    *packetOrder = packet[kFileNameSize + kPacketTotalSize];
    *packetOrder += packet[kFileNameSize + kPacketTotalSize + 1] << 8;
    memcpy(fileContent, packet + kFileNameSize + kPacketTotalSize + kPacketOrderSize, kFileContentSize);
    memcpy(checksum, packet + kFileNameSize + kPacketTotalSize + kPacketOrderSize + kFileContentSize, kChecksumSize);
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
    char generatedChecksum[kChecksumSize] = {};
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

double getTime()
{
    LARGE_INTEGER t, f;

    QueryPerformanceCounter(&t);
    QueryPerformanceFrequency(&f);

    return (double)t.QuadPart * 1000 / (double)f.QuadPart;
}