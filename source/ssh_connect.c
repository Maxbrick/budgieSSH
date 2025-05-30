#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>
#include <unistd.h>
#include <ssh.h>
#include <libssh2.h>
#include <sys/types.h>


void budgiessh_prompt(struct sockaddr_in *sa) {
	char host_addr[128];
	char ssh_port[8];
	SwkbdState swkbd;

	swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, sizeof(host_addr));
	swkbdSetHintText(&swkbd, "Host Address");
	swkbdInputText(&swkbd, host_addr, sizeof(host_addr));

	//wait till user inputs host address before continuing
	//while(host_addr[1] == NULL);

	printf("Host Address:%s\n", host_addr);

	SwkbdState swkbd2;
	swkbdInit(&swkbd2, SWKBD_TYPE_NUMPAD, 1, sizeof(ssh_port));
	swkbdSetHintText(&swkbd, "Port Number (usually 22)");
	swkbdInputText(&swkbd, ssh_port, sizeof(ssh_port));

	while(ssh_port[1] == NULL);

	printf("Port Number: %s\n", ssh_port);


	sa->sin_family = AF_INET;
	sa->sin_port = htons(atoi(ssh_port));
	sa->sin_addr.s_addr = inet_addr(host_addr);

	//return sa;
	//strcpy(addr, host_addr);
	//strcpy(port, ssh_port);
	return;
}
void budgiessh_connect(LIBSSH2_SESSION *session, const struct sockaddr_in *sa, s32 sock) {


	if (sock < 0) {
		printf("couldn't make socket :(\n");
	}


	printf("Connecting... \n");
	printf("%d", sa->sin_addr.s_addr);
	printf("%d", sa->sin_port);

	if(connect(
		sock,
		(const struct sockaddr*)sa,
		sizeof(struct sockaddr_in))) {
			printf("connecc failed :/\n");
		} else printf("Socket connected");


	int rc = libssh2_session_handshake(session, sock);
	
	libssh2_trace(session, 1);
	if(rc) {
		printf("faiiil: %d\n", rc);
	} else printf("Handshook\n");
	return;
}
