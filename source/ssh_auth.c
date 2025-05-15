#include <stdio.h>
#include <string.h>
#include <libssh2.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <3ds.h>
#include <ssh.h>


void budgiessh_authenticate(LIBSSH2_SESSION *session) {
	SwkbdState swkbd;
    int auth_pw = 0;

	char username[128];
	char password[256];
	char privkey[4096]; //im sorry
	char pubkey[1024];
	char passphrase[256];

    swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, sizeof(username));
    swkbdSetHintText(&swkbd, "Enter Username...");
    swkbdInputText(&swkbd, username, sizeof(username));

	while(username[1] == NULL);
    //Authentication stuff (if it wasn't obvious by now a lot of this is just copied from libssh2's examples)
    /* check what authentication methods are available */ 
    char *userauthlist = libssh2_userauth_list(session, username, (unsigned int)sizeof(username));

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
        swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 1, sizeof(stupid_buffer_im_lazy));
        swkbdSetHintText(&swkbd, "1 = password, 2 = keyboard interactive, 4 = publickey");
        swkbdInputText(&swkbd, stupid_buffer_im_lazy, sizeof(stupid_buffer_im_lazy));
		while(stupid_buffer_im_lazy[1] == NULL);
        auth_pw = atoi(stupid_buffer_im_lazy);
        }
    if(auth_pw & 1) {
        /* We could authenticate via password */ 
        swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, sizeof(password));
        swkbdSetHintText(&swkbd, "Password");
        swkbdInputText(&swkbd, password, sizeof(password));
		while(password[1] == NULL);
       
        if(libssh2_userauth_password(session, username, password)) {
            printf("Authentication by password failed.\n");
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
            }
            else {
                printf(
                        "Authentication by keyboard-interactive succeeded.\n");
            }
        }*/
        else if(auth_pw & 4) {
            /* Or by public key */ 
            
            swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, sizeof(privkey));
            swkbdSetHintText(&swkbd, "Enter name of the privkey file in sdmc:/3ds/ssh/");
            swkbdInputText(&swkbd, privkey, sizeof(privkey));

            printf("\n(Assuming that the pub key is %s.pub\n", privkey);

            snprintf(pubkey, (sizeof(privkey) + 4), "%s%s", privkey, ".pub");

            swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 1, sizeof(passphrase));
            swkbdSetHintText(&swkbd, "Enter Passphrase");
            swkbdInputText(&swkbd, passphrase, sizeof(passphrase));
            size_t fn1sz, fn2sz;
            char *fn1, *fn2;

            char const *h = "./ssh";
            if(!h || !*h)
                h = ".";
            fn1sz = sizeof(h) + sizeof(pubkey) + 2;
            fn2sz = sizeof(h) + sizeof(privkey) + 2;
            fn1 = malloc(fn1sz);
            fn2 = malloc(fn2sz);
            if(!fn1 || !fn2) {
                free(fn2);
                free(fn1);
                fprintf(stderr, "out of memory\n");
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
            int rc = libssh2_userauth_publickey_frommemory(
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
}
