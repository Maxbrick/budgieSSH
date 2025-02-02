#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>
#include <libssh2.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>


int main()
{
	int rc;
		

	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);

	printf("Welcome to 3DSSH!\n");

	rc = libssh2_init(0);
	if(rc) {
        	fprintf(stderr, "libssh2 initialization failed (%d)\n", rc);
        	return 1;
    	}
	
	swkbdInit(0, 0, 2, 255);


	// libssh2_session_handshake(rc);

	// Main loop
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		gfxSwapBuffers();
		hidScanInput();

		// Your code goes here
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu
	}

	gfxExit();
	return 0;
}
