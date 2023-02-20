/* CSC 3600 Spring 2023 Project 1
 * UDP Blaster
 * Joey Sachtleben
 */

// C++ Standard Library includes
#include <iostream>
#include <iomanip>
#include <string>
#include <set>
#include <cstdlib>
#include <stdexcept>

// C Standard Library and System libraries
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <memory.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>

// Local includes
#include "defaults.hpp"
#include "structure.hpp"

// Constants for errors
const int32_t UNKNOWN_ARGUMENT = 1;
const int32_t STD_EXCEPTION_THROWN = 2;
const int32_t UNKNOWN_EXCEPTION_THROWN = 3;

/* EstablishConnection
 * Responsible for opening a socket to the server.
 * Parameters:
 *   std::string serverAddress -- Address/IP of the server to connect to
 *   int16_t     port          -- Port to connect to
 *   bool        debug         -- Enable debug messages
 * Returns:
 *   An integer for the opened socket file descriptor.
 * Exceptions:
 *   Will throw an exception if there is an issue resolving the server name (i.e., the server address or port are invalid)
 *   or if the socket could not be opened.
 */
int EstablishConnection(std::string serverAddress, int16_t port, bool debug)
{
    // Set up the hints
    addrinfo hints;
    memset(&hints, 0, sizeof(hints)); // Do I need to do this since this is C++?
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    // Request the address info
    addrinfo* serverInfo;
    int rv = getaddrinfo(serverAddress.c_str(), std::to_string(port).c_str(), &hints, &serverInfo);
    if (rv != 0)
    {
        std::runtime_error ex(gai_strerror(rv));
        throw ex;
    }
}

int main(int argc, char* argv[])
{
    int retval = 0;
    int udpSocket = -1;
    int serverPort = PORT_NUMBER;
    std::string serverName = SERVER_IP;
    int32_t datagramsToSend = NUMBER_OF_DATAGRAMS;
    int32_t sendDelay = 0;
    bool debug = false;

    try
    {
        char c;
        while ((c = getopt(argc, argv, "dhs:p:n:y:")) != -1)
        {
            switch (c)
            {
            case 'd':
                debug = true;
                break;
            case 'h':
                std::cout << argv[0] << " (UDP Blaster) options:\n";
                std::cout << "-d           Enables debug output\n";
                std::cout << "-h           Displays this help and exit\n";
                std::cout << "-s [address] Set server address (default 127.0.0.1)\n";
                std::cout << "-p [port]    Set server port (default 39390)\n";
                std::cout << "-n [n]       Set number of datagrams to send (default 2^18)\n";
                std::cout << "-y [n]       Set delay in microseconds between datagrams (default 0)\n";
                throw 0; // Do I want to switch this to a proper exception and/or not throw here at all (ordering)?

            case 's':
                serverName = optarg;
                break;
            case 'p':
                serverPort = std::stoi(optarg);
                break;
            case 'n':
                datagramsToSend = std::stoi(optarg);
                break;
            case 'y':
                sendDelay = std::stoi(optarg);
                break;
            default:
                std::cerr << "Unknown argument encountered.\n";
                throw UNKNOWN_ARGUMENT; // Do I want to switch this to a proper exception?
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        retval = STD_EXCEPTION_THROWN;
    }
    catch (const int n)
    {
        retval = n;
    }
    catch (...)
    {
        std::cerr << "Unknown exception caught\n";
        retval = UNKNOWN_EXCEPTION_THROWN;
    }

    if (debug)
    {
        std::cout << "Ready to run with following settings: \n"
                  << "Server IP/address: " << serverName << "\n"
                  << "Server port: " << serverPort << "\n"
                  << "Number of datagrams: " << datagramsToSend << "\n"
                  << "Delay between datagrams: " << sendDelay << "\n";
    }

    if (udpSocket >= 0)
    {
        close(udpSocket);
    }

    return retval;
}
