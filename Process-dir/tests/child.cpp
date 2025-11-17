#include <iostream>
#include <unistd.h>
#include <sys/stat.h>

int main() {
    struct stat st0, st1;

    fstat(0, &st0);
    fstat(1, &st1);

    std::cout << "stdin is pipe?  " << S_ISFIFO(st0.st_mode) << "\n";
    std::cout << "stdout is pipe? " << S_ISFIFO(st1.st_mode) << "\n";

    std::string line;
    if (std::getline(std::cin, line)) {
        std::cout << "[child got]: " << line << std::endl;
    }

    return 0;
}
