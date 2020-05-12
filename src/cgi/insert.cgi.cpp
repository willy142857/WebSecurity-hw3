#include <iostream>
#include <fstream>
#include <filesystem>

#include <fmt/color.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include "include/base64.h"

using namespace std;
namespace fs = std::filesystem;

int main(int argc, char *argv[])
{
    string rawData;
    char buf[4096];
    while (cin) {
        cin.read(buf, sizeof(buf));
        rawData.append(buf, cin.gcount());
    }
    
    if (!Base64::Decode(rawData, &rawData)) {
        fmt::print(stderr, fmt::fg(fmt::color::red) | fmt::emphasis::bold,
                   "[insert.cgi] Base64 decode error\n");
        return 1;
    }
        
    if (auto expected = stoul(getenv("CONTENT_LENGTH")), actual = rawData.size() * sizeof(char);
        expected != actual) {
        fmt::print(stderr, fmt::fg(fmt::color::red) | fmt::emphasis::bold,
                   "[insert.cgi] Does not match content length. expected: {}, actual: {}\n", expected, actual);
        return 1;
    }

    const string filename = [&rawData]() {
        const string delimiter = "filename=\"";
        auto start = rawData.find(delimiter) + delimiter.length();
        string ret;
        while (rawData[start] != '\"')
            ret += rawData[start++];
        return ret;
    }();

    auto start = rawData.find("\r\n\r\n") + 4;
    auto end = rawData.rfind("------");
    auto fileContent = rawData.substr(start, end - start);
    
    fs::current_path("../../");
    ofstream fout("./upload/" + filename, ios::binary);
    if (!fout) {
        fmt::print(stderr, fmt::fg(fmt::color::red) | fmt::emphasis::bold,
            "[insert.cgi] fail to write file\n");
        cout << "Content-Type: text/html" << "\r\n";
        cout << "<h2> 500 Internal Server Error</h2>";
        return 1;
    }

    fout << fileContent;
    fout.close();
    fmt::print(stderr, fmt::fg(fmt::color::lime_green), 
        "[insert.cgi] {} was written successfully\n", filename);

    cout << "Content-Type: text/html" << "\r\n";
    cout << "Content-Length: " << fs::file_size("./static/done.html") << "\r\n\r\n";
    cout << fstream{"./static/done.html"}.rdbuf();
}