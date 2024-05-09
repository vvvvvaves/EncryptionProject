#include <iostream>

struct image {

private:
    enum class filetype {
        JPG, PNG, PPM
    };

    int width, height, channels, bytes, size;
    std::string modified, filename, message;
    unsigned char* data;
    filetype type;

    bool help_used = false;
    bool info_used = false;
    bool check_used = false;
    bool decrypt_used = false;
    bool encrypt_used = false;

    auto flags_used_() -> void;
    auto help() -> void;
    auto info() -> bool;
    auto check() -> bool;
    auto decrypt() -> void;
    auto encrypt() -> void;

public:
    image(int const& argc, char** const& argv);
};
