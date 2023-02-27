/* CSC 3600 Spring 2023 Project 1
 * UDP Blaster -- Client
 * Joey Sachtleben
 */

// C++ Standard Library includes
#include <iostream>
#include <iomanip>
#include <string>
#include <set>
#include <cstdlib>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <new>

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

// Convinience type aliases & using statements
using US = std::chrono::microseconds;

// Global constants
const std::string PAYLOAD = "jsachtleben";

// Constants for errors
const int32_t UNKNOWN_ARGUMENT = 1;
const int32_t SETUP_ERROR = 2;
const int32_t NETWORKING_ERROR = 3;

/* EstablishConnection
 * Responsible for opening a socket to the server.
 * Parameters:
 *   std::string serverAddress -- Address/IP of the server to connect to
 *   uint16_t    port          -- Port to connect to
 *   bool        debug         -- Enable debug messages
 * Returns:
 *   An integer for the opened socket file descriptor.
 * Exceptions:
 *   Will throw an exception if there is an issue resolving the server name (i.e., the server address or port are invalid)
 *   or if the socket could not be opened and connected to.
 */
int EstablishConnection(std::string serverAddress, uint16_t port, bool debug)
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

    if (debug)
    {
        std::cout << "Created hints.\n\n"
                  << "Preparing to request address info on " << serverAddress << "\n";
    }


    // Request the address info
    addrinfo* serverInfo;
    int rv = getaddrinfo(serverAddress.c_str(), std::to_string(port).c_str(), &hints, &serverInfo);
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

        // Attempt to connect to to the socket
        if (connect(socketFD, currentAddress->ai_addr, currentAddress->ai_addrlen) == -1)
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
            std::cout << "Successfully connected to " << serverAddress << "!\n";
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
        std::cout << "Attempting to set socket to nonblocking IO\n";
    }


    // Since the socket is open, set it to be nonblocking
    if (fcntl(socketFD, F_SETFL, O_NONBLOCK) == -1)
    {
        close(socketFD);
        std::runtime_error ex(strerror(errno)); // This function is not thread safe, if I end up needing to thread later
        throw ex;
    }

    if (debug)
    {
        std::cout << "Set socket to nonblocking IO.\n\n"
                  << "Ready to return socket\n\n";
    }


    // At long last, a prepared socket, ready to be returned:
    return socketFD;
}

/* SendAndRecieve
 * Main loop for sending and recieving packets. Keeps track of sequence numbers sent and recieved and displays
 * the final results. Will inform the users of any errors that occur, such as incorrect # of bytes sent or unknown
 * sequence numbers recieved.
 * Parameters:
 *   int      socketFD         -- File descriptor for socket as prepared by EstablishConnection
 *   uint32_t datagramsToSend  -- Number of packets to send
 *   US       delay            -- Time in us to wait between sending packets
 *   bool     debug            -- Enable debug messages
 * Returns:
 *   Nothing.
 * Exceptions:
 *   Will thow an exception if there is an error during any memory allocation or if a standard function throws.
 */
