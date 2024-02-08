#include <stdlib.h>
#include <string.h>
#include "ReliableUDP.h"

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