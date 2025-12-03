#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include <signal.h>
#include <errno.h>

static volatile int running = 1;

void sigint_handler(int signo) {
    running = 0;
}

int main(int argc, char **argv)
{
	printf("CAN Sockets Receive Filter Demo\r\n");

	// socket creation
	int sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (sockfd < 0) {
		perror("Socket");
		return 1;
	}

	// acquisition of interface index for "vcan0"
	struct ifreq ifr;

	strcpy(ifr.ifr_name, "vcan0" );
	ioctl(sockfd, SIOCGIFINDEX, &ifr);

	// binding a socket to the CAN interface
	struct sockaddr_can addr;

	memset(&addr, 0, sizeof(addr));
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Bind");
		return 1;
	}

	// message filter setup (can be an array)
	struct can_filter rfilter;

	rfilter.can_id   = 0x550;
	rfilter.can_mask = 0xFF0;

	setsockopt(sockfd, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));

	// interupt signal handler setup
	struct sigaction sa;

  sa.sa_handler = sigint_handler; // interupt signal handler
  sigemptyset(&sa.sa_mask);       // block no additional signals
  sa.sa_flags = 0;                // do not restart after receiving signal

  sigaction(SIGINT, &sa, NULL);   // install handler for SIGINT
  sigaction(SIGTERM, &sa, NULL);  // install handler for SIGTERM

	// reading loop
	while (running) {
		struct can_frame frame;

		int nbytes = read(sockfd, &frame, sizeof(struct can_frame)); // blocks until a frame is available

		if (nbytes < 0) {
			if (errno == EINTR && !running) break;
			perror("Read");
			return 1;
		}

		printf("0x%03X [%d] ",frame.can_id, frame.can_dlc);

		for (int i = 0; i < frame.can_dlc; i++)
			printf("%02X ",frame.data[i]);

		printf("\r\n");
	}

	if (close(sockfd) < 0) {
		perror("Close");
		return 1;
	}

	return 0;
}
