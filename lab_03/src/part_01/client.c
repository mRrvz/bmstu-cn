#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include "settings.h"

int get_file_from_server(int sock, char fname[])
{
    char buf[BUF_SIZE];

    FILE *f = fopen(fname, "w");
    if (f == NULL)
    {
        return EXIT_FAILURE;
    }

    int get_bytes = 0;
    while (buf[get_bytes] != EOF)
    {
        get_bytes = recv(sock, buf, BUF_SIZE, 0);

        #ifdef DEBUG
        printf("Get bytes: %d | %d %c | %d %c\n",
            get_bytes, buf[get_bytes - 1], buf[get_bytes - 1], buf[get_bytes - 2], buf[get_bytes - 2]);
        #endif

        if (buf[get_bytes - 1] == EOF)
        {
            get_bytes -= 1;
        }

        fwrite(buf, sizeof(char), get_bytes, f);
    }

    fclose(f);
    chmod(fname, 0777);
    return OK;
}

int main(void)
{
    setbuf(stdout, NULL);

    const int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        return EXIT_FAILURE;
    }

    struct sockaddr_in addr =
    {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(SOCKET_PORT)
    };

    if (connect(sock, (struct sockaddr *) &addr, sizeof addr) < 0)
    {
        return EXIT_FAILURE;
    }

    fprintf(stdout, "Connection has been established...\n\n");

    char buf[BUF_SIZE];
    if (recv(sock, buf, BUF_SIZE, 0) < 0)
    {
        return EXIT_FAILURE;
    }

    fprintf(stdout, "Please, enter the name of the file you want to retrieve:\n");
    fprintf(stdout, buf);

    if (fscanf(stdin, "%s", buf) != 1)
    {
        return EXIT_FAILURE;
    }

    if (sendto(sock, buf, strlen(buf), 0, (struct sockaddr *) &addr, sizeof addr) < 0)
    {
        return EXIT_FAILURE;
    }

    return get_file_from_server(sock, buf);
}
