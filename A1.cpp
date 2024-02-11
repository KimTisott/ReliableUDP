/*
 FILE: A1.cpp
 PROJECT: SENG2040 - Assignment #1
 PROGRAMMER: Quan Khuong - Kim Tisott
 FIRST VERSION: February 05 2024 
 DESCRIPTION: The file include functions to pack/unpack data, timer, and help function.
 */

#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "A1.h"
#include "hash-library/md5.h"
#include "hash-library/md5.cpp"

#pragma warning(disable : 4996)



/*
 * Method:       displayHelp()
 * Description:  Displays usage information for the program.
 * Parameters:   None
 * Returns:      void
 */
void displayHelp()
{
    printf(" Usage: \n");
    printf(" ReliableUDP [INPUT_FILE] [IPADDRESS] [-h]  :       Run as CLIENT with INPUTFILE and IPADDRESS\n");
    printf(" ReliableUDP [OUTPUT_FILE]                  :       Run as SERVER and write to OUTPUTFILE\n");
    printf(" INPUT_FILE     :       FILENAME is the INPUT file name \n");
    printf(" IPADDRESS      :       IP ADDRESS in IPv4 format (x.x.x.x) \n");
    printf(" -h             :       Display help\n");
    
}


/*
 * Method:       packData()
 * Description:  Packs data into a packet for transmission.
 * Parameters:
 *               packet:        The packet buffer where data will be packed.
 *               fileName:      The name of the file being transmitted.
 *               packetTotal:   Total number of packets for the file.
 *               packetOrder:   Order of the current packet.
 *               fileContent:   Content of the file to be transmitted.
 * Returns:      void
 */
void packData(unsigned char packet[kPacketSize], char fileName[kFileNameSize], short packetTotal, short packetOrder, unsigned char fileContent[kFileContentSize])
{
    // Copy file name into packet
    memcpy(packet, fileName, kFileNameSize);

    // Copy packet total into packet
    memcpy(packet + kFileNameSize, &packetTotal, kPacketTotalSize);

    // Copy packet order into packet
    memcpy(packet + kFileNameSize + kPacketTotalSize, &packetOrder, kPacketOrderSize);

    // Copy file content into packet
    memcpy(packet + kFileNameSize + kPacketTotalSize + kPacketOrderSize, fileContent, kFileContentSize);

    // Generate checksum and copy it into packet
    char checksum[kChecksumSize + 1] = {};
    generateChecksum(checksum, packet);
    checksum[kChecksumSize] = '\0';
    memcpy(packet + kFileNameSize + kPacketTotalSize + kPacketOrderSize + kFileContentSize, checksum, kChecksumSize);
}


/*
 * Method:       unpackData()
 * Description:  Unpacks data from a packet received.
 * Parameters:
 *               packet:        The packet buffer containing data to be unpacked.
 *               fileName:      Pointer to the buffer where the file name will be stored.
 *               packetTotal:   Pointer to the variable where the total number of packets will be stored.
 *               packetOrder:   Pointer to the variable where the order of the current packet will be stored.
 *               fileContent:   Buffer where the file content will be stored.
 *               checksum:      Buffer where the checksum will be stored.
 * Returns:      void
 */
void unpackData(unsigned char packet[kPacketSize], char fileName[kFileNameSize + 1], unsigned short* packetTotal, unsigned short* packetOrder, unsigned char fileContent[kFileContentSize], char checksum[kChecksumSize + 1])
{
    // Copy file name from packet
    memcpy(fileName, packet, kFileNameSize);

    // Ensure null termination for strings
    fileName[kFileNameSize] = '\0';

    // Extract packetTotal and packetOrder from packet
    *packetTotal = packet[kFileNameSize];
    *packetTotal += packet[kFileNameSize + 1] << 8;
    *packetOrder = packet[kFileNameSize + kPacketTotalSize];
    *packetOrder += packet[kFileNameSize + kPacketTotalSize + 1] << 8;

    // Copy file content from packet
    memcpy(fileContent, packet + kFileNameSize + kPacketTotalSize + kPacketOrderSize, kFileContentSize);

    // Copy checksum from packet
    memcpy(checksum, packet + kFileNameSize + kPacketTotalSize + kPacketOrderSize + kFileContentSize, kChecksumSize);
    // Ensure null termination for strings
    checksum[kChecksumSize] = '\0';
}


/*
 * Method:       generateChecksum()
 * Description:  Generates a checksum for the given packet using MD5 hashing algorithm.
 * Parameters:
 *               checksum:  Buffer to store the generated checksum.
 *               packet:    The packet buffer for which checksum needs to be generated.
 * Returns:      void
 */
void generateChecksum(char checksum[kChecksumSize], unsigned char packet[kPacketSize])
{
    MD5 md5;
    md5.add(packet, kPacketSize - kChecksumSize);
    memcpy(checksum, md5.getHash().c_str(), kChecksumSize);
}


/*
 * Method:       compareChecksum()
 * Description:  Compares the received checksum with the checksum generated from the packet.
 * Parameters:
 *               checksum:  The checksum to compare.
 *               packet:    The packet buffer containing data.
 * Returns:      1 if checksum match, otherwise 0.
 */
int compareChecksum(char checksum[kChecksumSize], unsigned char packet[kPacketSize])
{
    char generatedChecksum[kChecksumSize] = {};

    // Generate checksum from packet
    generateChecksum(generatedChecksum, packet);

    // Compare checksum
    for (int i = 0; i < kChecksumSize; i++)
    {
        if (checksum[i] != generatedChecksum[i])
        {
            // Checksum don't match
            return 0;
        }
    }

    // Checksum match
    return 1;
}


/*
 * Method:       getTime()
 * Description:  Gets the current system time in milliseconds.
 * Parameters:   None
 * Returns:      The current system time in milliseconds as a double.
 */
double getTime()
{
    LARGE_INTEGER t, f;

    QueryPerformanceCounter(&t);        // Get current time
    QueryPerformanceFrequency(&f);      // Get frequency for conversion

    // Convert time to milliseconds
    return (double)t.QuadPart * 1000 / (double)f.QuadPart;
}