/* CSC 3600 Spring 2023 Project 1
 * UDP Blaster
 * Joey Sachtleben
 */

// C/C++ Standard Library includes
#include <iostream>
#include <iomanip>
#include <string>
#include <set>
#include <cstdlib>

// System libraries
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



int main(int argc, char* argv[])
{
    int retval = 0;
    int udpSocket = -1;
    int serverPort = PORT_NUMBER;
    std::string serverName = SERVER_IP;
    int32_t datagramsToSend = NUMBER_OF_DATAGRAMS;

    char c;
    while ((c = getopt(argc, argv, "dhs:p:n:y:")) != -1)
    {
        switch (c)
        {
        case 'h':
            /* code */
            break;

        default:
            break;
        }
    }

}
