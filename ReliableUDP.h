#pragma once
#include <stdio.h>
#include <string.h>

const int kErrorNum = -1;
const char kInvalidFilenameChars[] = { '\\', '/', ':', '*', '?', '"', '<', '>', '|' };
const int kFilenameMaxLength = 255;
const int kPacketSize = 500;
const int kFileMaxSize = 31 * 1024 * 1024; // 31MB
void displayHelp();
int unpackIncomingPacketMetaData(char receiveData[]);