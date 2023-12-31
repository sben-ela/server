/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Servers.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aybiouss <aybiouss@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/09/04 13:11:31 by aybiouss          #+#    #+#             */
/*   Updated: 2023/11/04 13:44:38 by aybiouss         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../Includes/Servers.hpp"

Servers::Servers() {}

Servers::Servers(const Servers& other)
{
    *this = other;
}

Servers& Servers::operator=(const Servers& other)
{
    if (this != &other)
    {
        _servers = other._servers;
        _client = other._client;
    }
    return *this;
}


int Servers::ConfigFileParse(std::string file)
{
    std::ifstream File(file.c_str());
    if (!File.is_open())
    {
        std::cerr << "Error: Unable to open configuration file." << std::endl;
        return 1;
    }

    std::string line;
    bool insideServerBlock = false;
    std::vector<std::string> block;
    std::stack<char> blockStack; // Stack to keep track of nested blocks
    while (std::getline(File, line))
    {
        if (line == "server")
        {
            insideServerBlock = true;
            continue; // Skip the "server" line
        }
        if (insideServerBlock)
        {
            if (line == "{")
            {
                blockStack.push('{'); // Push a '{' for nested blocks
                continue;             // Skip the opening curly brace
            }
            else if (line == "}")
            {
                blockStack.pop(); // Pop a '{' to close the current block
                if (blockStack.empty())
                {
                    insideServerBlock = false; // Stop when we encounter the closing curly brace
                    Configuration config(block);
                    _servers.push_back(config);
                    block.clear(); // Clear the block for the next server
                }
            }
        }
        bool nospace = false;
        for (size_t i = 0; i < line.length(); ++i)
        {
            if (!std::isspace(static_cast<unsigned char>(line[i])))
            {
                nospace = true;
            }
        }
        if (nospace)
            block.push_back(line); // If any non-whitespace character is found
    }
    File.close();
    // printServerData();
    if (_servers.size() > 1)
        checkServers();
    AllServers();
    return 0;
}

void Servers::checkServers()
{
    std::vector<Configuration>::iterator it1;
    std::vector<Configuration>::iterator it2;
    for (it1 = this->_servers.begin(); it1 != this->_servers.end() - 1; it1++)
    {
        for (it2 = it1 + 1; it2 != this->_servers.end(); it2++)
        {
            if (it1->getPort() == it2->getPort() && it1->getHost() == it2->getHost() && it1->getServerNames() == it2->getServerNames())
                throw std::string("Failed server validation");
        }
    }
}

void Servers::printServerData() const
{
    for (std::vector<Configuration>::const_iterator it = _servers.begin(); it != _servers.end(); ++it)
    {
        std::cout << *it << std::endl;
    }
}

int Servers::AllServers()
{
    bool conditions = true;
    int maxFd = 2;   // will store the maximum file descriptor value for use in select()
    fd_set read_fds; // fd_set is a data structure used to manage file descriptors for I/O operations.
                     //  Fill up a fd_set structure with the file descriptors you want to know when data comes in on.
    int server_fd;
    
    fd_set write_fds;
    int yes = 1;
    std::vector<int> clientsocket;
    std::map<int, Configuration> serverSockets;
    std::vector<Configuration>  duplicated_servers;
    bool condition = false;
    for (std::vector<Configuration>::iterator it = _servers.begin(); it != _servers.end(); it++)
    {
        for (std::map<int, Configuration>::iterator its = serverSockets.begin(); its != serverSockets.end(); its++)
        {
            if (its->second.getHost() == it->getHost() && its->second.getPort() == it->getPort())
            {
                condition = true;
                it->_socketfd = its->second._socketfd;
                duplicated_servers.push_back(*it);
                break ;
            }
        }
        if (condition)
        {
            condition = false;
            continue ;
        }
        struct addrinfo hints, *p, *res; 
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        std::ostringstream s;
        s << it->getPort();
        if (getaddrinfo(it->getHost().c_str(), s.str().c_str(), &hints, &res) != 0)
        {
            std::cerr << "Error resolving hostname: " << it->getHost() << std::endl;
            continue;
        }
        for (p = res; p != NULL; p = p->ai_next)
        {
            if ((server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            {
                perror("server: socket");
                continue;
            }
            if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
            {
                perror("setsockopt");
                exit(1);
            }
            if (bind(server_fd, p->ai_addr, p->ai_addrlen) == -1)
            {
                ft_close(server_fd);
                perror("server: bind");
                continue;
            }
            break;
        }
        freeaddrinfo(res);
        if (p == NULL)
        {
            fprintf(stderr, "server: failed to bind\n");
            exit(1);
        }
        if (listen(server_fd, MAX_CLIENTS) < 0)
        {
            perror("Listen failed");
            exit(EXIT_FAILURE);
        } // listens for incoming connections on the server socket (server_fd)
        std::cout << "Listening on port " << it->getPort() << std::endl;
        if (server_fd > maxFd)
            maxFd = server_fd;
        it->_socketfd = server_fd;
        serverSockets[server_fd] = *it;
    }
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    for (std::map<int, Configuration>::iterator it = serverSockets.begin(); it != serverSockets.end(); it++)
    {
        if (it->first >= 0) {
            FD_SET(it->first, &read_fds);
        }
        else {
            std::cout << "FD_SET fails To add the it->first to read_fds" << std::endl;
            exit(20);
        }
    }
    while (true)
    {
        FD_CLR(0, &read_fds);
        FD_CLR(0, &write_fds);
        fd_set tmp_read = read_fds;
        fd_set tmp_write = write_fds;
        int readySockets = select(maxFd + 1, &tmp_read, &tmp_write, NULL, NULL);
        if (readySockets < 0)
        {
            for (int fd = 0; fd <= maxFd; fd++)
            {
                if (FD_ISSET(fd, &tmp_read) || FD_ISSET(fd, &tmp_write))
                {
                    if (!isOpen(fd))
                    {
                        if (FD_ISSET(fd, &tmp_read))
                            FD_CLR(fd, &read_fds);
                        else
                            FD_CLR(fd, &write_fds);
                    }
                }
            }
            continue;
        }
        for (std::map<int, Configuration>::iterator it = serverSockets.begin(); it != serverSockets.end(); it++)
        {
            if (FD_ISSET(it->first, &tmp_read))
            {
                Client new_client;
                sockaddr_in clientAddr;
                int clientSocketw;
                socklen_t addrlen = sizeof(clientAddr);
                if ((clientSocketw = accept(it->first, (struct sockaddr *)&clientAddr, (socklen_t *)&addrlen)) < 0) // is used to accept this incoming connection. It creates a new socket descriptor (new_socket) for this specific client connection. The client's address information is stored in address.
                {
                    perror("Error accepting connection");
                    continue;
                }
                if (clientSocketw > maxFd)
                    maxFd = clientSocketw;
                new_client.set_socket(clientSocketw);
                new_client.set_server(it->second);
                new_client._duplicated_servers = duplicated_servers;
                new_client._root = it->second.getRoot();
                new_client.initDefaultErrorPages();
                new_client._status = 0;
                _client.push_back(new_client);
                if (clientSocketw > 0)
                {
                    FD_SET(clientSocketw, &read_fds);
                }
            }
        }
        const std::string FAVICON_PATH = "/favicon.ico";
        for (std::vector<Client>::iterator its = _client.begin(); its != _client.end(); its++)
        {
            if (FD_ISSET(its->GetSocketId(), &tmp_read))
            {
                char buffer[1024] = {0};
                // Read the HTTP request from the client
                ssize_t bytesRead = recv(its->GetSocketId(), buffer, sizeof(buffer) - 1, 0);
                if (bytesRead < 0)
                {
                    perror("Error reading from socket");
                    ft_close(its->GetSocketId());
                    if (its->GetSocketId() == maxFd)
                        maxFd -= 1;
                    its = _client.erase(its);
                    its--;
                    FD_CLR(its->GetSocketId(), &read_fds);
                    continue;
                }
                else if (bytesRead == 0)
                {
                    ft_close(its->GetSocketId());
                    if (its->GetSocketId() == maxFd)
                        maxFd -= 1;
                    FD_CLR(its->GetSocketId(), &read_fds);
                    its = _client.erase(its);
                    its--;
                    continue;
                }
                else
                {
                    std::string buf(buffer, bytesRead);
                    // std::cout << buf << std::endl;
                    its->response._upload = its->getServer().getUpload();
                    its->response._client_max_body_size = its->getServer().getClientMaxBodySize();
                    if (!its->response.parseHttpRequest(buf))
                    {
                        FD_CLR(its->GetSocketId(), &read_fds);
                        FD_SET(its->GetSocketId(), &write_fds);                         
                    }
                    if (conditions)
                    {
                        for (std::vector<Configuration>::iterator it = duplicated_servers.begin(); it != duplicated_servers.end(); it++)
                        {
                            if (its->getServer().getPort() == it->getPort() && its->getServer().getHost() == it->getHost())
                            {
                                if (its->response._value == it->getServerNames())
                                {
                                    its->set_server(*it);
                                    break;
                                }
                            }
                        }
                        conditions = false;
                    }
                }
            }
        }
        for (std::vector<Client>::iterator its = _client.begin(); its != _client.end();)
        {
            conditions = true;
            if (FD_ISSET(its->GetSocketId(), &tmp_write))
            {
                its->_responseStatus = -2;
                if (its->_status == 0  || !its->_content_fd)
                    its->ft_Response();
                else if (its->_status == 1)
                    its->ft_send();
                else if (its->_status == CGI)
                {
                    its->_waitStatus = waitpid(its->_cgiPid, &its->_childExitStatus, WNOHANG);
                    if (its->_waitStatus > 0)
                    {
                        if (WIFEXITED(its->_childExitStatus) && WEXITSTATUS(its->_childExitStatus))
                            its->SendErrorPage(INTERNALSERVERERROR);
                        else if (WIFSIGNALED(its->_childExitStatus))
                            its->SendErrorPage(INTERNALSERVERERROR);
                        else if (its->_waitStatus == its->_cgiPid)
                        {
                            its->_content_fd = open (its->_CgiFile.c_str(), O_RDONLY);
                            if (its->_content_fd < 0)
                                its->SendErrorPage(INTERNALSERVERERROR);
                            else
                            {
                                its->_CgiHeader.clear();
                                its->readCgiHeader(its->_content_fd);
                                its->SendHeader(its->_content_fd);
                                its->_status = 1;
                            }
                        }
                    }
                    else if (its->_waitStatus == -1)
                        its->SendErrorPage(INTERNALSERVERERROR);
                    else if(std::time(NULL) - its->_cgiTimer >= TIMEOUT)
                    {
                        kill(its->_cgiPid , SIGTERM);
                        waitpid(its->_cgiPid, 0, 0);
                        its->_cgiPid = -1;
                        its->SendErrorPage(REQUESTTIMEOUT);
                    }
                }
                if (its->_responseStatus == -1 || its->_responseStatus == 0)
                {
                    FD_CLR(its->GetSocketId(), &write_fds);
                    ft_close(its->GetSocketId());
                    ft_close(its->_content_fd);
                    if (its->response.getFd() !=-1)
                        ft_close(its->response.getFd());
                    its = _client.erase(its);
                }
                else 
                    ++its;
            }
            else
                ++its;
        }
    } 
    return 0;
}