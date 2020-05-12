#include <boost/asio.hpp>
#include <fmt/color.h>
#include <fmt/core.h>

#include "include/server.h"

int main(int argc, char* argv[])
{
	try {
		if (argc != 2) {
			fmt::print(fmt::fg(fmt::color::antique_white), "Usage: server <port>\n");
			return 1;
		}

		fmt::print(fmt::fg(fmt::color::light_cyan), "Server is listening on port:{}\n", argv[1]);
		boost::asio::io_context ioContext;
		WebServer server(ioContext, std::atoi(argv[1]));

		ioContext.run();
	}
	catch (const std::exception& e) {
		fmt::print(fmt::fg(fmt::color::red) | fmt::emphasis::bold, "Exception: {}\n", e.what());
	}
}
