/* CSC 3600 Spring 2023 Project 1
 * UDP Blaster -- Server
 * Joey Sachtleben
 */

// C/C++ Standard Libraries
#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>

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


    return retval;
}
