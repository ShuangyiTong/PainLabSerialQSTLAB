#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include <chrono>
#include <queue>

// OS headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <termios.h> // Contains POSIX terminal control definitions
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 

#include "TcsII_device_descriptor.h"

class SimplePainlabProtocol
{
    int _sockfd = 0;
    char _recvBuff[4096];
    int _readOffset = 0;
    char _writeBuff[4096];
public:
    std::queue<std::string> _server_responses;
    uint32_t reverseWordEndianness(uint32_t src) { // use -O3 will get rid of the call stack
        uint32_t dest = 0;
        asm ("REV %0, %1"
            : "=r" (dest)
            : "r" (src));

        return dest;
    }

    int GetServerData()
    {
        int n = read(_sockfd, _recvBuff + _readOffset, 4096 - _readOffset);
        if (n == 0) {
            return 1;
        }
        int total_read = n + _readOffset;
        if (total_read >= 4)
        {
            uint32_t total_bytes = reverseWordEndianness(*(reinterpret_cast<uint32_t*>(_recvBuff)));
            if (total_read >= total_bytes + 4)
            {
                _server_responses.push(std::string(_recvBuff + 4, _recvBuff + 4 + total_bytes));
                // handling sticky packets
                int bytes_to_move = total_read - (total_bytes + 4);
                memmove(_recvBuff, _recvBuff + total_bytes + 4, bytes_to_move);
                _readOffset = 0;
                return 0;
            }
            else
            {
                _readOffset = total_read;
                return GetServerData();
            }
        }
        else
        {
            _readOffset = total_read;
            return GetServerData();
        }
    }

    int Connect(const char *server_ip, int server_port=8124) 
    {
        struct sockaddr_in serv_addr; 

        memset(_recvBuff, '0',sizeof(_recvBuff));
        if((_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            printf("\n Error : Could not create socket \n");
            return 1;
        } 

        memset(&serv_addr, '0', sizeof(serv_addr)); 

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(server_port); 

        if(inet_pton(AF_INET, server_ip, &serv_addr.sin_addr)<=0)
        {
            printf("\n inet_pton error occured\n");
            return 1;
        }

        if(connect(_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            printf("\n Error : Connect Failed \n");
            return 1;
        }
        else
        {
            std::cout << "Connected to server at " << server_ip << ":" << server_port << std::endl;
        }

        std::cout << "sending descriptor..." << std::endl;

        uint32_t data_length = reverseWordEndianness(strlen(descriptor));
        write(_sockfd, reinterpret_cast<char*>(&data_length), 4);
        int n = write(_sockfd, descriptor, strlen(descriptor));

        std::cout << "total " << n << " bytes descriptor sent to the server" << std::endl;

        GetServerData();

        std::cout << "Server responded: " << _server_responses.front() << std::endl;

        _server_responses.pop();

        return 0;
    }

    int SendData(const char* buff, int send_size)
    {
        uint32_t data_length = reverseWordEndianness((uint32_t)(send_size));
        write(_sockfd, reinterpret_cast<char*>(&data_length), 4);
        write(_sockfd, buff, send_size);

        return 0;
    }
};

struct DataFrame
{
    uint32_t timestamp;
    uint32_t temperature;
};

class TSAII_Serial_Connection
{
    int serial_port;
    const std::string silent = "F";
    const std::string get_temp = "E";
    std::chrono::system_clock::time_point start;
public:
    TSAII_Serial_Connection()
    {
        start = std::chrono::system_clock::now();
    }

    int Connect()
    {
        serial_port = open("/dev/ttyUSB0", O_RDWR);

        // Check for errors
        if (serial_port < 0) {
            printf("Error %i from open: %s\n", errno, strerror(errno));
            return 1;
        }

        // Create new termios struct, we call it 'tty' for convention
        // No need for "= {0}" at the end as we'll immediately write the existing
        // config to this struct
        struct termios tty;

        // Read in existing settings, and handle any error
        // NOTE: This is important! POSIX states that the struct passed to tcsetattr()
        // must have been initialized with a call to tcgetattr() overwise behaviour
        // is undefined
        if(tcgetattr(serial_port, &tty) != 0) {
            printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
            return 1;
        }

        tty.c_cc[VTIME] = 10;    // Wait for up to 10ms (1 decisecond) as in the Matlab example. Python example is 2 deciseconds
        tty.c_cc[VMIN] = 0;

        cfsetispeed(&tty, B115200);
        cfsetospeed(&tty, B115200);

        if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
            printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
            return 1;
        }
        write(serial_port, silent.c_str(), silent.size());
        tcflush(serial_port, TCIOFLUSH);

        std::cout << "TSAII connected" << std::endl;

        return 0;
    }

    int WriteSerial(std::string line)
    {
        int n = write(serial_port, line.c_str(), line.size());
        std::cout << line << std::endl;
        return n;
    }

    int PopulatingPostStimulationTemperatureData(DataFrame* dfs)
    {
        char read_buf [1024];
        int dfs_count = 0; 
        std::chrono::system_clock::time_point stimulation_start = std::chrono::system_clock::now();
        while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - stimulation_start).count() < 2000
               && dfs_count < 1000)
        {
            int write_bytes = write(serial_port, get_temp.c_str(), get_temp.size());
            if (write_bytes < 1)
            {
                std::cout << "Tcs device not responding: check serial connection" << std::endl;
                return dfs_count;
            }
            // Somehow the first a few bytes come from no where and is useless......
            int retry_count = 0;
            int num_bytes = read(serial_port, &read_buf, sizeof(read_buf));
            if (num_bytes < 24) 
            {
                continue;
            }
            dfs[dfs_count].timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count();
            dfs[dfs_count].temperature = std::stoi(std::string(read_buf + 12, 3));

            dfs_count++;
        }

