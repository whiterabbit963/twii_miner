#include "skill_output.h"
#include "skill_loader.h"

#include <fstream>
#include <regex>
#include <set>
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

void replaceUtf8(string &in, const Utf8Map &strip)
{
    static std::set<std::string> s_utf8chars;
    auto s = in.end();
    for(auto it = in.begin(); it != in.end(); ++it)
    {
        if(*it & 0x80)
        {
            if(s == in.end())
            {
                s = it;
            }
        }
        else if(s != in.end())
        {
            string_view value{};
            auto stripIt = strip.find(string_view{s, it});
            if(stripIt == strip.end())
            {
                auto [_, success] = s_utf8chars.insert(string{s, it});
                if(success)
                {
                    fmt::println("REPLACE UTF8 {}", string_view{s, it});
                }
            }
            else
            {
                value = stripIt->second;
            }
            in.replace(s, it, value);
            it = s;
            s = in.end();
        }
    }
}

static string convertToLuaGVarName(const string &in, const Utf8Map &strip)
{
    string out = in;
    replaceUtf8(out, strip);
    std::replace(out.begin(), out.end(), ' ', '_');
    std::replace(out.begin(), out.end(), '-', '_');
    std::transform(out.begin(), out.end(), out.begin(), ::toupper);
    std::erase_if(out, [](int c) { return c != '_' && !::isalnum(c); });
    out = std::regex_replace(out, std::regex("THE_"), "");
    out = std::regex_replace(out, std::regex("___"), "_");
    return out;
}

