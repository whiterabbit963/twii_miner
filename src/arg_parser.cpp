#include "arg_parser.h"

#include <string>
#include <fmt/format.h>

static void printUsage()
{
    fmt::println("Usage: twii_miner [options]");
    fmt::println("");
    fmt::println("Options:");
    fmt::println("  -h, --help       Show this help message");
    fmt::println("  -path <path>     Root directory containing lotro-data/ and lotro-items-db/");
    fmt::println("                   (default: C:\\projects)");
    fmt::println("");
    fmt::println("");
    fmt::println("Example:");
    fmt::println("  twii_miner -path \"C:\\projects\"\n");
}

std::optional<ParsedArgs> parseArguments(int argc, const char **argv)
{
    ParsedArgs result;
    result.dataRoot = "C:\\projects";

    for(int i = 0; i < argc; ++i)
    {
        std::string_view arg{argv[i]};
        if(arg == "-h" || arg == "--help")
        {
            result.helpRequested = true;
        }
        else if(arg == "-path")
        {
            ++i;
            if(i >= argc)
            {
                printUsage();
                return std::nullopt;
            }
            result.dataRoot = argv[i];
        }
    }

    return result;
}
