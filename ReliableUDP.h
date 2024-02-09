#pragma once
#include <stdio.h>
#include <string.h>

const int kErrorNum = -1;
const char kInvalidFilenameChars[] = { '\\', '/', ':', '*', '?', '"', '<', '>', '|' };
const int kUdpHeaderSize = 12;
const int kFileNameSize = 50;
const int kPacketTotalSize = sizeof(short);
const int kPacketOrderSize = sizeof(short);
const int kChecksumSize = 32;
const int kFileContentSize = 384 - kFileNameSize - kPacketTotalSize - kPacketOrderSize - kChecksumSize;
const int kPacketSize = kUdpHeaderSize + kFileNameSize + kPacketTotalSize + kPacketOrderSize + kFileContentSize + kChecksumSize;

void displayHelp();
static char* packData(char* fileName, short packetTotal, short packetOrder, char* fileContent);
static void unpackData(char* packet, char* fileName, int packetTotal, int packetOrder, char* fileContent);
void packData(unsigned char packet[kPacketSize], char fileName[kFileNameSize], short packetTotal, short packetOrder, unsigned char fileContent[kFileContentSize]);
void unpackData(unsigned char packet[kPacketSize], char fileName[kFileNameSize + 1], unsigned short* packetTotal, unsigned short* packetOrder, unsigned char fileContent[kFileContentSize], char checksum[kChecksumSize + 1]);
void generateChecksum(char checksum[kChecksumSize], unsigned char packet[kPacketSize]);
int compareChecksum(char checksum[kChecksumSize], unsigned char packet[kPacketSize]);