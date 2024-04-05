#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include <chrono>

// OS headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

using namespace std;
using namespace std::chrono;

const string silent = "F";
const string get_temp = "E";

int main() {
    int serial_port = open("/dev/ttyUSB0", O_RDWR);
    int count = 0;

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

    char read_buf [1024];
    auto start = system_clock::now();
    auto last = start;
    long us [1000];
    string temp_data [1000];
    write(serial_port, "L", 1);
    while (count < 100) {
        write(serial_port, get_temp.c_str(), get_temp.size());
        int num_bytes = read(serial_port, &read_buf, sizeof(read_buf));
        cout << read_buf << "|" << num_bytes << endl;
        auto now = system_clock::now();
        us[count] = duration_cast<microseconds>(now - last).count();
        count++;
        last = now;
    }

    count = 0;
    while (count < 100) {
        cout << us[count] << endl;
        count++;
    }
    cout << duration_cast<microseconds>(last - start).count() << endl;
}
