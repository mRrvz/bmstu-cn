#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "utils.h"

int convert_num(char *result, long n, const int base)
{
    char symbols[] = "0123456789ABCDEF";
    int mod = n % base;

    n /= base;
    if (n == 0)
    {
        result[0] = symbols[mod];
        return 1;
    }

    int k = convert_num(result, n, base);
    result[k++] = symbols[mod];
    return k;
}

void print_converted_message(char *const msg)
{
    long num = atol(msg);
    char binary[128];
    char twelve[128];

    convert_num(binary, num, 2);
    convert_num(twelve, num, 12);

    fprintf(stdout, "DEC: %ld\nBIN: %s\nOCT: %lo\nHEX: %lx\nTWELVE: %s\n", num, binary, num, num, twelve);
}

int main(void)
{
    fprintf(stdout, "Server started...\n");
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1)
    {
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        return -1;
    }

    char msg[128];
    struct sockaddr_in client_addr;
    unsigned int slen = sizeof(client_addr);

    while (1)
    {
        if (recvfrom(sock, msg, 128, 0, (struct sockaddr *)&client_addr, &slen) == -1)
        {
            return -1;
        }

        if (strcmp(msg, "stop\n") == 0)
        {
            fprintf(stdout, "Aborting\n");
            close(sock);
            return 0;
        }

        fprintf(stdout, "=========== MESSAGE FROM CLIENT ===========\n");
        print_converted_message(msg);
    }

    return 0;
}