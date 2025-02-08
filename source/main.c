#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <libssh2.h>
#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000

static u32 *SOC_buffer = NULL;
s32 sock = -1, csock = -1;

static const char *pubkey = "ssh/";
static const char *privkey = "ssh/";
//---------------------------------------------------------------------------------
void socShutdown() {
//---------------------------------------------------------------------------------
    printf("waiting for socExit...\n");
    socExit();
 
}

int main() {
	SwkbdState swkbd;
	uint32_t host_addr_int;
	int rc;
	int sock;
	char username[60];
	char password[60];
	LIBSSH2_SESSION *session = NULL;
	LIBSSH2_CHANNEL *channel;
	char host_addr[16];

	struct sockaddr_in sa;

	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);
	consoleDebugInit(debugDevice_3DMOO);

	printf("Welcome to 3DSSH!\n");
	// allocate buffer for SOC service
	SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);

	if(SOC_buffer == NULL) {
		printf("memalign: failed to allocate\n");
	}
	// Now initialise soc:u service
	if ((rc = socInit(SOC_buffer, SOC_BUFFERSIZE)) != 0) {
		printf("socInit: 0x%08X\n", (unsigned int)rc);
	}
	// register socShutdown to run at exit
	// atexit functions execute in reverse order so this runs before gfxExit
	atexit(socShutdown);
	

	rc = libssh2_init(0);
	if(rc) {
        	fprintf(stderr, "libssh2 initialization failed (%d)\n", rc);
        	return 1;
    	}

	// Prompt user for target user and host address
	swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, -1);
	swkbdSetHintText(&swkbd, "Username");
	swkbdInputText(&swkbd, username, sizeof(username));
	
        swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, -1);
        swkbdSetHintText(&swkbd, "Host Address");
        swkbdInputText(&swkbd, host_addr, sizeof(host_addr));
		
	printf("(Assuming port is 22)\n");

	printf("Username: %s\n", username);
	printf("Host Address: %s\n", host_addr);

	host_addr_int = inet_addr(host_addr);

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

    	if (sock < 0) {
		printf("couldn't make socket :(\n");
    	}

	sa.sin_family = AF_INET;
	sa.sin_port = htons(22);
	sa.sin_addr.s_addr = host_addr_int;

	printf("Connecting to %s:%d as user %s\n",
		host_addr, ntohs(sa.sin_port), username);

	if(connect(
		sock,
		(struct sockaddr*)(&sa),
		sizeof(struct sockaddr_in)))
		printf("connecc failed :/\n");

	session = libssh2_session_init();

	if(!session)
		printf("could not initialize SSH session x.x\n");

	rc = libssh2_session_handshake(session, sock);
	
	libssh2_trace(session, 1);
	if(rc) {
		printf("faiiil: %d\n", rc);
	} else printf("Handshook\n");

	swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, -1);
        swkbdSetHintText(&swkbd, "Password");
        swkbdInputText(&swkbd, password, sizeof(password));


	if(libssh2_userauth_password(session, username, password)) {
		printf("Authenticate by password failed!\n");
	} 
 
	channel = libssh2_channel_open_session(session);
	printf("Openning libssh2 session...\n");
    	if(!channel) {
        	printf("Unable to open a session\n");
    	}
	printf("Requesting pty..\n");
	if(libssh2_channel_request_pty(channel, "ansi")) {
        	printf("Failed requesting pty\n");
    	}
	printf("requesting shell..\n");
	if(libssh2_channel_shell(channel)) {
        	printf("Unable to request shell on allocated pty\n");
        }
	libssh2_channel_set_blocking(channel, 0);

	libssh2_channel_write(channel, "\x1b[?25h", 6);
	// Main loop
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		gfxSwapBuffers();
		char buf[1024];
		ssize_t err = libssh2_channel_read(channel, buf, sizeof(buf));

		if(err < 0)
             		fprintf(stderr, "Unable to read response: %ld\n", (long)err);
            	else {
                	fwrite(buf, 1, (size_t)err, stdout);
            	}
		#define ENTER_BUF "\x0A"
		#define UP_BUF "\x1b[1A"
		#define DOWN_BUF "\x1b[1B"
		#define RIGHT_BUF "\x1b[1C"
		#define LEFT_BUF "\x1b[1D"
	
		hidScanInput();
		u32 kDown = hidKeysDown();
		
		// loop through all possible button bits, check if
		// they are true, if so execute its function
		u32 i;

		for(i = 0; (i < 32); i++) {
		if(!(kDown & BIT(i))) continue;
		switch(BIT(i)) {
		case KEY_A:
			char textbuf[120];
			swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, -1);
			swkbdSetHintText(&swkbd, "Enter ASCII text");
			swkbdInputText(&swkbd, textbuf, sizeof(textbuf));
			libssh2_channel_write(channel, textbuf, sizeof(textbuf));
			textbuf = "";
		case KEY_B:
			libssh2_channel_write(channel, "\b", 1);
		case KEY_Y:
			libssh2_channel_write(channel, ENTER_BUF, 1);
		case KEY_X:
			continue;
		case KEY_UP:
			libssh2_channel_write(channel, UP_BUF, 4);
		case KEY_DOWN:
			libssh2_channel_write(channel, DOWN_BUF, 4);
		case KEY_RIGHT:
			libssh2_channel_write(channel, RIGHT_BUF, 4);
		case KEY_LEFT:
			libssh2_channel_write(channel, LEFT_BUF, 4);
		default:
			//this should never happen
			continue;
		}
		}
	}
//

	if(session) {
		libssh2_session_disconnect(session, "Normal Shutdown");
		libssh2_session_free(session);
	}
	
	if(sock != LIBSSH2_INVALID_SOCKET) {
		shutdown(sock, 2);
		LIBSSH2_SOCKET_CLOSE(sock);
	}
	 
	fprintf(stderr, "all done\n");
	 
	libssh2_exit();

	gfxExit();
	return 0;
}
