#include <pcap.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
//#include <stdint.h>

#define MIN(a, b) ((a)<(b) ? (a) : (b))

#define MAXCAP 2048

void packet_handler(u_char *arg, const struct pcap_pkthdr *pkthdr, const u_char *packet) {
	int *counter = (int *) arg;
	printf("Packet count %d\n", ++(*counter));
	printf("Received packet size: %d\n", pkthdr->len);
	printf("Payload:\n");

	for (int i = 0; i < MIN(16, pkthdr->len); i++) {
		if (isprint(packet[i])) {
			printf("%c", packet[i]);
		}
		else {
			printf(". ");
		}
	}
	printf("\n");
	for (int i = 0; i < MIN(16, pkthdr->len); i++) {
		printf("%02x", packet[i]);
	}
	printf("\n");
	printf("\n");

}

int main(int argc, char **argv) {
	char errbuf[PCAP_ERRBUF_SIZE];
	char *device = NULL;
	pcap_t *descr = NULL;
	int count = 0;

	memset(errbuf, 0, PCAP_ERRBUF_SIZE);

	device = pcap_lookupdev(errbuf);

	printf("Opening device %s\n", device);

	descr = pcap_open_live(device, MAXCAP, 1, 512, errbuf);

	pcap_loop(descr, -1, packet_handler, (u_char *) &count);
	return 0;
}