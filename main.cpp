#include <iostream>
#include "buffer.h"


uint8_t data [1024];
Buffer buf(data, sizeof(data));

int main(int, char**) {

    char in_message[120] {0};
    char out_message[120] {0};

    sprintf(in_message, "Hello there");

    int write_size = buf.write((uint8_t *)in_message, 30);
    std::cout << "Write " << write_size << " bytes" << std::endl;


    int read_size = buf.read((uint8_t *)out_message, sizeof(out_message));
    std::cout << "Read " << write_size << " bytes" << std::endl;
    std::cout << "Read msg: " << out_message  << std::endl;
}
