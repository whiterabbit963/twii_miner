#include "skill_input.h"

#define TOML_IMPLEMENTATION
#include <toml++/toml.hpp>
#include <fmt/format.h>

#include "skill_output.h"

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

std::optional<MapList> loadMapInput(toml::array *arr)
{
    MapList mapList;
    if(!arr)
        return std::nullopt;
    for(auto &mapLoc : *arr)
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

std::optional<std::vector<uint32_t>> loadOverlaps(toml::array *arr)
{
    std::vector<uint32_t> overlaps;
    if(!arr)
        return std::nullopt;

    for(auto &overlapItem : *arr)
    {
        auto strId = overlapItem.as_string();
        if(!strId)
            return std::nullopt;

        overlaps.push_back(atoi(strId->get().c_str()));
    }
    return overlaps;
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
        if(top.second.is_array_of_tables())
        {
            auto arr = top.second.as_array();
            if(!arr)
                return false;

            for(auto &items : *arr)
            {
                Skill skill;
                skill.group = getSkillType(top.first.str());
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
                        auto mapList = loadMapInput(item.second.as_array());
                        if(!mapList)
                            return false;
                        skill.mapList = std::move(*mapList);
                    }
                    else if(item.first == "level")
                    {
                        auto value = item.second.value<double>();
                        if(!value.has_value())
                            return false;
                        skill.sortLevel = fmt::format("{}", value.value());
                    }
                    else if(item.first == "overlap")
                    {
                        auto overlaps = loadOverlaps(item.second.as_array());
                        if(!overlaps)
                            return false;
                        skill.overlapIds = std::move(*overlaps);
                    }
                    else if(item.first == "tag")
                    {
                        auto value = item.second.as_string();
                        if(!value)
                            return false;
                        skill.tag = value->get();
                    }
                    else if(item.first == "autoLevel")
                    {
                        auto value = item.second.as_boolean();
                        if(!value)
                            return false;
                        skill.autoLevel = value->get();
                    }
                    else if(item.first == "store")
                    {
                        auto value = item.second.as_boolean();
                        if(!value)
                            return false;
                        skill.storeLP = value->get();
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
        auto &mapSkill = skillItem->second;
        auto it = std::ranges::find(xmlSkills, mapSkill.id, &Skill::id);
        if(it != xmlSkills.end())
        {
            auto mapIt = std::find_if(std::next(skillItem), skillMap.end(),
                                      [id = mapSkill.id](auto &item)
                { return item.second.id == id; });
            if(mapIt == skillMap.end())
                it->status = Skill::SearchStatus::Found;
            else
                it->status = Skill::SearchStatus::MultiFound;
            if(it->group == Skill::Type::Unknown)
                it->group = mapSkill.group;
            it->mapList = mapSkill.mapList;
            it->overlapIds = mapSkill.overlapIds;
            // TODO: copy other skill input values
        }
        skills.push_back(std::move(*it));
    }

    // TODO: verify overlaps against rep skills
    return true;
}
