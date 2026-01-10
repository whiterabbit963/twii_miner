#include <locale>
#include <vector>
#include <filesystem>
#include <fmt/format.h>
#include <arg_parser.h>
#include "skill_loader.h"
#include "skill_input.h"
#include "skill_output.h"

using namespace std;

static bool validateDataDirectories(const std::string &rootPath)
{
    namespace fsys = std::filesystem;

    if(!fsys::exists(rootPath))
    {
        fmt::println("Error: Data root directory does not exist: {}", rootPath);
        return false;
    }

    if(!fsys::is_directory(rootPath))
    {
        fmt::println("Error: Data root path is not a directory: {}", rootPath);
        return false;
    }

    const std::vector<std::string> requiredDirs
        {
            "lotro-data",
            "lotro-data/lore",
            "lotro-items-db"
        };

    for(const auto &subdir : requiredDirs)
    {
        fsys::path fullPath = fsys::path(rootPath) / subdir;

        if(!fsys::exists(fullPath))
        {
            fmt::println("Error: Missing required directory: {}", fullPath.string());
            return false;
        }

        if(!fsys::is_directory(fullPath))
        {
            fmt::println("Error: Path is not a directory: {}", fullPath.string());
            return false;
        }
    }

    return true;
}

int main(int argc, const char **argv)
{
#if WIN32
    // force UTF8 as the default multi-byte locale and not ANSI
    locale::global(locale(locale{}, locale(".utf8"), locale::ctype));
#endif

    auto args = parseArguments(argc, argv);
    if(!args)
    {
        return 1;
    }

    if(args->helpRequested)
    {
        return 0;
    }

    if(!validateDataDirectories(args->dataRoot))
    {
        return 1;
    }

    TravelInfo info;
    SkillLoader loader(args->dataRoot);
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
