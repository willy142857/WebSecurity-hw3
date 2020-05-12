#include <chrono>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

#include <boost/format.hpp>
#include <fmt/ostream.h>
#include <fmt/format.h>

using namespace std;
namespace fs = std::filesystem;

template <typename TP>
std::time_t to_time_t(TP tp)
{
    using namespace std::chrono;
    auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now()
              + system_clock::now());
    return system_clock::to_time_t(sctp);
}

int main(int argc, char *argv[])
{
    fs::current_path("../../");
    
    cout << "Content-Type: text/html" << "\r\n\r\n";
    cout << fstream{"./static/part1"}.rdbuf();

    const string path = "./upload";
    fs::create_directory(path);
    int i = 1;
    for (const auto& entry : fs::directory_iterator(path)) {
        const auto filename = entry.path().filename().string();
        
        auto ftime = fs::last_write_time(entry.path());
        std::time_t cftime = to_time_t(ftime);
        std::tm *gmt = std::localtime(&cftime);
        char buf[80]{};
	    strftime(buf, 80, "%F %H:%M:%S", gmt);
        const string updatedTime {buf};

        cout << fmt::format(
            "<tr>\r\n \
                <th scope=\"row\">{}</th>\r\n \
                <td><a href=\"view.cgi/?file={}\">{}</a></td>\r\n \
                <td>{}</td>\r\n \
            </tr>\r\n", i++, filename, filename, updatedTime);   
    }
    cout << fstream{"./static/part2"}.rdbuf();
}

