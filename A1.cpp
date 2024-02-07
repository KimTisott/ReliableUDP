#include "ReliableUDP.h"
#pragma warning(disable : 4996)


void displayHelp()
{
    printf(" Usage: \n");
    printf(" ReliableUDP [INPUT_FILE] [IPADDRESS] [-h]\n");
    printf(" INPUT_FILE     :       FILENAME is the INPUT file name \n");
    printf(" IPADDRESS      :       IP ADDRESS in IPv4 format (x.x.x.x) \n");
    printf(" -h             :       Display help\n");

}

// Return:  kErrorNum if the metadata cannot be read in the right format
//          0 if the metadata is read successfully
int unpackIncomingPacketMetaData(char receiveData[])
{
    char fileName [kFilenameMaxLength] = "";
    char packetString [6] = "";
    int packetTotal = NULL;
    int packetCounter = NULL;
    char* token = strtok(receiveData, "|");
    if (token != NULL)
    {
        strcpy(fileName, token);
        token = strtok(NULL, "|");
        if (token != NULL)
        {
            strcpy(packetString, token);
            packetTotal = (int)packetString;
            token = strtok(NULL, "|");
            if (token != NULL)
            {
                strcpy(packetString, token);
                packetCounter = (int)packetString;
            }
            else
            {
                printf("Not in the right data format");
                return kErrorNum;
            }
        }
        else
        {
            printf("Not in the right data format");
            return kErrorNum;
        }
    }
    else
    {
        printf("Not in the right data format");
        return kErrorNum;
    }
    return 0;
    
}