void SendAndRecieve(int socketFD, uint32_t datagramsToSend, US delay, bool debug)
{
    std::set<uint32_t> sentIDs;

    if (debug)
    {
        std::cout << "Entering SendAndRecieve...\n\n";
    }


    for (uint32_t i = 0; i < datagramsToSend; i++)
    {
        // Sending data

        if (debug)
        {
            std::cout << "Preparing to send packet " << i << "\n";
        }


        ClientDatagram prepDG = {i, static_cast<uint16_t>(PAYLOAD.size())}; // prepDG holds the info but not the payload

        if (debug)
        {
            std::cout << "Prep DG made, sequence # " << prepDG.sequence_number << " and payload size "
                      << prepDG.payload_length << "\n";
        }

        ssize_t datagramSize = sizeof(ClientDatagram) + prepDG.payload_length + 1; // add one to account for null byte
        ClientDatagram* realDG = static_cast<ClientDatagram*>(malloc(datagramSize));
        if (realDG == 0)
        {
            std::bad_alloc ex;
            throw ex;
        }

        realDG->sequence_number = htonl(prepDG.sequence_number);
        realDG->payload_length = htons(prepDG.payload_length);
        /* The next statement is... complicated.
         * The pointer arithmetic takes the pointer to realDG and moves it to point to the byte past the "end" of the
         * datagram struct. This is the designated space for the payload.
         * We then copy the C string representation of the payload into this space. The size of the payload is used to
         * make certain we only write the intended number of bytes.
         */
        //strncpy((reinterpret_cast<char*>(realDG) + sizeof(ClientDatagram)),
        strncpy((reinterpret_cast<char*>(realDG + 1)),
                PAYLOAD.c_str(), PAYLOAD.size() + 1);
        // The realDG is now ready to be sent!

        if (debug)
        {
                std::cout << "Real DG made and ready to be sent!\n";
        }


        std::this_thread::sleep_for(delay);
        ssize_t sentBytes = send(socketFD, static_cast<void*>(realDG), datagramSize, 0);
        if (sentBytes == -1)
        {
            std::cerr << "Error sending on socket\n";
            perror("send()");
        }
        else if (sentBytes != datagramSize)
        {
            std::cerr << "send() error: " << datagramSize << " bytes were requested to be sent, but " << sentBytes
                      << " were actually sent!\n";
        }

        if (debug)
        {
            std::cout << "Asked to send " << datagramSize << " bytes, sent " << sentBytes << " bytes\n"
                      << "Inserting sequence number " << prepDG.sequence_number << "\n\n";
        }
        sentIDs.insert(prepDG.sequence_number);
        free(realDG);

        // Reciving data
        ServerDatagram* serverDG = static_cast<ServerDatagram*>(malloc(sizeof(ServerDatagram)));
        if (serverDG == 0)
        {
            std::bad_alloc ex;
            throw ex;
        }

        const size_t RECIEVE_ATTEMPTS = 8;
        ssize_t recvBytes = 0;
        for (size_t i = 0; i < RECIEVE_ATTEMPTS; i++)
        {
            recvBytes = recv(socketFD, static_cast<void *>(serverDG), sizeof(ServerDatagram), 0);
            if (debug)
            {
                if (recvBytes == -1 && (errno != EAGAIN || errno != EWOULDBLOCK))
                {
                    std::cerr << "Error reading from socket\n";
                    perror("recv()");
                    continue;
                }
                else if (recvBytes == 0)
                {
                    std::cerr << "Socket closed unexpectedly\n";
                    continue;
                }
            }
            if (recvBytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                continue;
            }

            // We must have recieved something, if we get here, so break
            break;
        }
        if (recvBytes <= 0)
        {
            free(serverDG);
            continue;
        }


        if (debug)
        {
            std::cout << "Expected to recieve " << sizeof(ServerDatagram) << " bytes, recieved " << recvBytes
                      << "bytes\n";
        }

        // I suppose we're assuming that something was read here... Not anymore?
        ServerDatagram data = {ntohl(serverDG->sequence_number), ntohs(serverDG->datagram_length)};

        if (debug)
        {
            std::cout << "Recieved data: sequence number " << data.sequence_number << ", length "
                      << data.datagram_length << "\n"
                      << "Searching for recieved sequence number...\n";
        }

        if (data.datagram_length != static_cast<uint16_t>(datagramSize))
        {
            std::cout << "Sequence number " << data.sequence_number << " reports that " << data.datagram_length
                      << " bytes were sent, but we (expected to) send " << datagramSize << " bytes!\n";
        }

        auto recvID = sentIDs.find(data.sequence_number); // sorry I can't be bothered to get the type right
        if ( recvID == sentIDs.end())
        {
            std::cerr << "Recieved packet for unknown sequence ID " << serverDG->sequence_number << "!\n";
        }
        else
        {
            if (debug)
            {
                std::cout << "Found sequence number " << data.sequence_number << " and removing from set\n\n";
            }

            sentIDs.erase(recvID);
        }
        free(serverDG);
    }

    std::cout << datagramsToSend << " messages sent.\n"
              << "Unacknowledged packets: " << sentIDs.size() << "\n";


    if (debug)
    {
        std::cout << "Finished network transmission!\n\n";
    }


}

int main(int argc, char* argv[])
{
    int retval = 0;
    int udpSocket = -1;
    uint16_t serverPort = PORT_NUMBER;
    std::string serverName = SERVER_IP;
    uint32_t datagramsToSend = NUMBER_OF_DATAGRAMS;
    US sendDelay(0);
    bool debug = false;
    bool earlyStop = false;

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
                std::cout << argv[0] << " (UDP Blaster Client) options:\n"
                          << "-d           Enables debug output\n"
                          << "-h           Displays this help and exit\n"
                          << "-s [address] Set server address (default 127.0.0.1)\n"
                          << "-p [port]    Set server port (default 39390)\n"
                          << "-n [n]       Set number of datagrams to send (default 2^18)\n"
                          << "-y [n]       Set delay in microseconds between datagrams (default 0)\n";
                throw 0;

            case 's':
                serverName = optarg;
                break;
            case 'p':
                serverPort = std::stoi(optarg);
                break;
            case 'n':
                datagramsToSend = std::stoul(optarg);
                break;
            case 'y':
                sendDelay = US(std::stoi(optarg));
                break;
            default:
                std::cerr << "Unknown argument encountered.\n";
                throw UNKNOWN_ARGUMENT;
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        retval = SETUP_ERROR;
        earlyStop = true;
    }
    catch (const int n)
    {
        retval = n;
        earlyStop = true;
    }
    catch (...)
    {
        std::cerr << "Unknown exception caught\n";
        retval = SETUP_ERROR;
        earlyStop = true;
    }

    // If something went wrong earlier, don't even bother moving to the networking section
    if (earlyStop)
    {
        return retval;
    }

    std::cout << "Datagram length: " << sizeof(ClientDatagram) << " size of string: " << PAYLOAD.length() + 1
              << " total length: " << sizeof(ClientDatagram) + PAYLOAD.length() + 1 << "\n"
              << "Client attempting to connect to address " << serverName << " on port " << serverPort << "\n";


    if (debug)
    {
        std::cout << "Additional configuration information: \n"
                  << "Number of datagrams: " << datagramsToSend << "\n"
                  << "Delay between datagrams: " << sendDelay.count() << "us\n\n";
    }

    try
    {
        udpSocket = EstablishConnection(serverName, serverPort, debug);
        SendAndRecieve(udpSocket, datagramsToSend, sendDelay, debug);
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


    if (udpSocket >= 0)
    {
        close(udpSocket);
    }

    return retval;
}
