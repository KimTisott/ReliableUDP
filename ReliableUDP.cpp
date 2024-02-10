/*
	Reliability and Flow Control Example
	From "Networking for Game Programmers" - http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

/*
 FILE: ReliableUDP.cpp
 PROJECT: SENG2040 - Assignment #1
 PROGRAMMER: Quan Khuong - Kim Tisott
 Author: Glenn Fiedler <gaffer@gaffer.org>
 DESCRIPTION: The file include main.
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "A1.h"
#include "Net.h"
#pragma warning(disable : 4996)

//#define SHOW_ACKS

using namespace std;
using namespace net;

const int ServerPort = 30000;
const int ClientPort = 30001;
const int ProtocolId = 0x11223344;
const float DeltaTime = 1.0f / 30.0f;
const float SendRate = 1.0f / 30.0f;
const float TimeOut = 10.0f;

class FlowControl
{
public:

	FlowControl()
	{
		printf("flow control initialized\n");
		Reset();
	}

	void Reset()
	{
		mode = Bad;
		penalty_time = 4.0f;
		good_conditions_time = 0.0f;
		penalty_reduction_accumulator = 0.0f;
	}

	void Update(float deltaTime, float rtt)
	{
		const float RTT_Threshold = 250.0f;

		if (mode == Good)
		{
			if (rtt > RTT_Threshold)
			{
				printf("*** dropping to bad mode ***\n");
				mode = Bad;
				if (good_conditions_time < 10.0f && penalty_time < 60.0f)
				{
					penalty_time *= 2.0f;
					if (penalty_time > 60.0f)
						penalty_time = 60.0f;
					printf("penalty time increased to %.1f\n", penalty_time);
				}
				good_conditions_time = 0.0f;
				penalty_reduction_accumulator = 0.0f;
				return;
			}

			good_conditions_time += deltaTime;
			penalty_reduction_accumulator += deltaTime;

			if (penalty_reduction_accumulator > 10.0f && penalty_time > 1.0f)
			{
				penalty_time /= 2.0f;
				if (penalty_time < 1.0f)
					penalty_time = 1.0f;
				printf("penalty time reduced to %.1f\n", penalty_time);
				penalty_reduction_accumulator = 0.0f;
			}
		}

		if (mode == Bad)
		{
			if (rtt <= RTT_Threshold)
				good_conditions_time += deltaTime;
			else
				good_conditions_time = 0.0f;

			if (good_conditions_time > penalty_time)
			{
				printf("*** upgrading to good mode ***\n");
				good_conditions_time = 0.0f;
				penalty_reduction_accumulator = 0.0f;
				mode = Good;
				return;
			}
		}
	}

	float GetSendRate()
	{
		return mode == Good ? 30.0f : 10.0f;
	}

private:

	enum Mode
	{
		Good,
		Bad
	};

	Mode mode;
	float penalty_time;
	float good_conditions_time;
	float penalty_reduction_accumulator;
};

// ----------------------------------------------

int main(int argc, char* argv[])
{

	// parse command line

	enum Mode
	{
		Client,
		Server
	};

	Mode mode = Server;
	Address address;
	
	
	//Find and display the help menu once the user enter -h
	for (int i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], "-h") == 0)
		{
			displayHelp();
			return 0;
		}
	}

	// More than needed argument
	if (argc > 3)
	{
		displayHelp();
		return ARGUMENT_INVALIDNUMBER;
	}

	char ipAddress[kIPAddressSize] = {};
	char fileName[kFileNameSize] = {};
	if (argc < 3)
	{
		//strcpy(ipAddress, kIPAddressDefault);
	}
	else
	{
		strcpy(ipAddress, argv[2]);
	}

	if (argc < 2)
	{
		strcpy(fileName, kFileNameDefault);
	}
	else
	{
		strcpy(fileName, argv[1]);
	}

	// Display error message if the file name is longer than the limit
	if (strlen(fileName) > kFileNameSize)
	{
		printf("Filename should not be longer than %d characters", kFileNameSize);
		return FILENAME_TOOLONG;
	}

	// Check for invalid character character in the file name
	for (int i = 0; i < sizeof(kInvalidFilenameChars); i++)
	{
		char invalidChar = kInvalidFilenameChars[i];
		if (strchr(fileName, invalidChar) != NULL)
		{
			printf("Filename has invalid character: %c", invalidChar);
			return FILENAME_INVALIDCHARACTERS;
		}
	}

	// If there is an argument for IP Address, check the IP Address
	if (strlen(ipAddress))
	{
		int a, b, c, d;
		// Check the format of the IP Address
		if (sscanf(ipAddress, "%d.%d.%d.%d", &a, &b, &c, &d) == 4)
		{
			mode = Client;
			address = Address(a, b, c, d, ServerPort);
		}
		else
		{
			printf("IP Address is not in proper format: %s", ipAddress);
			return IPADDRESS_INVALIDFORMAT;
		}
	}

	// initialize

	if (!InitializeSockets())
	{
		printf("failed to initialize sockets\n");
		return 1;
	}

	ReliableConnection connection(ProtocolId, TimeOut);

	const int port = mode == Server ? ServerPort : ClientPort;

	if (!connection.Start(port))
	{
		printf("could not start connection on port %d\n", port);
		return 1;
	}

	if (mode == Client)
		connection.Connect(address);
	else
		connection.Listen();

	 /* 
	 * Open the FILE stream and open the file
	 * for read or write depends on if the 
	 * application is running in Client or
	 * in Server mode 
	 */
	FILE* file = NULL;
	int fileSize = 0;
	unsigned short packetTotal = 0;
	unsigned short packetOrder = 1;
	unsigned char fileContent[kFileContentSize] = {};
	char checksum[kChecksumSize] = {};
	size_t bytesRead;
	unsigned char packet[kPacketSize] = {};
	if (mode == Client)
	{
		file = fopen(fileName, "rb");
		if (file == NULL)
		{
			printf("Cannot open file: %s", fileName);
			return FILE_CANNOTOPEN;
		}

		// Find the size of the file
		fseek(file, 0L, SEEK_END);
		fileSize = ftell(file);
		rewind(file);

		// Calculate total packet number that is needed to send
		packetTotal = (fileSize / kFileContentSize) + 1;
	}
	else if (mode == Server)
	{
		file = fopen(fileName, "wb");
		if (file == NULL)
		{
			printf("Cannot open file: %s\n", fileName);
			return FILE_CANNOTOPEN;
		}
	}
	
	bool connected = false;
	float sendAccumulator = 0.0f;
	float statsAccumulator = 0.0f;
	bool firstPacket = true;
	double start;
	FlowControl flowControl;


	while (true)
	{
		// update flow control

		if (connection.IsConnected())
			flowControl.Update(DeltaTime, connection.GetReliabilitySystem().GetRoundTripTime() * 1000.0f);

		const float sendRate = flowControl.GetSendRate();

		// detect changes in connection state

		if (mode == Server && connected && !connection.IsConnected())
		{
			flowControl.Reset();
			printf("reset flow control\n");
			connected = false;
		}

		if (!connected && connection.IsConnected())
		{
			printf("client connected to server\n");
			connected = true;
		}

		if (!connected && connection.ConnectFailed())
		{
			printf("connection failed\n");
			break;
		}

		// send and receive packets

		sendAccumulator += DeltaTime;

		while (sendAccumulator > 1.0f / sendRate)
		{
			/*
			* CLIENT to check to see if it is the first packet or
			* not to start the timer and call the pack data to
			* pack everything into a packet to send to the receiver
			* using sendpacket
			*/
			if (mode == Client)
			{
				if (firstPacket)
				{
					start = getTime();
					firstPacket = false;
				}
				if (!feof(file))
				{
					// Keep on reading until it reaches end of file
					if ((bytesRead = fread(fileContent, sizeof(unsigned char), kFileContentSize, file)) != 0)
					{
						packData(packet, fileName, packetTotal, packetOrder, fileContent);
						if (connection.SendPacket((const unsigned char*)packet, sizeof(packet)))
						{
							packetOrder++;
						}
					}
				}
				// There is no more packet
				else
				{
					// Get the time from the start until this point
					double transmissionTime = getTime() - start;
					printf("\nTransfer time %.0fms, Transfer speed: %f Mbit/s\n", transmissionTime,((double)fileSize/transmissionTime)/125);

					// Close the file and shutdown the socket
					fclose(file);
					ShutdownSockets();
					return 0;
				}
			}

			// SERVER will send an empty packet to set ack bit
			else
			{
				memset(packet, 0, sizeof(packet));
				connection.SendPacket((const unsigned char*)packet, sizeof(packet));
			}
			sendAccumulator -= 1.0f / sendRate;
		}

		while (true)
		{
			/*
			* The server will receive the data here
			*/
			unsigned char packet[kPacketSize] = {};
			int bytes_read = connection.ReceivePacket(packet, sizeof(packet));
			if (bytes_read == 0)
			{
				break;
			}
			else
			{
				unpackData(packet, fileName, &packetTotal, &packetOrder, fileContent, checksum);

				// If the packet arrive correctly, then write into the file
				if (compareChecksum(checksum, packet))
				{
					fwrite(fileContent, kFileContentSize, sizeof(unsigned char), file);
				}
			}
		}

		// show packets that were acked this frame

#ifdef SHOW_ACKS
		unsigned int* acks = NULL;
		int ack_count = 0;
		connection.GetReliabilitySystem().GetAcks(&acks, ack_count);
		if (ack_count > 0)
		{
			printf("acks: %d", acks[0]);
			for (int i = 1; i < ack_count; ++i)
				printf(",%d", acks[i]);
			printf("\n");
		}
#endif

		// update connection

		connection.Update(DeltaTime);

		// show connection stats

		statsAccumulator += DeltaTime;

		while (statsAccumulator >= 0.25f && connection.IsConnected())
		{
			float rtt = connection.GetReliabilitySystem().GetRoundTripTime();

			unsigned int sent_packets = connection.GetReliabilitySystem().GetSentPackets();
			unsigned int acked_packets = connection.GetReliabilitySystem().GetAckedPackets();
			unsigned int lost_packets = connection.GetReliabilitySystem().GetLostPackets();

			float sent_bandwidth = connection.GetReliabilitySystem().GetSentBandwidth();
			float acked_bandwidth = connection.GetReliabilitySystem().GetAckedBandwidth();

			printf("rtt %.1fms, sent %d, acked %d, lost %d (%.1f%%), sent bandwidth = %.1fkbps, acked bandwidth = %.1fkbps\n",
				rtt * 1000.0f, sent_packets, acked_packets, lost_packets,
				sent_packets > 0.0f ? (float)lost_packets / (float)sent_packets * 100.0f : 0.0f,
				sent_bandwidth, acked_bandwidth);

			statsAccumulator -= 0.25f;
		}

		net::wait(DeltaTime);
	}
	fclose(file);
	ShutdownSockets();

	return 0;
}