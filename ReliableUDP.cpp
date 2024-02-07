/*
	Reliability and Flow Control Example
	From "Networking for Game Programmers" - http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "hash-library/md5.h"
#include "hash-library/md5.cpp"
#include "ReliableUDP.h"
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
const char* kPacketDelimiter = "|";

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
	// Method to get hash from the string
	/*
	*	std::string text = "hello world";
	*	MD5 md5;
	*	md5.add(text.c_str(), text.size());
	*	cout << md5.getHash(); 
	*/
	
	char fileName[kFilenameMaxLength] = {};
	// parse command line

	enum Mode
	{
		Client,
		Server
	};

	Mode mode = Server;
	Address address;
	/*
	* We have to introduce a new argument as a file name
	* We are going to put it as the first argument and make it mandatory
	* We are going to change argc>=2 -> argc >= 3 and validate the 
	* file name on argv[1] as well as IP Address on argv[2] 
	*/
	for (int i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], "-h") == 0)
		{
			displayHelp();
			return kErrorNum;
		}
	}
	if (argc == 3)
	{
		char* filenameArg = argv[1];
		if (strlen(filenameArg) > kFilenameMaxLength)
		{
			printf("Filename should not be longer than %d characters", kFilenameMaxLength);
			return kErrorNum;
		}

		for (int i = 0; i < sizeof(kInvalidFilenameChars); i++)
		{
			char invalidChar = kInvalidFilenameChars[i];
			if (strchr(fileName, invalidChar) != NULL)
			{
				printf("Filename has invalid character: %c", invalidChar);
				return kErrorNum;
			}
		}

		int a, b, c, d;
		if (sscanf(argv[2], "%d.%d.%d.%d", &a, &b, &c, &d))
		{
			mode = Client;
			address = Address(a, b, c, d, ServerPort);
		}
		strcpy(fileName, filenameArg);
	}
	else
	{
		displayHelp();
		return kErrorNum;
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
	* 
	* CLIENT check the file is there or not
	* before open the connection.
	* Also, checking for its content. If the file is empty, 
	* print an error message and stop the program.
	* SERVER will do File/IO error handling, if
	* the file cannot be opened, stop the program.
	* 
	*/
	FILE* file = fopen(fileName, "rb");
	if (file == NULL)
	{
		printf("Cannot open file: %s", fileName);
		return kErrorNum;
	}

	// Load the file contents into a buffer
	char* fileBuffer = (char*)malloc(kFileMaxSize);
	size_t fileSize;
	if (!(fileSize = fread(&fileBuffer, sizeof(unsigned char), kFileMaxSize, file)))
	{
		printf("Cannot read file: %s", fileName);
		return kErrorNum;
	}
	int packetTotal = fileSize / kPacketSize;

	bool connected = false;
	float sendAccumulator = 0.0f;
	float statsAccumulator = 0.0f;

	FlowControl flowControl;
	int packetCounter = 1;
	/*
	* CLIENT ONLY
	* Open the validated file and get its metadata (file name and file size)
	* and file content
	* 
	* Calculate how many packet that is needed to send
	* and split the file into pieces to be sent
	*/
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
			* CLIENT send file metadata and checksum firstly and then
			* put each piece into a packet and send the packet using SendPacket
			*/
			string packet = fileName + '|' + packetTotal + '|' + packetCounter + '|';
			int remaining = kPacketSize - packet.length();
			if (remaining > 0)
			{
				char* chunk = NULL;
				strncpy(chunk, fileBuffer, remaining);
				if (connection.SendPacket((const unsigned char*)packet.c_str(), sizeof(packet)))
				{
					packetCounter++;
					fileBuffer += remaining;
				}
			}
			sendAccumulator -= 1.0f / sendRate;
		}
		
		while (true)
		{
			/*
			* SERVER is going to receive metadata and content
			* in the packet and store it into a buffer to 
			* pack the pieces when the last packet is
			* received.
			* When the last packet is received,
			* SERVER should check for checksum 
			* If it's OK, write to the disk
			* 
			*/
			unsigned char packet[kPacketSize];
			int bytes_read = connection.ReceivePacket(packet, sizeof(packet));
			if (bytes_read == 0)
				break;

			char* filename = NULL;
			int packetTotal;
			int packetOrder;
			char* fileData = NULL;
			char* token = NULL;
			for (int i = 0; i < 4; i++)
			{
				token = strtok((char*)packet, kPacketDelimiter);
				if (token == NULL)
				{
					// could not find one of the data fields... corrupted packet?
				}
				else
				{ // split data fields for further analysis
					switch (i)
					{
						case 0:
						{ // filename
							strcpy(filename, token);
						}
						case 1:
						{ // packet total
							packetTotal = atoi(token);
						}
						case 2:
						{ // packet order
							packetOrder = atoi(token);
						}
						case 3:
						{ // file data
							strcpy(fileData, token);
						}
					}
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

	ShutdownSockets();

	free(fileBuffer);

	return 0;
}
