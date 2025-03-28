#include "skill_output.h"
#include "skill_loader.h"

#include <fstream>
#include <fmt/format.h>
#include <fmt/ostream.h>

using namespace std;

struct TravelOutputState
{
    std::string_view groupName;
    unsigned nextLevel{0};
};

string_view getGroupName(Skill::Type type)
{
    switch(type)
    {
    case Skill::Type::Hunter: return "hunter";
    case Skill::Type::Warden: return "warden";
    case Skill::Type::Mariner: return "mariner";
    case Skill::Type::Gen: return "gen";
    case Skill::Type::Rep: return "rep";
    case Skill::Type::Creep: return "creep";
    case Skill::Type::Racial: return "racial";
    default: return "UNKNOWN";
    }
}

Skill::Type getSkillType(string_view name)
{
    if(name == "hunter")
        return Skill::Type::Hunter;
    if(name == "warden")
        return Skill::Type::Warden;
    if(name == "mariner")
        return Skill::Type::Mariner;
    if(name == "gen")
        return Skill::Type::Gen;
    if(name == "rep")
        return Skill::Type::Rep;
    if(name == "racial")
        return Skill::Type::Racial;
    if(name == "creep")
        return Skill::Type::Creep;
    return Skill::Type::Unknown;
}

void outputSkill(ostream &out, const TravelInfo &info, const Skill &skill, TravelOutputState &state)
{
    fmt::println(out, "    self.{}:AddSkill({{", state.groupName);
    fmt::println(out, "        id=\"0x{:08X}\",", skill.id);
    fmt::println(out, "        EN={{ name=\"{}\", }},", skill.name.at(EN));
    fmt::println(out, "        DE={{ name=\"{}\", }},", skill.name.at(DE));
    fmt::println(out, "        FR={{ name=\"{}\", }},", skill.name.at(FR));
    fmt::println(out, "        RU={{ name=\"{}\", }},", skill.name.at(RU));
    fmt::println(out, "        map={{{{MapType.HARADWAITH, -1, -1}}}},");
    if(skill.minLevel)
        fmt::println(out, "        minLevel={},", skill.minLevel);
    fmt::println(out, "        level={}.{}", skill.sortLevel, ++state.nextLevel);
    fmt::println(out, "    }})");
}

void outputSkillDataFile(const TravelInfo &info)
{
    std::ofstream out("SkillData.lua", ios::out);
    if(!out.is_open())
    {
        fmt::println("Failed to create SkillData.lua");
        return;
    }

#if 1 // TODO: after initial commit replace these top lines
    fmt::println(out, "---[[ travel skills ]] --");
    fmt::println(out, "--[[ Add all the travel skills ]] --");
    fmt::println(out, "-- add the data to custom dictionaries to maintain the order");
#else
    fmt::println(out, "---[[ auto-generated travel skills ]] --");
#endif
    fmt::println(out, "function TravelDictionary:CreateDictionaries()");
    TravelOutputState state;
    auto groups = {Skill::Type::Hunter, Skill::Type::Warden, Skill::Type::Mariner,
        Skill::Type::Gen, Skill::Type::Rep, Skill::Type::Creep};
    for(auto group : groups)
    {
        state.groupName = getGroupName(group);
        fmt::println(out, "    -- add the hunter locations");
        fmt::println(out, "    self.{}:AddLabelTag({{EN=\"Guide\", DE=\"Führer\", FR=\"Guide\", RU=\"Путь\" }})", state.groupName);
        for(auto &skill : info.skills)
        {
            if(skill.group == group)
            {
                outputSkill(out, info, skill, state);
            }
        }
    }
}
