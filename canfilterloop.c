
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include <signal.h>

static volatile int running = 1;

void sigint_handler(int signo) {
    running = 0;   // tell the loop to stop
}

int main(int argc, char **argv)
{
	int s, i; 
	struct sockaddr_can addr;
	struct ifreq ifr;
	struct can_frame frame;

	printf("CAN Sockets Receive Filter Demo\r\n");

	// socket creation
	if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("Socket");
		return 1;
	}

	// acquisition of interface index for "vcan0"
	strcpy(ifr.ifr_name, "vcan0" );
	ioctl(s, SIOCGIFINDEX, &ifr);

	// binding a socket to the CAN interface
	memset(&addr, 0, sizeof(addr));
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Bind");
		return 1;
	}

	// setting up filter
	struct can_filter rfilter[1];

	rfilter[0].can_id   = 0x550;
	rfilter[0].can_mask = 0xFF0;
	//rfilter[1].can_id   = 0x200;
	//rfilter[1].can_mask = 0x700;

	setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));

	// reading loop
	signal(SIGINT, sigint_handler);
	signal(SIGTERM, sigint_handler);

	while (running) {
		int nbytes = read(s, &frame, sizeof(struct can_frame)); // blocks until a frame is available

		if (nbytes < 0) {
			if (!running) break;
			perror("Read");
			return 1;
		}

		printf("0x%03X [%d] ",frame.can_id, frame.can_dlc);

		for (i = 0; i < frame.can_dlc; i++)
			printf("%02X ",frame.data[i]);

		printf("\r\n");
	}

	if (close(s) < 0) {
		perror("Close");
		return 1;
	}

	return 0;
}
