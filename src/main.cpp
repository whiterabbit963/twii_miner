#include <locale>
#include <vector>
#include <filesystem>
#include <fmt/format.h>
#include <arg_parser.h>
#include "skill_loader.h"
#include "skill_input.h"
#include "skill_output.h"

#if defined(_WIN32)
#include <ShlObj_core.h>
#include <Windows.h>
#endif

using namespace std;

static std::string getHomeDir()
{
    std::string homeDirPath;
#if defined(_WIN32)
    PWSTR homeDir = nullptr;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_Profile, KF_FLAG_DEFAULT, nullptr, &homeDir);
    if(SUCCEEDED(hr))
    {
        std::filesystem::path result{ homeDir };
        CoTaskMemFree(homeDir);
        homeDirPath = result.string();
    }
#else
    // TODO:
#endif
    return homeDirPath;
}

std::string getDefaultTwIIFolder()
{
    return fmt::format("{}/Documents/The Lord of the Rings Online/Plugins/TravelWindowII", getHomeDir());
}

static bool validateDataDirectories(std::string_view rootPath, std::string_view twiiRoot)
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
            "lotro-items-db",
            std::string{twiiRoot}
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

    if(args->twiiRoot.empty())
    {
        args->twiiRoot = getDefaultTwIIFolder();
        if(args->twiiRoot.empty())
            return 1;
    }
    if(!validateDataDirectories(args->dataRoot, args->twiiRoot))
    {
        return 1;
    }

    TravelInfo info;
    SkillLoader loader(args->dataRoot, args->twiiRoot);
    info.skills = loader.getSkills();
    if(!loadSkillInputs(loader, info))
    {
        return 1;
    }
    getNewSkills(info);
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

    generateNewSkillInputFile(info);
    outputSkillDataFile(info);
    outputLocaleDataFile(info);
    return 0;
}
