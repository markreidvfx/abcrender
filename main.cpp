#include "rendercontext.h"
#include "abcrender.h"
#include "vertex.h"

#include <iostream>
#include <string>
#include <chrono>
#include <stdlib.h>
#include <Alembic/Abc/All.h>

using namespace std;

void usage_message(const char argv0[])
{
    cerr << "usage: " << argv0 << " [options] file.abc [dest.%04d.ext]" << endl;
    cerr << "       -t --texture         texture to use on geometry." << endl;
    cerr << "       -i --imageplane      background image.%04d.jpg." << endl;
    cerr << "       -s --start           start frame." << endl;
    cerr << "       -e --end             end frame." << endl;
    cerr << "       -h --hel             display this usage information." << endl;
}

std::string basename(std::string const & path)
{
  return path.substr(path.find_last_of("/\\") + 1);
}


std::string dirname(std::string const & path)
{
  return path.substr(0, path.find_last_of("/\\"));
}

void splitext(std::string const & path, std::string &name, std::string &ext)
{
    size_t pos = path.find_last_of(".");
    if ( pos == string::npos) {
        name = path;
        ext = "";
        return;
    }
    name  = path.substr(0, pos);
    ext = path.substr(pos);
}

std::string guess_dest_path(std::string abc_path)
{
    string abc_dirname = dirname(abc_path);
    string abc_basename = basename(abc_path);
    string name;
    string ext;
    splitext(abc_basename, name, ext);

    std::string dest_dirname = abc_dirname + "/" + name;
    std::string dest = dest_dirname + "/" + name + ".%04d.png";

    //std::cerr << dest_dirname << endl;

    std::string cmd = "mkdir " + dest_dirname;
    system(cmd.c_str());

    return dest;
}

static bool parse_int(const std::string &str, int &result)
{
    // not set ignore
    if (str.empty())
        return true;

    char *temp;
    long val = strtol(str.c_str(), &temp, 0);

    if (temp == str || *temp != '\0' ||
            ((val == LONG_MIN || val == LONG_MAX) && errno == ERANGE))
         return false;
    result = val;
    return true;
}

int main(int argc, char* argv[])
{

    if (argc < 1) {
           usage_message(argv[0]);
           return 1;
    }

    std::vector<std::string> args;

    std::string texture_arg = "";
    std::string imageplane_arg = "";
    std::string start_arg = "";
    std::string end_arg = "";

    for (int i = 1; i < argc; ++i) {
        string a(argv[i]);

        if (a[0] == '-') {
            if ( (a == "-t" || a == "--texture") && i+1 < argc) {
                texture_arg = argv[i+1];
                i++;
            } else if ( (a == "-i" || a == "--imageplane") && i+1 < argc) {
                imageplane_arg = argv[i+1];
                i++;
            } else if ( (a == "-s" || a == "--start") && i+1 < argc) {
                start_arg = argv[i+1];
                i++;
            } else if ( (a == "-e" || a == "--end") && i+1 < argc) {
                end_arg = argv[i+1];
                i++;
            } else if (a == "-h" || a == "--help") {
                usage_message(argv[0]);
                return 0;
            } else {
                usage_message(argv[0]);
                std::cerr << "invalid option \""<< a << "\"" << std::endl;
                return -1;
            }
        } else {
            args.push_back(a);
        }
    }

    if (args.size() < 1) {
        usage_message(argv[0]);
        return 1;
    }

    std::string abc_path = args[0];
    std::string dest;
    int start_frame;
    int end_frame;

    {
    AbcF::IFactory factory;
    factory.setPolicy(Abc::ErrorHandler::kQuietNoopPolicy);
    AbcF::IFactory::CoreType coreType;
    IArchive archive = factory.getArchive(abc_path, coreType);

    double fps = 24.0;
    double f,l;
    Abc::GetArchiveStartAndEndTime(archive, f,l);

    start_frame = (int)(f*fps + 0.5);
    end_frame = (int)(l*fps + 0.5);
    }

    if (!parse_int(start_arg, start_frame)) {
        std::cerr << "error parsing start frame: \"" << start_arg << "\"" << std::endl;
        return -1;
    }

    if (!parse_int(end_arg, end_frame)) {
        std::cerr << "error parsing end frame: \"" << end_arg << "\"" << std::endl;
        return -1;
    }

    if (args.size() > 1)
        dest = args[1];
    else
        dest = guess_dest_path(abc_path);

    std::cerr << texture_arg << std::endl;

    return abcrender(abc_path, dest,
                     imageplane_arg,
                     texture_arg,
                     start_frame, end_frame);
}

