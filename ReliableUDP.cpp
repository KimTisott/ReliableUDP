/*
	Reliability and Flow Control Example
	From "Networking for Game Programmers" - http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
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
	// Method to get hash from the string
	/*
	*	std::string text = "hello world";
	*	MD5 md5;
	*	md5.add(text.c_str(), text.size());
	*	cout << md5.getHash();
	*/

	char iFileName[kFileNameSize] = {};
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
			return 0;
		}
	}

	if (argc > 1)
	{
		char* fileName = argv[1];
		if (strlen(fileName) > kFileNameSize)
		{
			printf("Filename should not be longer than %d characters", kFileNameSize);
			return FILENAME_TOOLONG;
		}

		for (int i = 0; i < sizeof(kInvalidFilenameChars); i++)
		{
			char invalidChar = kInvalidFilenameChars[i];
			if (strchr(fileName, invalidChar) != NULL)
			{
				printf("Filename has invalid character: %c", invalidChar);
				return FILENAME_INVALIDCHARACTERS;
			}
		}
	}

	if (argc == 3)
	{
		int a, b, c, d;
		char* ipAddress = argv[2];
		if (sscanf(ipAddress, "%d.%d.%d.%d", &a, &b, &c, &d))
		{
			mode = Client;
			address = Address(a, b, c, d, ServerPort);
		}
		else
		{
			printf("IP Address is not in proper format: %c", ipAddress);
			return IPADDRESS_INVALIDFORMAT;
		}
	}
	else if (argc > 3)
	{
		displayHelp();
		return ARGUMENT_INVALIDNUMBER;
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
	FILE* file = NULL;
	int fileSize = 0;
	unsigned short iPacketTotal = 0;
	unsigned short iPacketOrder = 0;
	unsigned char iFileContent[kFileContentSize];
	char oFileName[kFileNameSize + 1];
	unsigned short oPacketTotal = 0;
	unsigned short oPacketOrder = 0;
	unsigned char oFileContent[kFileContentSize];
	char oChecksum[kChecksumSize + 1];
	size_t bytesRead;
	unsigned char iPacket[kPacketSize];
	if (mode == Client)
	{
		file = fopen(iFileName, "rb");
		if (file == NULL)
		{
			printf("Cannot open file: %s", iFileName);
			return FILE_CANNOTOPEN;
			fseek(file, 0L, SEEK_END);
			fileSize = ftell(file);
			rewind(file);
			iPacketTotal = (fileSize / kFileContentSize) + 1;
			
		}
	}
	FILE* ofile = NULL;
	if (mode == Server)
	{
		ofile = fopen(iFileName, "wb");
		if (ofile == NULL)
		{
			printf("Cannot open file: %s", iFileName);
			return FILE_CANNOTOPEN;
		}
	}

	
	
	


	bool connected = false;
	float sendAccumulator = 0.0f;
	float statsAccumulator = 0.0f;

	FlowControl flowControl;

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



			if (!feof(file))
			{
				if ((bytesRead = fread(iFileContent, sizeof(unsigned char), kFileContentSize, file)) != 0)
				{
					packData(iPacket, iFileName, iPacketTotal, iPacketOrder, iFileContent);
					if (connection.SendPacket((const unsigned char*)iPacket, sizeof(iPacket)))
					{
						iPacketOrder++;
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
				{
					break;
				}
				else
				{
					unpackData(iPacket, oFileName, &oPacketTotal, &oPacketOrder, oFileContent, oChecksum);
					printf("\nFile Name: %s", oFileName);
					printf("\nPacket Total: %d", oPacketTotal);
					printf("\nPacket Order: %d", oPacketOrder);
					printf("\nFile Content: %s", oFileContent);
					printf("\nChecksum: %s", oChecksum);
					int status = compareChecksum(oChecksum, iPacket);
					printf("\nOK: %d", status);
					if (status == 1)
					{
						fwrite(oFileContent, kFileContentSize, sizeof(unsigned char), ofile);
						iPacketOrder++;
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
		fclose(ofile);
		ShutdownSockets();

		return 0;
	}
}
