#pragma once
#include <stdio.h>
#include <string.h>

const int kErrorNum = -1;
const char kInvalidFilenameChars[] = { '\\', '/', ':', '*', '?', '"', '<', '>', '|' };
const int kFilenameMaxLength = 255;
const int kPacketSize = 500;
const int kFileMaxSize = 31 * 1024 * 1024; // 31MB
void displayHelp();
static char* packData(char* fileName, short packetTotal, short packetOrder, char* fileContent);
static void unpackData(char* packet, char* fileName, int packetTotal, int packetOrder, char* fileContent);