#include "../include/Process.h"
#include <iostream>

int main() {
    Process p("./child", {});
    p.start();

    p.writeStdin("hello from parent\n");
    p.closeStdin();

    p.wait();

    std::string out = p.readStdout();
    std::cout << "CHILD OUTPUT:\n" << out << "\n";

    std::string err = p.readStderr();
    std::cout << "CHILD STDERR:\n" << err << "\n";

    return 0;
}
