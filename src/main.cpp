#include <locale>
#include <fmt/format.h>
#include "skill_loader.h"
#include "skill_input.h"
#include "skill_output.h"

using namespace std;

void printNewSkills(const std::vector<Skill> &skills)
{
#if 0 // TODO: REDO? probably obsolete
    // verification
    fmt::println("Skills: {}:{}", skills.size(), skillIds.size());
    for(auto &skill : skills)
    {
        if(!std::any_of(skillIds.begin(), skillIds.end(), [&skill](auto &id) { return skill.id == id; }))
        {
            fmt::println("{}", skill.name.at(EN));
        }
        if(skill.name.size() != 4)
            fmt::println("{}", skill.id);
    }
    for(auto &id : skillIds)
    {
        if(!std::any_of(skills.begin(), skills.end(), [&id](auto &s) { return s.id == id; }))
        {
            fmt::println("{}", id);
        }
    }
    fmt::println("\n\n");

    unsigned next = 0;
    for(auto &skill : skills)
    {
        if(!std::any_of(skillIds.begin(), skillIds.end(), [&skill](auto &id) { return skill.id == id; }))
        {
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
#endif
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
    if(!mergeSkillInputs(info))
    {
        return -1;
    }
    if(!loader.getCurrencies(info))
    {
        return -1;
    }
    info.factions = loader.getFactions();

    printNewSkills(info.skills);
    outputSkillDataFile(info);
    outputLocaleDataFile(info);
    return 0;
}
