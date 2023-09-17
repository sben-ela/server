/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sben-ela <sben-ela@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/09/17 11:36:51 by sben-ela          #+#    #+#             */
/*   Updated: 2023/09/17 14:39:49 by sben-ela         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Response.hpp"

#define FILESIZE 10

std::string	CgiExecute(const Client &client)
{
	int fd[2];
    ssize_t bytesRead;
    std::string output;
    char buffer[1024];
	char *Path[3] = {(char *)"/usr/bin/php-cgi", (char *)client.GetPath().c_str(), NULL};

	pipe(fd);
	int pid  = fork();
	if (!pid)
	{
		dup2(fd[1], 1);
		execve(Path[0], Path, 0);
		std::cout << "ERRRRORRR" << std::endl;
        exit(0);
	}
	waitpid(pid, 0, 0);
    close(fd[1]);
    while ((bytesRead = read(fd[0], buffer, sizeof(buffer))) > 0)
        output.append(buffer, bytesRead);
	return(output);
}


void ft_putstr(char *str)
{
    int i = 0;
    while (i < BUFFER_SIZE)
    {
        if (write (1 , &str[i], 1) < 0)
        {
            std::cout << "putstr failed : " << str[i] << std::endl;
            exit (0);
        }
        i++;
    }
}

void    Get(const Client &client)
{
    char buff[BUFFER_SIZE];
    std::string header;
    int fd;

    header = client.GetHttpVersion() + " 200 OK\r\nContent-Type: "
    + client.GetConetType() + "\r\ncontent-length: 54" + "\r\n\r\n";

    send(client.GetSocketId(), header.c_str(), header.size(), 0);
        // throw(std::runtime_error("Send Failed"));/
    if (client.GetFileExtention() == ".php")
    {
        std::string out = CgiExecute(client);;
        int rd = write(client.GetSocketId(), out.c_str(), out.size());
        std::cout << rd << std::endl;
        std::cout << "i am here " << std::endl;
        return ;
    }
    fd = open (client.GetPath().c_str(), O_RDONLY);
    if (fd < 0)
        throw(std::runtime_error("open Path in Get failed"));
    while (read(fd, buff, BUFFER_SIZE) > 0)
        if (write (client.GetSocketId() , buff, BUFFER_SIZE) < 0)
            throw(std::runtime_error("Write buff in Get Failed"));
    close (fd);
}

std::string GenerateFile( void )
{
    std::string Base = "ABCDEFJHIGKLMNOPQRSTUVWXYZabcdefjhiGklmnopqrstuvwxyz+-_";
    std::string file;
    
    for (size_t i = 0; i < FILESIZE; i++)
    {
        file += Base[rand() % Base.size()];
    }
    return(file);
}

// void    Post(const Client& client, std::string Content)
// {

// }

void    Response(const Client &client)
{
    
    if (client.GetMethod() == "GET")
        Get(client);
    // else if (client.GetMethod() == "POST")
    //     Post();
    // else if (client.GetMethod() == "DELETE")
    //     Delete();
}

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>


int main() {

    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the specified address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return 1;
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        return 1;
    }

    // std::cout << "Listening on port " << PORT << "..." << std::endl;

    // Accept a new connection
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        perror("Accept failed");
        return 1;
    }
    try
    {
        Response(Client("GET" , "test.php", ".php",  "text/plain", "HTTP/1.1 ", new_socket));
    }
    catch(std::exception &e){
        std::cout << "Error : " << e.what() << std::endl;
    }
}