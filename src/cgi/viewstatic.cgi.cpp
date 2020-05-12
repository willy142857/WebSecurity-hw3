#include <iostream>
#include <fstream>
#include <filesystem>

#include "include/utility.h"

using namespace std;
namespace fs = std::filesystem;

int main(int argc, char *argv[])
{
    fs::current_path("../../");
    const auto filename = string{getenv("QUERY_STRING")}.substr(5);
    cout << "Content-Type: " << mineType(filename) << "\r\n";
    cout << "Content-Length: " << fs::file_size("./upload/" + filename) << "\r\n\r\n";
    
    fstream fin("./upload/" + filename);
    if(!fin)
        cout <<"Not Found";
    else
        cout << fin.rdbuf();
}