static string extractNameAttr(const string &in)
{
    // ${PLAYERNAME:Verwandter[m]|Verwandte[f]}
    const std::regex attr("^\\$\\{PLAYERNAME:(.*)\\[.*\\|(.*)\\[.*\\}$");
    std::smatch matches;

    if(std::regex_match(in, matches, attr))
    {
        if(matches.size() >= 3)
        {
            return matches[1].str();
        }
    }
    return in;
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

static string outputOverlapIds(const vector<uint32_t> &ids)
{
    string out;
    string inner;
    for(auto it = ids.begin(); it != ids.end(); ++it)
    {
        if(std::next(it) == ids.end())
            fmt::format_to(std::back_inserter(inner), "\"0x{:08X}\"", *it);
        else
            fmt::format_to(std::back_inserter(inner), "\"0x{:08X}\", ", *it);
    }
    fmt::format_to(std::back_inserter(out), "{{{}}}", inner);
    return out;
}

static string outputVendor(const string &locale, const NPC &npc, uint32_t bartererId)
{
    if(!npc.titleKey.empty())
    {
        // TODO: many of these checks could likely be resolved
        //       by tracking if multiple vendors sell the same
        //       skill and have the same title
        string_view title = npc.title.at(EN);
        if(title == "Delving Mission Giver"sv ||
                title == "Ikorbâni Quartermaster"sv ||
                title == "Iron Garrison Miners"sv)
        {
            return npc.title.at(locale);
        }

        const std::regex attr("^.* Quartermaster$");
        std::smatch matches;
        if(std::regex_match(npc.name.at(EN), matches, attr))
        {
            return npc.name.at(locale);
        }

        return fmt::format("{} ({})", npc.name.at(locale), npc.title.at(locale));
    }
    return npc.name.at(locale);
}

static string outputVendors(const TravelInfo &info, uint32_t id)
{
    string buf;
    auto it = ranges::find(info.npcs, id, &NPC::id);
    if(it == info.npcs.end())
        return buf;
    auto in = std::back_inserter(buf);
    fmt::format_to(in, "                EN={{vendor=\"{}\" }},\n", outputVendor(EN, *it, id));
    fmt::format_to(in, "                DE={{vendor=\"{}\" }},\n", outputVendor(DE, *it, id));
    fmt::format_to(in, "                FR={{vendor=\"{}\" }},\n", outputVendor(FR, *it, id));
    fmt::format_to(in, "                RU={{vendor=\"{}\" }} }}", outputVendor(RU, *it, id));
    return buf;
}

static string outputQuest(const string &locale, const Acquire &acquire)
{
    string buf;
    auto in = std::back_inserter(buf);
    fmt::format_to(in, "quest=\"{}\"", acquire.questName.at(locale));
    return buf;
}

static void outputAcquire(ostream &out, const TravelInfo &info, const Skill &skill)
{
    if(skill.group == Skill::Type::Creep)
        return;
    if(skill.autoLevel)
    {
        fmt::println(out, "        acquire={{ {{ autoLevel=true }} }},");
    }
    else
    {
        string buf;
        auto in = std::back_inserter(buf);
        bool onlyCost = false;
        if(!skill.acquire.empty())
        {
            onlyCost = true;
            bool acquireFront = true;
            for(auto &acquire : skill.acquire)
            {
                if(!acquire.itemId)
                    continue;

                if(acquire.questId &&
                        !acquire.questNameKey.empty() &&
                        !acquire.questName.empty())
                {
                    fmt::format_to(in, "{}\n            {{", acquireFront ? "" : ",");
                    acquireFront = false;

                    fmt::format_to(in, "\n                EN={{{}}},", outputQuest(EN, acquire));
                    fmt::format_to(in, "\n                DE={{{}}},", outputQuest(DE, acquire));
                    fmt::format_to(in, "\n                FR={{{}}},", outputQuest(FR, acquire));
                    fmt::format_to(in, "\n                RU={{{}}}", outputQuest(RU, acquire));
                    fmt::format_to(in, " }}");
                }

                for(auto &bartersList : acquire.barters)
                {
                    fmt::format_to(in, "{}\n            {{cost={{", acquireFront ? "" : ",");
                    acquireFront = false;

                    bool tokenFront = true;
                    if(bartersList.currency.empty())
                    {
                        unsigned amt = bartersList.buyAmt * bartersList.sellFactor;
                        unsigned copper = amt % 100;
                        unsigned silver = (amt / 100) % 1000;
                        unsigned gold = amt / 100000;
                        if(gold)
                        {
                            fmt::format_to(in, "{}{{amount={}, token=LC.token.GOLD}}",
                                           tokenFront ? "" : ", ", gold);
                            tokenFront = false;
                        }
                        if(silver)
                        {
                            fmt::format_to(in, "{}{{amount={}, token=LC.token.SILVER}}",
                                           tokenFront ? "" : ", ", silver);
                            tokenFront = false;
                        }
                        if(copper)
                        {
                            fmt::format_to(in, "{}{{amount={}, token=LC.token.COPPER}}",
                                           tokenFront ? "" : ", ", copper);
                        }
                    }
                    else
                    {
                        for(auto &token : bartersList.currency)
                        {
                            auto it = ranges::find(info.currencies, token.id, &Currency::id);
                            if(it == info.currencies.end())
                            {
                                fmt::println("CURRENCY NOT FOUND");
                                return;
                            }
                            auto tokenName = convertToLuaGVarName(it->name.at(EN), info.strip);
                            fmt::format_to(in, "{}{{amount={}, token=LC.token.{}}}",
                                           tokenFront ? "" : ", ", token.amt, tokenName);
                            tokenFront = false;
                        }
                    }
                    fmt::format_to(in, "}},\n");
                    fmt::format_to(in, "{}", outputVendors(info, bartersList.bartererId));
                }
            }
        }
        if(skill.storeLP)
        {
            bool addComma = !buf.empty();
            fmt::format_to(in, "{}{} {{ store=true }}", addComma ? "," : "",
                           onlyCost ? "\n           " : "");
            onlyCost = false;
        }
        if(skill.autoRep)
        {
            bool addComma = !buf.empty();
            fmt::format_to(in, "{}{} {{ autoRep=true }}",
                           addComma ? "," : "",
                           onlyCost ? "\n           " : "");
            // onlyCost = false;
        }

        if(!buf.empty())
        {
            fmt::println(out, "        acquire={{{} }},", buf);
        }
    }
}

static string outputReputation(const Skill &skill, const TravelInfo &info)
{
    string s;
    auto factionIt = std::ranges::find(info.factions, skill.factionId, &Faction::id);
    if(factionIt == info.factions.end())
    {
        fmt::println("MISSING FACTION {}", skill.factionId);
        return s;
    }
    auto rankIt = factionIt->ranks.find(skill.factionRank);
    if(rankIt == factionIt->ranks.end())
    {
        fmt::println("MISSING FACTION RANK {}", skill.factionRank);
        return s;
    }
    auto rankLabelIt = ranges::find(info.repRanks, rankIt->second, &RepRank::key);
    if(rankLabelIt == info.repRanks.end())
    {
        fmt::println("MISSING FACTION RANK LABEL {}", rankIt->second);
        return s;
    }
    string factionTitle = convertToLuaGVarName(factionIt->name.at(EN), info.strip);
    string rankTitle = convertToLuaGVarName(rankLabelIt->name.at(EN), info.strip);
    s = fmt::format("rep=LC.rep.{}, repLevel=LC.repLevel.{},",
                    factionTitle, rankTitle);
    return s;
}

static string outputLabelTag(const LCLabel &tag)
{
    return fmt::format("{{EN=\"{}\", DE=\"{}\", FR=\"{}\", RU=\"{}\" }}",
                       tag.at(EN), tag.at(DE), tag.at(FR), tag.at(RU));
}

static string outputLabelField(std::optional<std::reference_wrapper<const LCLabel>> labelsRef,
                               std::string_view locale,
                               std::string_view name)
{
    if(labelsRef.has_value())
    {
        auto &labels = labelsRef.value().get();
        auto it = labels.find(locale);
        if(it != labels.end())
        {
            if(name == "desc")
                return fmt::format(", {}=[[{}]]", name, it->second);
            return fmt::format("{} {}=\"{}\"", name == "name" ? "" : ",", name, it->second);
        }
    }
    return {};
}

static string outputLabelFields(const Skill &skill, std::string_view locale)
{
    string lc;
    std::transform(locale.begin(), locale.end(), std::back_inserter(lc), ::tolower);
    if(skill.group == Skill::Type::Creep)
    {
        return fmt::format("{}", outputLabelField(skill.name, lc, "name"));
    }
    return fmt::format("{}{}{}{}{}{}",
                       outputLabelField(skill.name, lc, "name"),
                       outputLabelField(skill.desc, lc, "desc"),
                       outputLabelField(skill.label, lc, "label"),
                       outputLabelField(skill.detail, lc, "detail"),
                       outputLabelField(skill.zlabel, lc, "zlabel"),
                       outputLabelField(skill.zone, lc, "zone"));
}

void outputSkill(ostream &out, const TravelInfo &info, const Skill &skill, TravelOutputState &state)
{
    fmt::println(out, "    self.{}:AddSkill({{", getGroupName(skill.group));
    fmt::println(out, "        id=\"0x{:08X}\",", skill.id);
    fmt::println(out, "        EN={{{} }},", outputLabelFields(skill, EN));
    fmt::println(out, "        DE={{{} }},", outputLabelFields(skill, DE));
    fmt::println(out, "        FR={{{} }},", outputLabelFields(skill, FR));
    fmt::println(out, "        RU={{{} }},", outputLabelFields(skill, RU));
    fmt::println(out, "        map={},", outputMapList(skill.mapList));
    if(!skill.overlapIds.empty())
    {
        fmt::println(out, "        overlap={},", outputOverlapIds(skill.overlapIds));
    }
    outputAcquire(out, info, skill);
    if(skill.factionId)
    {
        fmt::println(out, "        {}", outputReputation(skill, info));
    }
    if(skill.minLevel)
    {
        fmt::println(out, "        minLevel={},", skill.minLevel);
    }
    else if(skill.minLevelInput)
    {
        fmt::println(out, "        minLevel={}, -- config", skill.minLevelInput);
    }
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

    fmt::println(out, "---[[ auto-generated travel skills ]] --\n\n");
    fmt::println(out, "function TravelDictionary:CreateDictionaries()");
    TravelOutputState state;
    auto groups = {Skill::Type::Hunter, Skill::Type::Warden, Skill::Type::Mariner,
        Skill::Type::Racial, Skill::Type::Gen, Skill::Type::Rep, Skill::Type::Creep};
    for(auto group : groups)
    {
        auto groupName = getGroupName(group);
        auto tagIt = info.labelTags.find(group);
        if(group == Skill::Type::Creep)
            fmt::println(out, "end\n\nfunction TravelDictionary:CreateCreepDictionary()");
        fmt::println(out, "    -- add the {} skills", groupName);
        if(tagIt != info.labelTags.end())
        {
            fmt::println(out, "    self.{}:AddLabelTag({})",
                         groupName, outputLabelTag(tagIt->second));
        }

        for(auto &skill : info.skills)
        {
            if(skill.group != group)
                continue;

            outputSkill(out, info, skill, state);
        }
        if(group == Skill::Type::Creep)
            fmt::println(out, "end");
    }
}

void outputLocaleDataFile(const TravelInfo &info)
{
    std::ofstream out("LocaleData.lua", ios::out);
    if(!out.is_open())
    {
        fmt::println("Failed to create LocaleData.lua");
        return;
    }

    fmt::println(out, "---[[ auto-generated travel skill locale data ]] --\n\n");
    fmt::println(out, "local Locale = {{\n"
                      "    [Turbine.Language.English] = {{}},\n"
                      "    [Turbine.Language.German] = {{}},\n"
                      "    [Turbine.Language.French] = {{}},\n"
                      "    [Turbine.Language.Russian] = {{}}\n"
                      "}}\n"
                      "local LC_EN = Locale[Turbine.Language.English]\n"
                      "local LC_DE = Locale[Turbine.Language.German]\n"
                      "local LC_FR = Locale[Turbine.Language.French]\n"
                      "local LC_RU = Locale[Turbine.Language.Russian]\n");
    fmt::println(out, "if GLocale == Turbine.Language.German then\n"
                      "    LC_DE = LC\n"
                      "elseif GLocale == Turbine.Language.French then\n"
                      "    LC_FR = LC\n"
                      "elseif GLocale == Turbine.Language.Russian then\n"
                      "    LC_RU = LC\n"
                      "else\n"
                      "    LC_EN = LC\n"
                      "end\n");

    fmt::println(out, "LC_EN.repLevel = {{}}");
    fmt::println(out, "LC_DE.repLevel = {{}}");
    fmt::println(out, "LC_FR.repLevel = {{}}");
    fmt::println(out, "LC_RU.repLevel = {{}}");
    fmt::println(out, "");

    for(const auto &rank : info.repRanks)
    {
        auto title = convertToLuaGVarName(rank.name.at(EN), info.strip);
        fmt::println(out, "LC_EN.repLevel.{} = \"{}\"",
                     title, extractNameAttr(rank.name.at(EN)));
        fmt::println(out, "LC_DE.repLevel.{} = \"{}\"",
                     title, extractNameAttr(rank.name.at(DE)));
        fmt::println(out, "LC_FR.repLevel.{} = \"{}\"",
                     title, extractNameAttr(rank.name.at(FR)));
        fmt::println(out, "LC_RU.repLevel.{} = \"{}\"",
                     title, extractNameAttr(rank.name.at(RU)));
        fmt::println(out, "");
    }

    fmt::println(out, "LC_EN.rep = {{}}");
    fmt::println(out, "LC_DE.rep = {{}}");
    fmt::println(out, "LC_FR.rep = {{}}");
    fmt::println(out, "LC_RU.rep = {{}}");
    fmt::println(out, "");

    for(const auto &faction : info.factions)
    {
        auto title = convertToLuaGVarName(faction.name.at(EN), info.strip);
        fmt::println(out, "LC_EN.rep.{} = \"{}\"", title, faction.name.at(EN));
        fmt::println(out, "LC_DE.rep.{} = \"{}\"", title, faction.name.at(DE));
        fmt::println(out, "LC_FR.rep.{} = \"{}\"", title, faction.name.at(FR));
        fmt::println(out, "LC_RU.rep.{} = \"{}\"", title, faction.name.at(RU));
        fmt::println(out, "");
    }

    fmt::println(out, "LC_EN.token = {{}}");
    fmt::println(out, "LC_DE.token = {{}}");
    fmt::println(out, "LC_FR.token = {{}}");
    fmt::println(out, "LC_RU.token = {{}}");
    fmt::println(out, "");
    fmt::println(out, "LC_EN.token.COPPER = \"Copper\"");
    fmt::println(out, "LC_DE.token.COPPER = \"Kupfer\"");
    fmt::println(out, "LC_FR.token.COPPER = \"Cuivre\"");
    fmt::println(out, "LC_RU.token.COPPER = \"Медь\"");
    fmt::println(out, "");
    fmt::println(out, "LC_EN.token.SILVER = \"Silver\"");
    fmt::println(out, "LC_DE.token.SILVER = \"Silber\"");
    fmt::println(out, "LC_FR.token.SILVER = \"Argent\"");
    fmt::println(out, "LC_RU.token.SILVER = \"Серебро\"");
    fmt::println(out, "");
    fmt::println(out, "LC_EN.token.GOLD = \"Gold\"");
    fmt::println(out, "LC_DE.token.GOLD = \"Gold\"");
    fmt::println(out, "LC_FR.token.GOLD = \"Or\"");
    fmt::println(out, "LC_RU.token.GOLD = \"Золото\"");
    fmt::println(out, "");
    fmt::println(out, "LC_EN.token.LOTRO_POINT = \"LOTRO Points\"");
    fmt::println(out, "LC_DE.token.LOTRO_POINT = \"HdRO-Punkte\"");
    fmt::println(out, "LC_FR.token.LOTRO_POINT = \"Points SdAO\"");
    fmt::println(out, "LC_RU.token.LOTRO_POINT = \"ВКО марки\"");
    fmt::println(out, "");
    for(const auto &currency : info.currencies)
    {
        auto title = convertToLuaGVarName(currency.name.at(EN), info.strip);
        fmt::println(out, "LC_EN.token.{} = \"{}\"", title, currency.name.at(EN));
        fmt::println(out, "LC_DE.token.{} = \"{}\"", title, currency.name.at(DE));
        fmt::println(out, "LC_FR.token.{} = \"{}\"", title, currency.name.at(FR));
        fmt::println(out, "LC_RU.token.{} = \"{}\"", title, currency.name.at(RU));
        fmt::println(out, "");
    }
}
