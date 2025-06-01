#include "pak.h"
#include <boost/filesystem.hpp>
#include <algorithm>
#include <cstring>
#include <functional>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace {
	void usage(char const* progname) {
		std::cerr << "Usage: " << progname << " file.pak --extract [files] [--output directory]" << std::endl;
		std::cerr << "       " << progname << " file.pak --dump file" << std::endl;
		std::cerr << "       " << progname << " file.pak --list" << std::endl;
	}

	struct Extract {
		Extract(Pak& p): m_p(p) {}
		Extract(Pak& p, std::string const& output): m_p(p), output(output) {}
		void operator()(std::string const& filename) { operator()(make_pair(filename, m_p[filename])); }
		void operator()() {
			for (Pak::files_t::const_iterator it = m_p.files().begin(); it != m_p.files().end(); ++it) operator()(*it);
		}
		void operator()(std::pair<std::string, PakFile> const& fp) {
			std::string filename = fp.first;
			// Remove path elements from m_path until it matches the filename's beginning
			while (m_path != filename.substr(0, m_path.size())) {
				std::string::size_type pos = m_path.rfind('/');
				if (pos == std::string::npos) m_path.clear();
				else m_path.erase(pos);
			}

			// Add output as required
			if (!output.empty()) {
				if (output[output.size() -1 ] != '/') {
					output += '/';
				}

				filename.insert(0, output);
			}

			// Try to create new folders as required
			for (std::string::size_type pos; (pos = filename.find('/', m_path.size() + 1)) != std::string::npos;) {
				m_path = filename.substr(0, pos);
				boost::filesystem::create_directory(m_path);
			}
			// Extract the file
			std::ofstream f(filename.c_str(), std::ios::binary);
			if (!f.is_open()) throw std::runtime_error("Unable to create file: " + filename);
			std::cout << filename << std::flush;
			std::vector<char> buf;
			fp.second.get(buf);
			f.write(&buf[0], buf.size());
			std::cout << "  " << buf.size() << " bytes" << std::endl;
		}
	  private:
		Pak& m_p;
		std::string m_path;
		std::string output;
	};
}

int main(int argc, char** argv) {
	std::ios::sync_with_stdio(false);
	std::string output;
	unsigned int offset = 0;
	if( argc < 3 ) { usage(argv[0]); return EXIT_FAILURE; }
	try {
		Pak p(argv[1]);
		if (!strcmp(argv[2],"--list")) std::cout << p.files();
		else if (!strcmp(argv[2],"--dump")) {
			if (argc != 4) { usage(argv[0]); return EXIT_FAILURE; }
			std::vector<char> buf;
			p[argv[3]].get(buf);
			std::cout.write(&buf[0], buf.size());
		} else if (!strcmp(argv[2],"--extract")) {
			if (argc >= 5 && !strcmp(argv[argc - 2], "--output")) {
				std::cout << "Change output path: " << argv[argc - 1] << std::endl;
				output = argv[argc - 1];
				offset = 2;
			}

			if (argc == 3 || (argc == 5 && offset)) {
				 (Extract(p, output))();
			} else {
				std::for_each(argv + 3, argv + argc - offset, Extract(p, output));
			}
		} else { usage(argv[0]); return EXIT_FAILURE; }
	} catch (std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}

