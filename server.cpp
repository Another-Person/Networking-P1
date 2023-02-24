/* CSC 3600 Spring 2023 Project 1
 * UDP Blaster -- Server
 * Joey Sachtleben
 */

// C/C++ Standard Libraries
#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>

// System libraries
#ifdef	 __linux__
#include <unistd.h>
#endif

#include <getopt.h>
#include <memory.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>

// Local includes
#include "defaults.hpp"
#include "structure.hpp"

// Constants for errors
const int32_t SETUP_ERROR = 1;
const int32_t NETWORKING_ERROR = 4; // to match with the client

/* EstablishConnection
 * Responsible for opening the server the socket will be bound to.
 * Parameters:
 *   uint16_t port  -- Port to bind to
 *   bool     debug -- Enable debug messages
 * Returns:
 *   An integer for the opened socket file descriptor.
 * Exceptions:
 *   Will throw an exception if there is an issue opening the socket and binding to it.
 */
int EstablishConnection(uint16_t port, bool debug)
{
    if (debug)
    {
        std::cout << "Entering EstablishConnection...\n\n"
                  << "Preparing to create hints\n";
    }

    // Set up the hints
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if (debug)
    {
        std::cout << "Created hints.\n\n"
                  << "Preparing to request address info for localhost\n";
    }

    // Request the address info
    addrinfo* serverInfo;
    int rv = getaddrinfo(0, std::to_string(port).c_str(), &hints, &serverInfo);
    if (rv != 0)
    {
        freeaddrinfo(serverInfo);
        std::runtime_error ex(gai_strerror(rv));
        throw ex;
    }

    if (debug)
    {
        std::cout << "Successfully recieved address info.\n\n"
                  << "Preparing to create and attempt to connect to socket\n";
    }

    // Loop through the returned addresses and validate them by trying to connect to them
    int socketFD = -1;
    for (addrinfo* currentAddress = serverInfo; currentAddress != nullptr; currentAddress = currentAddress->ai_next)
    {
        // Attempt to create socket
        if ((socketFD = socket(currentAddress->ai_family, currentAddress->ai_socktype, currentAddress->ai_protocol)) == -1)
        {
            /* Explain why the socket creation failed (if debugging is enabled).
             * This is not fatal since we have multiple addrinfo's to go through
             */
            if (debug)
            {
                perror("Error creating socket");
            }
            continue;
        }

        // Attempt to bind to to the socket
        if (bind(socketFD, currentAddress->ai_addr, currentAddress->ai_addrlen) == -1)
        {
            /* Explain why the connection attempt failed (if debuggign is enabled).
             * This is not fatal since we have multiple addrinfo's to go through
             */
            if (debug)
            {
                perror("Error connecting");
            }
            close(socketFD);
            continue;
        }

        // Getting this far means that we must have connected, so no need to keep iterating
        if (debug)
        {
            std::cout << "Successfully bound to localhost!\n";
        }

        break;
    }

    if (debug)
    {
        std::cout << "Finished attempting to open and connect to socket.\n\n";
    }

    /* Since we either connected to an address or failed on all of them, we don't need the
     * linked list anymore. As such, we free it up here.
     */
    freeaddrinfo(serverInfo);

    // Check to see if our socket is actually open
    if (socketFD == 0)
    {
        std::runtime_error ex("Unable to open socket");
        throw ex;
    }

    if (debug)
    {
        std::cout << "Ready to return socket!\n";
    }

    return socketFD;
}

int main(int argc, char* argv[])
{
    int retval = 0;
    uint16_t port = PORT_NUMBER;
    bool debug = false;
    bool earlyStop = false;

    try
    {
        char c;
        while ((c = getopt(argc, argv, "dhp:")) != -1)
        {
            switch (c)
            {
            case 'd':
                debug = true;
                break;

            case 'h':
                std::cout << argv[0] << " (UDP Blaster Server) options: \n"
                          << "-d        Enable debug messages\n"
                          << "-h        Display this help and exit\n"
                          << "-p [port] Bind to the provided port (default 39390)\n";
                throw 0;

            case 'p':
                port = std::stoi(optarg);
                break;

            default:
                break;
            }
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        retval = SETUP_ERROR;
        earlyStop = true;
    }
    catch(const int n)
    {
        retval = n;
        earlyStop = true;
    }
    catch(...)
    {
        std::cerr << "Unknown exception caught\n";
        retval = SETUP_ERROR;
        earlyStop = true;
    }

    if (debug)
    {
        std::cout << "Server ready to run and bind to port " << port << "\n";
    }

    if (earlyStop)
    {
        return retval;
    }

    int socketFD = -1;
    try
    {
        socketFD = EstablishConnection(port, debug);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        retval = NETWORKING_ERROR;
    }
    catch(...)
    {
        std::cerr << "Unknown exception caught\n";
        retval = NETWORKING_ERROR;
    }

    if (socketFD >= 0)
    {
        close(socketFD);
    }


    return retval;
}
