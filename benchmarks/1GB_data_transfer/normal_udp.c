#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/types.h>

int main() {
	int i, err, fd;
	struct sockaddr_in src_addr;
	struct sockaddr_in dst_addr;
	struct msghdr msg;
	struct iovec vec;

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	src_addr.sin_family = AF_INET;
	src_addr.sin_port = htons(12345);
	inet_aton("133.27.170.122", (struct in_addr *)&(src_addr.sin_addr));
	err = bind(fd, (struct sockaddr *)&(src_addr), sizeof(struct sockaddr_in));

	vec.iov_base = malloc(1024);

	for(i=0; i<10000; i++) {
		dst_addr.sin_family = AF_INET;
		dst_addr.sin_port = htons(12345);
    inet_aton("133.27.170.1", &(dst_addr.sin_addr));

    msg.msg_name = &dst_addr;
    msg.msg_namelen = sizeof(struct sockaddr_in);
    msg.msg_control = NULL;
    msg.msg_flags = 0;

    memset(vec.iov_base, 0, 1024);
    memset(vec.iov_base, 'A', 1024);
    vec.iov_len = 1024;

    msg.msg_iov = &vec;
    msg.msg_iovlen = vec.iov_len;

    sendmsg(fd, &msg, 0);
	}
	return 0;
}
