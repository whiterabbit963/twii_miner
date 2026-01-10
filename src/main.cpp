#include <locale>
#include <filesystem>
#include <vector>
#include <fmt/format.h>
#include "skill_loader.h"
#include "skill_input.h"
#include "skill_output.h"

using namespace std;

struct ParsedArgs {
    std::string dataRoot;
    bool helpRequested{false};
    bool valid{true};
    std::string errorMessage;
};

ParsedArgs parseArguments(int argc, const char **argv) {
    ParsedArgs result;
    result.dataRoot = "C:\\projects";  // Default value

    // Check for help flags
    for(int i = 1; i < argc; ++i) {
        std::string_view arg{argv[i]};
        if(arg == "-h" || arg == "--help") {
            result.helpRequested = true;
            return result;
        }
    }

    // Parse positional argument
    if(argc > 1) {
        result.dataRoot = argv[1];
    }

    // Reject too many arguments
    if(argc > 2) {
        result.valid = false;
        result.errorMessage = "Too many arguments provided";
    }

    return result;
}

bool validateDataDirectories(const std::string &rootPath, std::string &errorMsg) {
    namespace fs = std::filesystem;

    // Check root exists
    if(!fs::exists(rootPath)) {
        errorMsg = fmt::format("Data root directory does not exist: {}", rootPath);
        return false;
    }

    if(!fs::is_directory(rootPath)) {
        errorMsg = fmt::format("Data root path is not a directory: {}", rootPath);
        return false;
    }

    // Check required subdirectories
    std::vector<std::string> requiredDirs{
        "lotro-data",
        "lotro-data/lore",
        "lotro-items-db"
    };

    for(const auto &subdir : requiredDirs) {
        fs::path fullPath = fs::path(rootPath) / subdir;

        if(!fs::exists(fullPath)) {
            errorMsg = fmt::format("Missing required directory: {}", fullPath.string());
            return false;
        }

        if(!fs::is_directory(fullPath)) {
            errorMsg = fmt::format("Path is not a directory: {}", fullPath.string());
            return false;
        }
    }

    return true;
}

void printUsage(const char *programName) {
    fmt::println("Usage: {} [data-root-path]",
                 std::filesystem::path(programName).filename().string());
    fmt::println("");
    fmt::println("Arguments:");
    fmt::println("  data-root-path   Root directory containing lotro-data/ and lotro-items-db/");
    fmt::println("                   (default: C:\\projects)");
    fmt::println("");
    fmt::println("Options:");
    fmt::println("  -h, --help       Show this help message");
    fmt::println("");
    fmt::println("Example:");
    fmt::println("  {} /path/to/lotro/data",
                 std::filesystem::path(programName).filename().string());
}

int main(int argc, const char **argv)
{
#if WIN32
    // force UTF8 as the default multi-byte locale and not ANSI
    locale::global(locale(locale{}, locale(".utf8"), locale::ctype));
#endif

    // Parse command-line arguments
    auto args = parseArguments(argc, argv);

    // Handle help request
    if(args.helpRequested) {
        printUsage(argv[0]);
        return 0;
    }

    // Handle parsing errors
    if(!args.valid) {
        fmt::println(stderr, "Error: {}", args.errorMessage);
        printUsage(argv[0]);
        return 1;
    }

    // Validate directory structure
    std::string validationError;
    if(!validateDataDirectories(args.dataRoot, validationError)) {
        fmt::println(stderr, "Error: {}", validationError);
        fmt::println(stderr, "");
        printUsage(argv[0]);
        return 1;
    }

    TravelInfo info;
    SkillLoader loader(args.dataRoot);
    info.skills = loader.getSkills();
    if(!loadSkillInputs(info))
    {
        return 1;
    }
    printNewSkills(info);
    if(!mergeSkillInputs(info, info.inputs))
    {
        return 1;
    }
    if(!loader.getCurrencies(info))
    {
        return 1;
    }
    if(!loader.getFactions(info))
    {
        return 1;
    }

    outputSkillDataFile(info);
    outputLocaleDataFile(info);
    return 0;
}
