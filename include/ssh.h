#include <libssh2.h>
void budgiessh_prompt(struct sockaddr_in *sa);
void budgiessh_connect(LIBSSH2_SESSION *session, const struct sockaddr_in *sa, s32 sock);
void budgiessh_authenticate(LIBSSH2_SESSION *session);
