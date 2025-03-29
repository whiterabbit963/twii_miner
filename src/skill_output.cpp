#include "skill_output.h"
#include "skill_loader.h"

#include <fstream>
#include <fmt/format.h>
#include <fmt/ostream.h>

using namespace std;

struct TravelOutputState
{
    //std::string_view groupName;
    //std::map<std::string, unsigned> sortLevelNext;
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
    case Skill::Type::Racial: return "racials";
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
    if(name == "racials")
        return Skill::Type::Racial;
    if(name == "creep")
        return Skill::Type::Creep;
    return Skill::Type::Unknown;
}

std::string_view getRegionText(MapLoc::Region region)
{
    switch(region)
    {
    case MapLoc::Region::Eriador:
        return "ERIADOR"sv;
    case MapLoc::Region::Rhovanion:
        return "RHOVANION"sv;
    case MapLoc::Region::Rohan:
        return "ROHAN"sv;
    case MapLoc::Region::Gondor:
        return "GONDOR"sv;
    case MapLoc::Region::Haradwaith:
        return "HARADWAITH"sv;
    case MapLoc::Region::Creep:
        return "CREEP"sv;
    default: return "NONE"sv;
    }
}

static string outputMapLoc(const MapLoc &loc)
{
    return fmt::format("{{MapType.{}, {}, {}}}", getRegionText(loc.region), loc.x, loc.y);
}

static string outputMapList(const vector<MapLoc> &locs)
{
    string output;
    for(auto it = locs.begin(); it != locs.end(); ++it)
    {
        if(std::next(it) != locs.end())
            output += fmt::format("{},", outputMapLoc(*it));
        else
            output += outputMapLoc(*it);
    }
    return fmt::format("{{{}}}", output);
}

static string outputLabelTag(const LCLabel &tag)
{
    return fmt::format("{{EN=\"{}\", DE=\"{}\", FR=\"{}\", RU=\"{}\" }}",
                       tag.at(EN), tag.at(DE), tag.at(FR), tag.at(RU));
}

void outputSkill(ostream &out, const TravelInfo &info, const Skill &skill, TravelOutputState &state)
{
    //auto [sortIt, res] = state.sortLevelNext.insert({skill.sortLevel, 0});
    fmt::println(out, "    self.{}:AddSkill({{", getGroupName(skill.group));
    fmt::println(out, "        id=\"0x{:08X}\",", skill.id);
    fmt::println(out, "        EN={{ name=\"{}\", }},", skill.name.at(EN));
    fmt::println(out, "        DE={{ name=\"{}\", }},", skill.name.at(DE));
    fmt::println(out, "        FR={{ name=\"{}\", }},", skill.name.at(FR));
    fmt::println(out, "        RU={{ name=\"{}\", }},", skill.name.at(RU));
    fmt::println(out, "        map={},", outputMapList(skill.mapList));
    if(skill.minLevel)
        fmt::println(out, "        minLevel={},", skill.minLevel);
    fmt::println(out, "        level={}", skill.sortLevel);
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
        Skill::Type::Racial, Skill::Type::Gen, Skill::Type::Rep, Skill::Type::Creep};
    for(auto group : groups)
    {
        auto groupName = getGroupName(group);
        auto tagIt = info.labelTags.find(group);
        fmt::println(out, "    -- add the {} locations", groupName);
        if(tagIt != info.labelTags.end())
            fmt::println(out, "    self.{}:AddLabelTag({})",
                         groupName, outputLabelTag(tagIt->second));
        for(auto &skill : info.skills)
        {
            if(skill.group != group)
                continue;

            outputSkill(out, info, skill, state);
        }
    }
}
