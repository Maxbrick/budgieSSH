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
#include <jimmy.h>
#define SOC_ALIGN       0x1000
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
    SwkbdState swkbd;
    uint32_t host_addr_int;

    //should probably change this to not a char array
    char ssh_port[8];

    int auth_pw = 0;
    int rc;
    int sock;
    char username[60];
    char password[60];
    char passphrase[256];
    char pubkey[1024];
    char privkey[128];
    char *userauthlist;
    LIBSSH2_SESSION *session = NULL;
    LIBSSH2_CHANNEL *channel;
    char host_addr[120];


    struct sockaddr_in sa;

    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);
    consoleDebugInit(debugDevice_CONSOLE);

    printf("Welcome to \x1b[0;32mbudgieSSH!\x1b[0;37m\n");

    //jimmy
    printf(jimmy);

    printf("\nA to input text, b for backsppace, x for tab,\nr for carriage return (for DOS/Windows)\ny for linefeed (Enter)\n");
    printf("\nWARNING: Extremely early alpha version!!!\nBugs will happen! Also, pubkey authentication\nprobably won\'t work.\n");
    printf("\nPlease only use for local networks with password \nauthentication.\n \n");

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
    // I'm not super smart with how to use keyboards
    swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, -1);
    swkbdSetHintText(&swkbd, "Username");
    swkbdInputText(&swkbd, username, sizeof(username));
    
    swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, -1);
    swkbdSetHintText(&swkbd, "Host Address");
    swkbdInputText(&swkbd, host_addr, sizeof(host_addr));
        
    swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 1, -1);
    swkbdSetHintText(&swkbd, "Port Number (usually 22)");
    swkbdInputText(&swkbd, ssh_port, sizeof(ssh_port));

    printf("Username: %s\n", username);
    printf("Host Address: %s\n", host_addr);

    host_addr_int = inet_addr(host_addr);

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

        if (sock < 0) {
        printf("couldn't make socket :(\n");
        }

    sa.sin_family = AF_INET;
    sa.sin_port = htons(atoi(ssh_port));
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


    //Authentication stuff (if it wasn't obvious by now a lot of this is just copied from libssh2's examples)
    /* check what authentication methods are available */ 
    userauthlist = libssh2_userauth_list(session, username, (unsigned int)strlen(username));
    if(userauthlist) {
        printf("Authentication methods: %s\n", userauthlist);
        if(strstr(userauthlist, "password")) {
            auth_pw = 1;
        } else
        if(strstr(userauthlist, "keyboard-interactive")) {
            auth_pw = 2;
        } else
        if(strstr(userauthlist, "publickey")) {
            auth_pw = 4;
        } 
        char stupid_buffer_im_lazy[2];
        swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 1, -1);
        swkbdSetHintText(&swkbd, "1 = password, 2 = keyboard interactive, 4 = publickey");
        swkbdInputText(&swkbd, stupid_buffer_im_lazy, sizeof(stupid_buffer_im_lazy));
        auth_pw = atoi(stupid_buffer_im_lazy);
        }
    if(auth_pw & 1) {
        /* We could authenticate via password */ 
        swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, -1);
        swkbdSetHintText(&swkbd, "Password");
        swkbdInputText(&swkbd, password, sizeof(password));
       
        if(libssh2_userauth_password(session, username, password)) {
            printf("Authentication by password failed.\n");
            goto shutdown;
        }
        else {
            printf("Authentication by password succeeded.\n");
        }
    }/* implement this later probably her her her
        else if(auth_pw & 2) {
            if(libssh2_userauth_keyboard_interactive(session, username,

                                                     &kbd_callback) ) {
                printf(
                        "Authentication by keyboard-interactive failed.\n");
                goto shutdown;
            }
            else {
                printf(
                        "Authentication by keyboard-interactive succeeded.\n");
            }
        }*/
        else if(auth_pw & 4) {
            /* Or by public key */ 
            
            swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, -1);
            swkbdSetHintText(&swkbd, "Enter name of the privkey file in sdmc:/3ds/ssh/");
            swkbdInputText(&swkbd, privkey, sizeof(privkey));

            printf("\n(Assuming that the pub key is %s.pub\n", privkey);

            snprintf(pubkey, (sizeof(privkey) + 4), "%s%s", privkey, ".pub");

            swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, -1);
            swkbdSetHintText(&swkbd, "Enter Passphrase");
            swkbdInputText(&swkbd, passphrase, sizeof(passphrase));
            size_t fn1sz, fn2sz;
            char *fn1, *fn2;

            char const *h = "./ssh";
            if(!h || !*h)
                h = ".";
            fn1sz = strlen(h) + strlen(pubkey) + 2;
            fn2sz = strlen(h) + strlen(privkey) + 2;
            fn1 = malloc(fn1sz);
            fn2 = malloc(fn2sz);
            if(!fn1 || !fn2) {
                free(fn2);
                free(fn1);
                fprintf(stderr, "out of memory\n");
                goto shutdown;
            }
            snprintf(fn1, fn1sz, "%s/%s", h, pubkey);
            snprintf(fn2, fn2sz, "%s/%s", h, privkey);
            
            printf(pubkey);
            printf(privkey);
            printf("\n%s,%s\n", fn1, fn2);

            //open pubkey and privkey and store them into a buffer, since I can't get the fromfile function that
            //lssh2 provides to work...
            printf("loading key files into memory...");
            char pubkey_buffer[256];
            char privkey_buffer[2048];
            FILE *pubkey_file = fopen(fn1, "r");
            FILE *privkey_file = fopen(fn2, "r");
            if (pubkey_file == NULL)
                printf("Error opening pubkey file %s :(\n", fn1);
            if (privkey_file == NULL)
                printf("Error opening privkey file %s :(\n", fn2);
            int i = 0, c;
            while ((c = fgetc(pubkey_file)) != EOF) {
                pubkey_buffer[i] = c;
                i++;
            }
            i = 0;
            while ((c = fgetc(privkey_file)) != EOF) {
                privkey_buffer[i] = c;
                i++;
            }
            printf("\nFiles loaded into memory\n");
            rc = libssh2_userauth_publickey_frommemory(
                session, username, sizeof(username), pubkey_buffer,
                sizeof(pubkey_buffer), privkey_buffer, sizeof(privkey_buffer), passphrase
            );
            if(rc) {
                printf("Authentication by pubkey from memory failed: %d\nAttempting from file...\n", rc);
            } else {
                printf("Authentication by pubkey succeeded\n");
            }
            rc = libssh2_userauth_publickey_fromfile_ex(
                session, username, sizeof(username), fn1, fn2, passphrase
            );
            if(rc) {
                printf("Authentication by privkey from file failed: %d\n", rc);
            } else {
                printf("Authentication by pubkey succeeded\n");
            }
    }
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
