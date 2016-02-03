#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/types.h>

int main(int argc, char** argv) {
	int i, err, fd;
	struct sockaddr_in src_addr;
	struct sockaddr_in dst_addr;
	struct msghdr msg;
	struct iovec vec;
        int loop_num;
        int packet_size;

        clock_t start = 0;
        clock_t end = 0;

	if(argc != 2) {
	    printf("Invalid argument\n");
	    exit(EXIT_FAILURE);
	}

        packet_size = atoi(argv[1]);

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	if(!fd) {
            perror("socket");
	    exit(EXIT_FAILURE);
	}


	src_addr.sin_family = AF_INET;
	src_addr.sin_port = htons(12345);
	inet_aton("10.10.2.10", (struct in_addr *)&(src_addr.sin_addr));
	err = bind(fd, (struct sockaddr *)&(src_addr), sizeof(struct sockaddr_in));

	if(err) {
	    perror("bind");
            exit(EXIT_FAILURE);
        }

	vec.iov_base = malloc(packet_size);

	loop_num = (1024*1024*1024)/packet_size;

        start = clock();
	for(i=0; i<loop_num; i++) {
		dst_addr.sin_family = AF_INET;
		dst_addr.sin_port = htons(12345);
		inet_aton("10.10.2.2", &(dst_addr.sin_addr));

		msg.msg_name = &dst_addr;
		msg.msg_namelen = sizeof(struct sockaddr_in);
		msg.msg_control = NULL;
		msg.msg_controllen = 0;
		msg.msg_flags = 0;

		memset(vec.iov_base, 0, packet_size);
		memset(vec.iov_base, 'A', packet_size);
		vec.iov_len = packet_size;

		msg.msg_iov = &vec;
		msg.msg_iovlen = 1;

		err = sendmsg(fd, &msg, 0);

        }
        end = clock();

        printf("%d,%.2f,%ld\n", packet_size, (double)(end-start), CLOCKS_PER_SEC);

        return 0;
}
