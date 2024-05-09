#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <iostream>
#include <fstream>
#include "image.hpp"
#include "stb_image.h"
#include "stb_image_write.h"
#include <regex>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

image::image(int const& argc, char** const& argv) {

    /**
     * booleans *flag*_used are needed to put flags used by the user in certain order and
     * prevent iterating through "argv_" multiple times.
     * Flags can be written in the command line in different ways
     * (-d -i -c -e file mess, -e file mess -c -i -d)
     * but the program has one most efficient sequence of actions.
     * Therefore, it's better we know exactly what flags were used before executing them.
     *
     * And usual char*[] doesn't throw our_of_range exception, so it can be redefined as vector
     */

    if(argc == 1) {
        help();
        return;
    }

    auto argv_ = std::vector<char*>();
    for(int i = 1; i < argc; i++) {
        argv_.push_back(argv[i]);
    }

        try {
            for (int i = 0; i < argc-1; i++) {
                if (!std::strcmp(argv_[i], "-h") || !std::strcmp(argv_[i], "--help")) {
                    help_used = true;
                    continue;
                } else
                if (!std::strcmp(argv_[i], "-i") || !std::strcmp(argv_[i], "--info")) {
                    info_used = true;
                    filename = filename == "" ? argv_.at(++i) : filename;
                    continue;
                } else
                if (!std::strcmp(argv_[i], "-c") || !std::strcmp(argv_[i], "--check")) {
                    check_used = true;
                    filename = filename == "" ? argv_.at(++i) : filename;
                    message = message == "" ? argv_.at(++i) : message;
                    continue;
                } else
                if (!std::strcmp(argv_[i], "-d") || !std::strcmp(argv_[i], "--decrypt")) {
                    decrypt_used = true;
                    filename = filename == "" ? argv_.at(++i) : filename;
                    continue;
                } else
                if (!std::strcmp(argv_[i], "-e") || !std::strcmp(argv_[i], "--encrypt")) {
                    encrypt_used = true;
                    filename = filename == "" ? argv_.at(++i) : filename;
                    message = message == "" ? argv_.at(++i) : message;
                    continue;
                } else {
                    std::cout << "Flag syntax is incorrect. Use -h for more details.\n";
                    return;
                }

            }
        } catch(std::out_of_range exception) {
            std::cout << "Flag syntax is incorrect. Use -h for more details.\n";
            return;
        }

        flags_used_();
}
auto inline image::flags_used_() -> void {
    if (help_used) help();
    if (!(info_used || check_used || decrypt_used || encrypt_used)) return;

    if (!info()) return;
    if (info_used) {
        std::cout << "Filename: " << filename
                  << "\nDimentions: " << width << "x" << height
                  << "\nWidth: " << width
                  << "\nHeight: " << height
                  << "\nSize on disk: " << (float)size/1000 << "Mb"
                  << "\nModified: " << modified
                  << "\n\n";
    }

    if (!check()) return;
    if (check_used) {
        std::cout << "Current message can be encrypted into the image.\n\n";
    }

    if (decrypt_used) decrypt();
    if(encrypt_used) encrypt();
}
auto inline image::help() -> void {
    std::cout << "Flags: "
              << "\n-h, --help (User information)"
              << "\n-i, --info [filepath] (Image properties)"
              << "\n-c, --check [filepath] [message] (Checks if the message can be encrypted into the image)"
              << "\n-d, --decrypt [filepath] (Decrypts a message)"
              << "\n-e, --encrypt [filepath] [message] (Encrypts a message)"
              << "\nThe order of flags is insignificant. If multiple flags require the same argument, it must be placed after the first appropriate flag and, then, ignored."
              << "\nThe message, if has any spaces, must be written in quotes."
              << "\nApplication works with PNG, JPG and PPM (ASCII) image formats."
              << "\nUser can choose whether or not he wants the message to be encrypted into the copy of the image."
                 " Except for the JPG, which is always converted to PNG copy file. By default, application uses a copy.\n\n";
}
auto image::info() -> bool {

       struct stat fileInfo;
        if(stat(filename.c_str(), &fileInfo) != 0) {
            std::cout << "Reading of a file was not successful.";
            std::cerr << " Error: " << strerror(errno) << '\n';
            return 0;
        } else {
            size = fileInfo.st_size;
            time_t mod = fileInfo.st_mtime;
            struct tm date;
            localtime_s(&date, &mod);
            char timebuf[80];
            strftime(timebuf, sizeof(timebuf), "%c", &date);
            modified = timebuf;
        }

    auto type_ = std::regex_replace(filename, std::regex("^.+\\."), "");

    if(type_ == "ppm") {
        type = filetype::PPM;
    } else if(type_ == "jpg") {
        type = filetype::JPG;
    } else if (type_ == "png") {
        type = filetype::PNG;
    }

    /**
     * PPM:
     * Program takes first four values of a PPM file, which are P3, width, height and color range
     * respectively (skipping a line if the value starts with a #, which is a commentary.
     * If any of those values proves to be inappropriate by the standard of ASCII PPM,
     * program ends with 0.
     * PNG/JPG:
     * PNG and JPG are implemented with stb_image library. stbi_info function initializes width, height
     * and channels with found values and returns 0 if the file is incorrectly encoded.
     * =========
     * If there is decrypting or encrypting to do, program loads the data in form of
     * unsigned char* . Unsigned char can store values from 0 to 255 which is exactly the range
     * for every color value. In case of PPM, ifstream is used. PNG and JPG are loaded by
     * stbi_load function, which initializes width, height, channels and also takes forced number of
     * channels the user wants to leave in the unsigned char array. By default, there are three of them.
     */

    switch (type) {
        case filetype::PPM: {

            std::ifstream read;
            read.clear();
            read.open(filename);
            sleep(1);
            if(!read.is_open()) {
                std::cout << "I/O exception has occurred. Please, try again.\n";
                return 0;
            }

            auto buff = std::string();
            read >> buff;
            if(!std::regex_match(buff, std::regex("^P3$"))) {
                std::cout << filename << " p3: " << buff << '\n';
                std::cout << "Your image is invalid or damaged. Use -h for more details.\n";
                return 0;
            }

            auto buff_is_commentary = [&] () {
                return std::regex_match(buff, std::regex("^#.*"));
            };

            auto three_numbers = 0; //resolution and max color value
            while(three_numbers < 3) {
                read >> buff;
                if (buff_is_commentary()) {
                    std::getline(read, buff);
                } else {
                    if(!std::regex_match(buff, std::regex("[0-9]+"))) {
                        std::cout << "Your image is invalid or damaged. Use -h for more details.\n";
                        return 0;
                    } else {
                        three_numbers++;
                        switch(three_numbers) {
                            case 1: {
                                width = std::stoi(buff); break;
                            }
                            case 2: {
                                height = std::stoi(buff); break;
                            }
                            case 3: {
                                channels = 3; break;
                            }
                        }
                    }
                }
            }


            if (encrypt_used || decrypt_used) {

                bytes = width*height*channels;
                data = new unsigned char[bytes];
                for (int i = 0; i < bytes; i++) {
                    read >> buff;
                    data[i] = (unsigned char) std::stoi(buff);
                }

            }

            read.close();
            return 1;
        }
        case filetype::PNG:
        case filetype::JPG: {

            if (stbi_info(filename.c_str(), &width, &height, &channels) == 0) {
                std::cout << "Your image is invalid or damaged. Use -h for more details.\n";
                return 0;
            } else {
                if(encrypt_used || decrypt_used) {
                    data = stbi_load(filename.c_str(), &width, &height, &channels, 0);
                }
                return 1;
            }

        }
        default: {
            return 0;
        }
    }
}
auto inline image::check() -> bool {

        message = "starrt!" + message + "~end";
        if (!(message.size() * 8 <= width * height)) {
            std::cout << "The message is too long for the current image.\n";
            return 0;
        } else return 1;

}
auto image::decrypt() -> void {

    /**
     * Starting from the blue value at index 2, program goes through every blue value, collecting
     * the least significant bits and storing them in a "character" char. After filling the character
     * (8 times loop), it is assigned to the "output" string. If, at the output length 7 or more,
     * there is no "starrt!", then, the image does not contain a message and program ends.
     * Otherwise, characters are stored until the "~end".
     */

    auto output = std::string();
    auto end = std::regex("~end");
    auto start = std::regex(".*starrt!");

    for(int i = 2; i < width*height*channels; ) {
        auto character = char(0);
        for(int r = 7; r >= 0; r--) {
            int bit = data[i] & 1;
            character = character | bit << r;
            i+=3;
        }
        output += character;
        if (std::regex_search(output, end)) break;
        if (output.size() >= 7 && !std::regex_search(output, start)) {
            output = "";
            break;}
    }

    output = std::regex_replace(output, end, "");
    output = std::regex_replace(output, start, "");
    std::cout << "Message: " << output << "\n\n";

}
auto image::encrypt() -> void {

    /**
     * Program splits characters of the message by bits and stores them in the least significant bits
     * of blue values, going through every pixel from the first one to the end of the message
     * (message.size()*8).
     * PNG and JPG are encoded back into the image by stbi_write_png function from stbi_image_write
     * library.
     * PPM is encoded via ifstream, space between every color value and a new line after each three pixels.
     */

    int data_index = 2;

    for(int i = 0; i < message.length(); i++) {
        auto character = message[i];
        for(int r = 7; r >= 0; r--) {
            int bit = character>>r & 0b00000001;
            if(bit == 0) {
                bit = 0b11111110;
                data[data_index] = (data[data_index] & bit);
            } if(bit == 1) {
                data[data_index] = (data[data_index] | bit);
            }
            data_index+=3;
        }
    }

    switch (type) {
        case filetype::PPM: {
                std::ofstream write;
                std::cout << "Encrypt the message into a copy? y/n\n";
                auto yn = char();
                std::cin >> yn;
                if (yn == 'N' || yn == 'n') {
                    write.open(filename);
                } else {
                    auto filename_copy = filename.substr(0, filename.size() - 4) + "_copy.ppm";
                    write.open(filename_copy);
                }

                sleep(1);
                if (!write.is_open()) {
                    std::cout << "IO exception has occurred. Please, try again.\n";
                    return;
                }

                write << "P3 " + std::to_string(width) + " " + std::to_string(height) + " 255\n";

                int enter = 0;
                for (int i = 0; i < bytes; i++) {
                    write << (int) data[i]; //from the second we take their values (unsigned char can store from 0-255)
                    enter++;
                    if (enter % 9 == 0) {
                        write << "\n";
                    } else write << " ";

                }
                write.close();
            break;
        }
        case filetype::JPG:
        case filetype::PNG: {
            auto yn = char(0);
            if (type == filetype::PNG) {
                std::cout << "Encrypt the message into a copy? y/n\n";
                std::cin >> yn;
            }
            int non_failure = 1;
            if (yn == 'N' || yn == 'n') {
                non_failure = stbi_write_png(filename.c_str(), width, height, channels, data, width * channels);
            } else {
                auto filename_copy = filename.substr(0, filename.size() - 4) + "_copy.png";
                non_failure = stbi_write_png(filename_copy.c_str(), width, height, channels, data, width * channels);
            }
            if(non_failure == 0) {
                std::cout << "I/O exception has occurred. Please, try again.\n";
                return;}
            }
        }
            std::cout << "The message has been encrypted.\n\n";
}








