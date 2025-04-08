#include <locale>
#include <fmt/format.h>
#include "skill_loader.h"
#include "skill_input.h"
#include "skill_output.h"

using namespace std;

void printNewSkills(const TravelInfo &info)
{
    // verification
    std::vector<const Skill*> newSkills;
    for(auto &skill : info.skills)
    {
        auto it = info.inputs.find(skill.id);
        if(it == info.inputs.end())
        {
            auto bit = ranges::find(info.blacklist, skill.id);
            if(bit == info.blacklist.end())
                newSkills.push_back(&skill);
        }
    }
    fmt::println("Skills: {}:{}", info.skills.size(), info.inputs.size());
    for(auto &skill : info.skills)
    {
        if(!std::any_of(info.inputs.begin(), info.inputs.end(), [&skill](auto &item)
                         { return skill.id == item.second.id; }))
        {
            fmt::println("{}", skill.name.at(EN));
        }
        if(skill.name.size() != 4)
            fmt::println("{}", skill.id);
    }
    for(auto &item : info.inputs)
    {
        if(!std::any_of(info.skills.begin(), info.skills.end(), [&item](auto &s)
                         { return s.id == item.second.id; }))
        {
            fmt::println("{}", item.first);
        }
    }
    fmt::println("\n\n");

    unsigned next = 0;
    for(auto &skill : info.skills)
    {
        if(!std::any_of(info.inputs.begin(), info.inputs.end(), [&skill](auto &item)
                         { return skill.id == item.second.id; }))
        {
            // TODO: convert to toml and append to toml input file
            fmt::println("id=\"0x{:08X}\",", skill.id);
            fmt::println("EN={{ name=\"{}\", }},", skill.name.at(EN));
            fmt::println("DE={{ name=\"{}\", }},", skill.name.at(DE));
            fmt::println("FR={{ name=\"{}\", }},", skill.name.at(FR));
            fmt::println("RU={{ name=\"{}\", }},", skill.name.at(RU));
            fmt::println("map={{{{MapType.HARADWAITH, -1, -1}}}},");
            fmt::println("minLevel=150,");
            fmt::println("level=150.{}", ++next);
            fmt::println("");
        }
    }
}

int main(int argc, const char **argv)
{
#if WIN32
    // force UTF8 as the default multi-byte locale and not ANSI
    locale::global(locale(locale{}, locale(".utf8"), locale::ctype));
#endif

    TravelInfo info;
    SkillLoader loader("C:\\projects"); // TODO: make an input arg
    info.skills = loader.getSkills();
    if(!loadSkillInputs(info))
    {
        return -1;
    }
    printNewSkills(info);
    if(!mergeSkillInputs(info, info.inputs))
    {
        return -1;
    }
    if(!loader.getCurrencies(info))
    {
        return -1;
    }
    if(!loader.getFactions(info))
    {
        return -1;
    }

    outputSkillDataFile(info);
    outputLocaleDataFile(info);
    return 0;
}
