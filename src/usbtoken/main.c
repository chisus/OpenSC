#include <asm/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "usbtoken.h"

struct token usbtoken;

int main(int argc, char **argv)
{
	char *action, *device, *product;
	int rc;

	if (argc == 0) {
		perror("usbtoken hotplug should be called by the kernel");
		return 1;
	}

	openlog(SYSLOG_NAME, LOG_CONS | LOG_PERROR | LOG_PID, LOG_KERN);

	if (pid_init() != USBTOKEN_OK) {
		syslog(LOG_DEBUG,
		       "all slots in use. increased MAXTOKEN, recompile");
		return 1;
	}

	/* usb device? first parameter shoiuld be "usb". */
	if (!argv || argc < 1 || !argv[1] || strcmp(argv[1], "usb") != 0) {
		syslog(LOG_DEBUG, "%s called with %s (not \"usb\")\n",
		       argv[0], argv[1]);
		return 0;
	}

	/* action should be "add".
	 * we will notice removes ourself. */
	action = getenv("ACTION");
	if (!action || strcmp(action, "add") != 0) {
		syslog(LOG_DEBUG, "action is %s, ignoring.\n", action);
		return 0;
	}

	product = getenv("PRODUCT");
	{
		if (eutron_test(product) == USBTOKEN_OK ||
		    etoken_test(product) == USBTOKEN_OK ||
		    ikey2k_test(product) == USBTOKEN_OK ||
		    ikey3k_test(product) == USBTOKEN_OK) {
			syslog(LOG_DEBUG,
			       "found product : %s", usbtoken.drv.name);
		} else {
			syslog(LOG_ERR, "unknown product %s", product);
			return 1;
		}
	}

	/* DEVICE should hold the path to the device file we need. */
	device = getenv("DEVICE");
	if (!device) {
		syslog(LOG_ERR, "device not found, aborting!\n");
		return 1;
	}

	/* the device might not be there right now. wait one second */
	sleep(1);
	usbtoken.usbfd = open(device, O_EXCL | O_RDWR);
	if (usbtoken.usbfd == -1) {
		syslog(LOG_ERR, "open device %s failed: %s, aborting!\n",
		       device, strerror(errno));
		return 1;
	}

	if (usbtoken.drv.init() != USBTOKEN_OK) {
		return 1;
	}

	if (parse_atr() != USBTOKEN_OK) {
		return 1;
	}

	if (increase_ifsc() != USBTOKEN_OK) {
		return 1;
	}

	if (socket_init() != USBTOKEN_OK) {
		return 1;
	}

	while (1) {
		struct pollfd pollfd[3];
		pollfd[0].fd = usbtoken.usbfd;
		pollfd[0].events = POLLIN | POLLPRI | POLLOUT;
		pollfd[1].fd = usbtoken.unixfd;
		pollfd[1].events = POLLIN | POLLPRI;
		if (usbtoken.connfd) {
			pollfd[2].fd = usbtoken.connfd;
			pollfd[2].events = POLLIN | POLLPRI;
			rc = poll(pollfd, 3, 1000);
		} else {
			rc = poll(pollfd, 2, 1000);
		}

		if (rc == -1 && errno == EINTR)
			continue;

		if (rc == -1) {
			syslog(LOG_ERR,
			       "poll returned error, aborting: %s!\n",
			       strerror(errno));
			return 1;
		}

		if (pollfd[0].revents) {
			if ((pollfd[0].revents & 0x18) == 0x18) {
				syslog(LOG_DEBUG,
				       "device removed. exiting.\n");
				return 0;
			}
			/* handle usb stuff */
		}

		if (usbtoken.connfd && (pollfd[2].revents & POLLHUP)) {
			socket_hangup();
			continue;
		}

		if (pollfd[1].revents) {
			/* handle unix socket (listen()ing) */
			socket_accept();
			continue;
		}

		if (usbtoken.connfd && pollfd[2].revents) {
			socket_xmit();
		}

		/* ignore other poll results */
	}


	return 0;
}
