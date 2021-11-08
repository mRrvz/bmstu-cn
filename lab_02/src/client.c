#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "utils.h"

int main(void)
{
    fprintf(stdout, "Client started...\n\n");
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1)
    {
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_aton(SERVER_IP, &server_addr.sin_addr) == 0)
    {
        return -1;
    }

    char msg[128];
    fprintf(stdout, "Input message: ");
    fscanf(stdin, "%s", msg);
    if (sendto(sock, msg, 128, 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        return -1;
    }

    close(sock);

    return 0;
}