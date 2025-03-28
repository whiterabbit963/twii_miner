#include "skill_input.h"

#define TOML_IMPLEMENTATION
#include <toml++/toml.hpp>
#include <fmt/format.h>

MapLoc::Region getMapLocRegion(std::string_view name)
{
    if(name == "ERIADOR"sv)
        return MapLoc::Region::Eriador;
    if(name == "RHOVANION"sv)
        return MapLoc::Region::Rhovanion;
    if(name == "ROHAN"sv)
        return MapLoc::Region::Rohan;
    if(name == "GONDOR"sv)
        return MapLoc::Region::Gondor;
    if(name == "HARADWAITH"sv)
        return MapLoc::Region::Haradwaith;
    if(name == "CREEP"sv)
        return MapLoc::Region::Creep;
    if(name == "NONE"sv)
        return MapLoc::Region::None;
    return MapLoc::Region::Invalid;
}

std::optional<MapList> loadMapInput(toml::array *mapArr)
{
    MapList mapList;
    if(!mapArr)
        return std::nullopt;
    for(auto &mapLoc : *mapArr)
    {
        auto *loc = mapLoc.as_table();
        if(!loc)
            return std::nullopt;
        unsigned require = 0;
        MapLoc item;
        for(auto &mapItem : *loc)
        {
            if(mapItem.first == "type")
            {
                auto name = mapItem.second.as_string();
                if(name)
                {
                    item.region = getMapLocRegion(name->get());
                    if(item.region != MapLoc::Region::Invalid)
                        require |= 1;
                }
            }
            else if(mapItem.first == "x")
            {
                auto xval = mapItem.second.as_integer();
                if(xval)
                {
                    item.x = xval->get();
                    require |= 2;
                }
            }
            else if(mapItem.first == "y")
            {
                auto yval = mapItem.second.as_integer();
                if(yval)
                {
                    item.y = yval->get();
                    require |= 4;
                }
            }
            else
            {
                return std::nullopt;
            }
        }
        if((require & 0x7) != 0x7)
        {
            return std::nullopt;
        }

        mapList.push_back(item);
    }
    return mapList;
}

bool mergeSkillInputs(std::vector<Skill> &skills)
{
    toml::parse_result result = toml::parse_file("C:\\projects\\twii_miner\\skill_input.toml");

    if(!result)
    {
        auto &err = result.error();
        fmt::println("Parsing failed @ line: {}: {}",
                     err.source().begin.line, err.description());
        return false;
    }

    using SkillMap = std::map<unsigned, Skill>;
    SkillMap skillMap;
    auto &table = result.table();
    for(auto &top : table)
    {
        Skill skill;
        if(top.second.is_array_of_tables())
        {
            auto arr = top.second.as_array();
            if(!arr)
                return false;
            for(auto &items : *arr)
            {
                auto *itemTable = items.as_table();
                if(!itemTable)
                    continue;

                for(auto &item : *itemTable)
                {
                    if(item.first == "id")
                    {
                        auto value = item.second.as_string();
                        if(!value)
                            return false;

                        skill.id = strtol((*value)->c_str(), nullptr, 16);
                    }
                    else if(item.first == "map")
                    {
                        if(!loadMapInput(item.second.as_array()))
                            return false;
                    }
                    else if(item.first == "level")
                    {

                    }
                    else if(item.first == "overlap")
                    {

                    }
                }
                skillMap.insert({itemTable->source().begin.line, std::move(skill)});
            }
        }
        else
        {
            continue;
        }

    }

    std::vector<Skill> xmlSkills = std::move(skills);
    for(auto skillItem = skillMap.begin(); skillItem != skillMap.end(); ++skillItem)
    {
        auto &skill = skillItem->second;
        auto it = std::ranges::find(xmlSkills, skill.id, &Skill::id);
        if(it != xmlSkills.end())
        {
            auto mapIt = std::find_if(std::next(skillItem), skillMap.end(),
                                      [id = skill.id](auto &item)
                { return item.second.id == id; });
            if(mapIt == skillMap.end())
                skill.status = Skill::SearchStatus::Found;
            else
                skill.status = Skill::SearchStatus::MultiFound;
            skill.mapList = it->mapList;
            // TODO: copy other skill input values
        }
        skills.push_back(std::move(skill));
    }
    return true;
}
