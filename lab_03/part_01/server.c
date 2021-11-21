#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <arpa/inet.h>

#include "settings.h"

#define MAX_CLIENTS_COUNT 10

static int master_sd;
static int clients[MAX_CLIENTS_COUNT];

int cleanup()
{
    close(master_sd);
    exit(EXIT_FAILURE);
}

void sigint_handler(int signum)
{
    cleanup();
    exit(OK);
}

void list_directory_to_user(int sock, char dirname[])
{
    char buf[BUF_SIZE] = "\0";
    DIR *dh = opendir(dirname);
    struct dirent *entry;

    while ((entry = readdir(dh)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, ".."))
        {
            snprintf(buf + strlen(buf), BUF_SIZE - strlen(buf), "%s\n",  entry->d_name);
        }
    }

    write(sock, buf, strlen(buf));
}

int send_file(int sock, char fname[])
{
    FILE *f = fopen(fname, "r");
    if (f == NULL)
    {
        return EXIT_FAILURE;
    }

    fprintf(stdout, "Sending file \"%s\"...\n", fname);

    char buf[BUF_SIZE];
    int read_bytes;

    while ((read_bytes = fread(buf, sizeof(char), BUF_SIZE, f)) > 0)
    {
        if (read_bytes != BUF_SIZE)
        {
            buf[read_bytes++] = EOF;
        }

        if (write(sock, buf, read_bytes) < 0)
        {
            return EXIT_FAILURE;
        }
    }

    fclose(f);
    fprintf(stdout, "File \"%s\" sent!\n", fname);

    return OK;
}

void handle_connection(void)
{
    const int sock = accept(master_sd, NULL, NULL);
    if (sock == -1) {
        cleanup();
    }

    char buf[1024] = "\n";
    for (int i = 0; i < MAX_CLIENTS_COUNT; ++i)
    {
        if (!clients[i])
        {
            clients[i] = sock;
            list_directory_to_user(sock, ".");
            return;
        }
    }

    fprintf(stderr, "Reached MAX_CLIENTS_COUNT (%d)\n", MAX_CLIENTS_COUNT);
    cleanup();
}

int handle_client(int i)
{
    char fname[BUF_SIZE];
    const ssize_t bytes = recv(clients[i], &fname, BUF_SIZE, 0);

    if (!bytes)
    {
        close(clients[i]);
        clients[i] = 0;
        return OK;
    }

    fname[bytes] = '\0';

    return send_file(clients[i], fname);
}

int main(void)
{
    setbuf(stdout, NULL);

    if ((master_sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        return EXIT_FAILURE;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(SOCKET_PORT)
    };

    if (bind(master_sd, (struct sockaddr *) &addr, sizeof addr) < 0)
    {
        cleanup();
    }

    if (listen(master_sd, MAX_CLIENTS_COUNT) < 0)
    {
        cleanup();
    }

    signal(SIGINT, sigint_handler);
    fprintf(stdout, "Server is listening.\nTo stop server press Ctrl + C.\n\n");

    while (1)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(master_sd, &readfds);

        int max_sd = master_sd;

        for (int i = 0; i < MAX_CLIENTS_COUNT; ++i)
        {
            if (clients[i] > 0)
            {
                FD_SET(clients[i], &readfds);
            }

            if (clients[i] > max_sd)
            {
                max_sd = clients[i];
            }
        }

        if (pselect(max_sd + 1, &readfds, NULL, NULL, NULL, NULL) < 0)
        {
            cleanup();
        }

        if (FD_ISSET(master_sd, &readfds))
        {
            handle_connection();
        }

        for (int i = 0; i < MAX_CLIENTS_COUNT; ++i)
        {
            if (clients[i] && FD_ISSET(clients[i], &readfds))
            {
                if (handle_client(i) != 0)
                {
                    cleanup();
                    return EXIT_FAILURE;
                }
            }
        }
    }
}