        return dfs_count;
    }
};

int main(int argc, char *argv[])
{
    SimplePainlabProtocol* protocol = new SimplePainlabProtocol();
    TSAII_Serial_Connection* tsa_serial = new TSAII_Serial_Connection();

    protocol->Connect(argv[1]);
    tsa_serial->Connect();

    int in_post_stimulation_data_collection = 0;
    DataFrame dfs[1000];
    memset(dfs, 0, sizeof(DataFrame) * 1000);

    while (1)
    {
        int ret = protocol->GetServerData();

        while (ret)
        {
            std::cout << "try to reconnect..." << std::endl;
            protocol->Connect(argv[1]);
            ret = protocol->GetServerData();
        }

        if (protocol->_server_responses.front() == "OK" || protocol->_server_responses.front() == "FAIL")
        {
            // should check OK before sending next post stimulation data batches, but that's fine for now.
        }
        else
        {
            if (in_post_stimulation_data_collection)
            {
                // can't apply command during post stimulation data collection phase
                protocol->SendData("FAIL", 4);
            }
            else
            {
                // block further command before post stimulation data collection complete
                if (protocol->_server_responses.front() == "L") in_post_stimulation_data_collection = 1;

                int n = tsa_serial->WriteSerial(protocol->_server_responses.front());
                if (n == protocol->_server_responses.front().size())
                {
                    protocol->SendData("OK", 2);
                }
                else
                {
                    protocol->SendData("FAIL", 4);
                }
                int collected_frames = 0;
                if (protocol->_server_responses.front() == "L")
                {
                    collected_frames = tsa_serial->PopulatingPostStimulationTemperatureData(dfs);
                    protocol->SendData(reinterpret_cast<char*>(dfs), sizeof(DataFrame) * collected_frames);
                    // unblock new commands from the host
                    in_post_stimulation_data_collection = 0;
                }
            }
        }
        protocol->_server_responses.pop();
    }

    return 0;
}
