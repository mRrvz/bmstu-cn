#include <fstream>
#include <string>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "thread_pool.hpp"

#define MSG_LEN 500
#define SOCK_ADDR "localhost"
#define SOCK_PORT 9999

#define MAX_CLIENTS 10

#define HTML_NOT_FOUND "<html>\n\r<body>\n\r<h1>404 Not Found</h1>\n\r</body>\n\r</html>"

int clients[MAX_CLIENTS] = { 0 };

std::string get_extention(std::string filename)
{
    int pos = filename.rfind('.');
    if (pos > 0)
    {
        return filename.substr(pos + 1);
    }

    return "";
}

void log(std::string user, std::string ext)
{
    std::ofstream fout("statistics.txt", std::ios::app);
    fout << "User \"" + user + "\" requested ." + ext + " file\n";
}

int handle_request(char *msg, int client_id)
{
    std::string debug(msg);
    char *method = strtok(msg, " ");
    char *filename = strtok(NULL, " /");
    char *version = strtok(NULL, " \r\n");
    char *tag = strtok(NULL, "\r\n:");
    char user[50];

    if (tag && !strcmp(tag, "User"))
    {
        strcpy(user, strtok(NULL, " \r\n"));
    }
    else
    {
        strcpy(user, "Unknown");
    }

    if (strcmp(method, "GET"))
    {
        return EXIT_FAILURE;
    }

    int rc = 0;
    std::string status, status_code;
    std::string body = "";
    std::string content_type = "text/html";

    std::string response(version);
    std::ifstream fin("" + std::string(filename));

    if (fin.is_open())
    {
        std::string ext = get_extention(filename);
        log(user, ext);

        if (ext == "mp3")
        {
            content_type = "audio/mpeg";
        }
        else if (ext == "ico")
        {
            content_type = "image/x-icon";
        }

        std::string buf;
        while (std::getline(fin, buf))
        {
            body += buf + "\n";
        }

        status_code = "200";
        status = "OK";
    }
    else
    {
        rc = -2;
        status_code = "404";
        status = "Not Found";
        body = HTML_NOT_FOUND;
    }

    response.append(" " + status_code + " " + status + "\r\n");
    response.append("Content-Length: " + std::to_string(body.length()) + "\r\n");
    response.append("Connection: closed\r\n");
    response.append("Content-Type: " + content_type + "; charset=UTF-8\r\n");
    response.append("\r\n");
    response.append(body);

    send(clients[client_id], response.c_str(), response.size(), 0);

    if (rc)
    {
        std::cout << "Client " <<  client_id << " has sent an invalid message" << std::endl;
    }
    else
    {
        std::cout << "Successfully handled a request from client " << client_id << std::endl;
    }

    return rc;
}

void handle_new_connection(unsigned int socketfd)
{
    struct sockaddr_in client_addr;
    int addrSize = sizeof(client_addr);

    int clientfd = accept(socketfd, (struct sockaddr*) &client_addr, (socklen_t*) &addrSize);
    if (clientfd < 0)
    {
        exit(1);
    }

    std::cout << std::endl << "New connection: " << std::endl
        << "Client socket fd = " << clientfd << std::endl
        << "ip = " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << std::endl;

    int i = 0;
    while (i < MAX_CLIENTS && clients[i])
    {
        i++;
    }

    if (i < MAX_CLIENTS)
    {
        clients[i] = clientfd;
        std::cout << "Connected client " << i << std::endl << std::endl;
    }
}

void remove_client(int fd, int id)
{
    struct sockaddr_in client_addr;
    int addr_size = sizeof(client_addr);

    getpeername(fd, (struct sockaddr*) &client_addr, (socklen_t*) &addr_size);
    std::cout << "Client " << id << " has disconnect (ip " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << ")" << std::endl;
    close(fd);
    clients[id] = 0;
}

int main(void)
{
    ThreadPool threads;
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener < 0)
    {
        return listener;
    }

    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(SOCK_PORT);
    client_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listener, (struct sockaddr*) &client_addr, sizeof(client_addr)) < 0)
    {
        return EXIT_FAILURE;
    }

    std::cout << "Server is listening on the " << SOCK_PORT << " port" << std::endl;

    if (listen(listener, MAX_CLIENTS) < 0)
    {
        return EXIT_FAILURE;
    }

    std::cout << "Waiting for the connections" << std::endl;

    while (1)
    {
        fd_set readfds;
        int max_fd;
        int active_clients_count;

        FD_ZERO(&readfds);
        FD_SET(listener, &readfds);
        max_fd = listener;

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            int fd = clients[i];

            if (fd > 0)
            {
                FD_SET(fd, &readfds);
            }

            if (fd > max_fd)
            {
                max_fd = fd;
            }
        }

        active_clients_count = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (active_clients_count < 0 && (errno != EINTR))
        {
            return active_clients_count;
        }

        if (FD_ISSET(listener, &readfds))
        {
            handle_new_connection(listener);
        }

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i] > 0 && FD_ISSET(clients[i], &readfds))
            {
                char msg[MSG_LEN];

                int msg_size = recv(clients[i], msg, MSG_LEN, 0);
                if (msg_size == -1)
                {
                    return EXIT_FAILURE;
                }
                else if (msg_size == 0)
                {
                    remove_client(clients[i], i);
                }
                else
                {
                    threads.add(handle_request, msg, i);
                }
            }
        }
    }

    return 0;
}