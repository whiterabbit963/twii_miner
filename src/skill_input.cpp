#include "skill_input.h"

#include <algorithm>
#include <cctype>

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

bool validLocaleLabel(std::string_view locale)
{
    if(locale == EN || locale == DE || locale == FR || locale == RU)
        return true;
    return false;
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

std::optional<LCLabel> loadAcquireInput(toml::table *tbl)
{
    LCLabel input;
    if(!tbl)
        return std::nullopt;
    for(auto &item : *tbl)
    {
        if(item.first != "EN" &&
                item.first != "DE" &&
                item.first != "FR" &&
                item.first != "RU")
            return std::nullopt;

        std::string lc;
        std::transform(item.first.begin(), item.first.end(), std::back_inserter(lc), ::tolower);

        auto descOpt = item.second.as_string();
        if(!descOpt)
            return std::nullopt;
        input[lc] = descOpt->get();
    }
    return input;
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

        overlaps.push_back(strtoul(strId->get().c_str(), nullptr, 16));
    }
    return overlaps;
}

bool loadSkillInput(toml::table *itemTable, Skill &skill)
{
    if(!itemTable)
    {
        return false;
    }
    for(auto &item : *itemTable)
    {
        std::string_view name = item.first.str();
        if(name == "id")
        {
            auto value = item.second.as_string();
            if(!value)
                return false;

            skill.id = strtol((*value)->c_str(), nullptr, 16);
        }
        else if(name == "map")
        {
            auto mapList = loadMapInput(item.second.as_array());
            if(!mapList)
                return false;
            skill.mapList = std::move(*mapList);
        }
        else if(name == "level")
        {
            auto value = item.second.value<double>();
            if(!value.has_value())
                return false;
            skill.sortLevel = fmt::format("{}", value.value());
        }
        else if(name == "overlap")
        {
            auto overlaps = loadOverlaps(item.second.as_array());
            if(!overlaps)
                return false;
            skill.overlapIds = std::move(*overlaps);
        }
        else if(name == "tag")
        {
            auto value = item.second.as_string();
            if(!value)
                return false;
            skill.tag = value->get();
        }
        else if(name == "minLevel")
        {
            auto value = item.second.value<int>();
            if(!value)
                return false;
            skill.minLevelInput = value.value();
        }
        else if(name == "store")
        {
            auto value = item.second.as_boolean();
            if(!value)
                return false;
            skill.storeLP = value->get();
        }
        else if(name == "autoRep")
        {
            auto value = item.second.as_boolean();
            if(!value)
                return false;
            skill.autoRep = value->get();
        }
        else if(name == "acquire_desc")
        {
            auto acquire = loadAcquireInput(item.second.as_table());
            if(!acquire)
                return false;
            skill.acquireDesc = std::move(*acquire);
        }
        else if(name == "EN" || name == "DE" || name == "FR" || name == "RU")
        {
            auto value = item.second.as_table();
            if(!value)
                return false;
            for(auto &label : *value)
            {
                auto &lblName = label.first;
                auto lblValue = label.second.value<std::string_view>();
                if(!lblValue.has_value())
                    return false;

                std::optional<LCLabel> *lclPtr = nullptr;
                if(lblName == "label")
                {
                    lclPtr = &skill.label;
                }
                else if(lblName == "zone")
                {
                    lclPtr = &skill.zone;
                }
                else if(lblName == "zlabel")
                {
                    lclPtr = &skill.zlabel;
                }
                else if(lblName == "detail")
                {
                    lclPtr = &skill.detail;
                }
                if(lclPtr)
                {
                    std::string lc;
                    std::transform(name.begin(), name.end(), std::back_inserter(lc), ::tolower);
                    if(!lclPtr->has_value())
                        (*lclPtr) = std::make_optional<LCLabel>();
                    auto &lcl = *lclPtr;
                    lcl->insert({lc, std::string{lblValue.value()}});
                }
            }
        }
    }
    return true;
}

bool mergeSkillInputs(TravelInfo &info)
{
    toml::parse_result result = toml::parse_file("C:\\projects\\twii_miner\\skill_input.toml");

    if(!result)
    {
        auto &err = result.error();
        fmt::println("Parsing failed @ line: {}: {}",
                     err.source().begin.line, err.description());
        return false;
    }

    std::map<unsigned, Skill> skillInputs;
    auto &table = result.table();
    for(auto &top : table)
    {
        if(top.second.is_array_of_tables())
        {
            auto arr = top.second.as_array();
            for(auto &items : *arr)
            {
                Skill skill;
                skill.group = getSkillType(top.first.str());
                auto *itemTable = items.as_table();
                if(!loadSkillInput(itemTable, skill))
                    return false;
                skillInputs.insert({itemTable->source().begin.line, std::move(skill)});
            }
            continue;
        }

        if(top.second.is_table() && top.first.str() == "labels"sv)
        {
            auto table = top.second.as_table();
            for(auto &group : *table)
            {
                Skill::Type type = getSkillType(group.first.str());
                if(type == Skill::Type::Unknown)
                    return false;
                auto labelTable = group.second.as_table();
                if(!labelTable)
                    return false;
                LCLabel tags;
                for(auto &label : *labelTable)
                {
                    std::string locale = std::string{label.first.str()};
                    std::transform(locale.begin(), locale.end(), locale.begin(), ::tolower);
                    if(!validLocaleLabel(locale))
                        return false;
                    auto tag = label.second.value<std::string>();
                    if(!tag)
                        return false;
                    tags.insert({std::string{locale}, *tag});
                }
                info.labelTags.insert({type, tags});
            }
        }
    }

    std::vector<Skill> xmlSkills = std::move(info.skills);
    info.skills = std::vector<Skill>{};
    info.skills.reserve(xmlSkills.size());
    for(auto item = skillInputs.begin(); item != skillInputs.end(); ++item)
    {
        auto &skillInput = item->second;
        const uint32_t skillId = skillInput.id;
        auto it = std::ranges::find(xmlSkills, skillId, &Skill::id);
        if(it != xmlSkills.end())
        {
            auto mapIt = std::find_if(std::next(item), skillInputs.end(),
                                      [skillId](auto &item)
                { return item.second.id == skillId; });
            if(mapIt == skillInputs.end())
                it->status = Skill::SearchStatus::Found;
            else
                it->status = Skill::SearchStatus::MultiFound;
            if(it->group == Skill::Type::Unknown)
                it->group = skillInput.group;
            it->storeLP = skillInput.storeLP;
            it->minLevelInput = skillInput.minLevelInput;
            it->mapList = skillInput.mapList;
            it->acquireDesc = skillInput.acquireDesc;
            it->overlapIds = skillInput.overlapIds;
            it->sortLevel = skillInput.sortLevel;
            it->label = skillInput.label;
            it->zone = skillInput.zone;
            it->zlabel = skillInput.zlabel;
            it->detail = skillInput.detail;
            // TODO: copy other skill input values
        }
        info.skills.push_back(std::move(*it));
    }

    // TODO: verify overlaps against rep skills
    return true;
}
