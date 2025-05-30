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
#include <ssh.h>
#include <jimmy.h>
#define SOC_ALIGN	   0x1000
#define SOC_BUFFERSIZE  0x100000

static u32 *SOC_buffer = NULL;
s32 sock = -1, csock = -1;

//---------------------------------------------------------------------------------
void socShutdown() {
//---------------------------------------------------------------------------------
	printf("waiting for socExit...\n");
	socExit();
 
}

int main() {
	//init console
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);
	consoleDebugInit(debugDevice_CONSOLE);

	printf("Welcome to \e[32mbudgieSSH!\e[37m\n");
	//jimmy
	printf("%s", jimmy);

	printf("\nA to input text, b for backsppace, x for tab,\nr for carriage return (for DOS/Windows)\ny for linefeed (Enter)\n");
	printf("\nWARNING: Extremely early alpha version!!!\nBugs will happen! Also, pubkey authentication\nprobably won\'t work.\n");
	printf("\nPlease only use for local networks with password \nauthentication.\n\n");

	int rc;
	LIBSSH2_SESSION *session = NULL;
	LIBSSH2_CHANNEL *channel;

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

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

	session = libssh2_session_init();

	struct sockaddr_in sa;

	budgiessh_prompt(&sa);

	//sa.sin_family = AF_INET;
	//sa.sin_port = htons(atoi(ssh_port));
	//sa.sin_addr.s_addr = inet_addr(host_addr);
	

   	//if you want to disable libssh2 debugging, remove this line. Additionally you may compile libssh2 without debugging enabled.
	// libssh2_trace(session, ~0);

		
	budgiessh_connect(session, &sa, sock);
	budgiessh_authenticate(session);

	channel = libssh2_channel_open_session(session);
	printf("Openning libssh2 session...\n");
	if(!channel) {
		printf("Unable to open a session\n");
	}
	printf("Requesting pty..\n");
	if(libssh2_channel_request_pty(channel, "vt100")) {
		printf("Failed requesting pty\n");
	}

	printf("requesting shell..\n");
	if(libssh2_channel_shell(channel)) {
		printf("Unable to request shell on allocated pty\n");
	}
	libssh2_channel_set_blocking(channel, 0);

	//stop 2004h from appearing
	libssh2_channel_write(channel, "bind 'set enable-bracketed-paste off'\x0A", 39);

	// Main loop
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		gfxSwapBuffers();
		char buf[1024];
		char textbuf[128];

		//probably silly to store these as char arrays
		char enterbuf[2] = "\x0A";
		char upbuf[5] = "\x1b[1A";
		char downbuf[4] = "\x1b[1B";
		char rightbuf[4] = "\x1b[C";
		char leftbuf[4] = "\x1b[D";
		char scrupbuf[4] = "\x1b[S";
		char scrdownbuf[4] = "\x1b[T";
		char crbuf[2] = "\r";
		ssize_t err = libssh2_channel_read(channel, buf, sizeof(buf));

				//  fprintf(stderr, "Unable to read response: %ld\n", (long)err);
		if(err < 0) {
			//bullcrap filler statement bc
			//idk what im doing
			memset(textbuf, '\0', sizeof(textbuf));
		} else {
			fwrite(buf, 1, (size_t)err, stdout);
		}
		hidScanInput();
		u32 kDown = hidKeysDown();
		
		// loop through all possible button bits, check if
		// they are true, if so execute its function
		u32 i;
		SwkbdState swkbdl;
		for(i = 0; (i < 32); i++) {
		if(!(kDown & BIT(i))) continue;
		switch(BIT(i)) {
		case KEY_A:
			swkbdInit(&swkbdl, SWKBD_TYPE_NORMAL, 1, -1);
			swkbdSetHintText(&swkbdl, "Enter ASCII text");
			swkbdInputText(&swkbdl, textbuf, sizeof(textbuf));
			libssh2_channel_write(channel, textbuf, sizeof(textbuf));
			memset(textbuf, '\0', sizeof(textbuf));
			break;
		case KEY_B:
			libssh2_channel_write(channel, "\b", 1);
			break;
		case KEY_Y:
			libssh2_channel_write(channel, enterbuf, (sizeof(enterbuf) - 1));
			break;
		case KEY_X:
			libssh2_channel_write(channel, "\t", 1);
		case KEY_UP:
			libssh2_channel_write(channel, upbuf, strlen(upbuf));
			break;
		case KEY_DOWN:
			libssh2_channel_write(channel, downbuf, strlen(downbuf));
			break;
		case KEY_RIGHT:
			libssh2_channel_write(channel, rightbuf, strlen(rightbuf));
			break;
		case KEY_LEFT:
			libssh2_channel_write(channel, leftbuf, strlen(leftbuf));
			break;
		case KEY_L:
			libssh2_channel_write(channel, scrupbuf, strlen(scrupbuf));
			break;
		case KEY_R:
			libssh2_channel_write(channel, crbuf, strlen(crbuf));
			break;
		default:
			continue;
		}
		}
		if(kDown & KEY_START)
			// goto shutdown;
			break;
	}
shutdown:

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
