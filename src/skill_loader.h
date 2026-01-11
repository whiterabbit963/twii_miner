#ifndef SKILL_LOADER_H
#define SKILL_LOADER_H

#include <optional>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>

#include "xml_loader.h"

using namespace std::literals;

constexpr auto EN = "en";
constexpr auto DE = "de";
constexpr auto FR = "fr";
constexpr auto RU = "ru";

using LCLabelMap = std::map<std::string, std::string, std::less<>>;

struct LCLabel
{
    const std::string &at(std::string_view locale) const
    {
        static std::string s_empty{""};
        if(data.contains(locale))
        {
            auto it = data.find(locale);
            if(it != data.end() && it->second != s_empty)
                return it->second;
        }
        if(locale != EN && data.contains(EN))
            return data.at(EN);

        return s_empty;
    }
    const size_t size() const { return data.size(); }
    std::string &operator[](const std::string &name) { return data[name]; }
    bool empty() const { return data.empty(); }
    LCLabelMap data;
};

struct MapLoc
{
    enum class Region
    {
        Invalid, None,
        Eriador, Rhovanion, Rohan, Gondor, Haradwaith,
        Creep
    };
    Region region;
    int x, y;
};
using MapList = std::vector<MapLoc>;

enum class SkillCategory : uint32_t
{
    Hunter = 48,
    Creep = 97,
    Travel = 102,
};

struct Token
{
    uint32_t id{0};
    unsigned amt{0};
};

struct Barter
{
    uint32_t bartererId{0};
    double sellFactor{0};
    double buyAmt{0};
    std::vector<Token> currency;
};

struct Deed
{
    uint32_t id{0};
    LCLabel name;
};

struct Allegiance
{
    uint32_t id{0};
    uint32_t rank{0};
    LCLabel name;
};

struct Acquire
{
    uint32_t itemId{0};

    // barters & vendors
    std::vector<Barter> barters;

    // vendors
    uint32_t valueTableId{0};
    unsigned level{0};
    std::string quality;

    // quests
    uint32_t questId{0};
    std::string questNameKey;
    LCLabel questName;
};

struct Skill
{
    enum class SearchStatus
    {
        NotFound, // new skill
        Found, // good
        MultiFound // input error
    };

    enum class Type
    {
        Unknown,
        Ignore,
        Hunter,
        Warden,
        Mariner,
        Racial,
        Gen,
        Rep,
        Creep,
    };

    uint32_t id;
    std::string nameId;
    bool isNew{false};
    std::optional<std::string> race;
    SearchStatus status{SearchStatus::NotFound};
    Type group{Type::Unknown}; // parseable?
    std::optional<std::string> skillTag; // input
    LCLabel name; // parse
    std::optional<LCLabel> desc; // parse
    std::optional<LCLabel> label; // input
    std::optional<LCLabel> zone; // input
    std::optional<LCLabel> zlabel; // input
    std::optional<LCLabel> detail; // input
    std::optional<LCLabel> tag; // input
    MapList mapList; // input
    std::vector<uint32_t> overlapIds; // input
    std::vector<Acquire> acquire; // parse
    LCLabel acquireDesc;
    std::optional<Deed> acquireDeed;
    std::optional<Allegiance> allegiance;
    uint32_t factionId{0}; // parse
    unsigned factionRank{0}; // parse
    unsigned minLevel{0}; // parse
    unsigned minLevelInput{0}; // input
    std::string sortLevel; // input
    bool storeLP{false}; // input; adds acquired entry
    bool autoLevel{false}; // input; parseable??? acquired on minLevel for the class

    SkillCategory cat;
    std::string descKey;
};

struct Faction
{
    uint32_t id{0};
    LCLabel name;
    std::map<unsigned, std::string> ranks;
};

struct Currency
{
    uint32_t id{0};
    LCLabel name;
};

struct RepRank
{
    std::string key;
    LCLabel name;
};

struct NPC
{
    uint32_t id{0};
    std::string titleKey;
    LCLabel name;
    LCLabel title;
};

using FactionLabels = std::map<std::string, LCLabel, std::less<>>;
using Utf8Map = std::map<std::string_view, std::string_view>;

struct TravelInfo
{
    std::vector<Skill> skills;
    std::map<Skill::Type, std::vector<Skill>> newSkills;
    std::map<unsigned, Skill> inputs;
    std::map<Skill::Type, LCLabel> labelTags;
    std::vector<Currency> currencies;
    std::vector<Faction> factions;
    std::vector<RepRank> repRanks;
    std::vector<NPC> npcs;
    Utf8Map strip{{"á", "a"}, {"â", "a"}, {"ê", "e"}, {"ú", "u"},
                  {"é", "e"}, {"ó", "o"}, {"í", "i"}, {"û", "u"}};
};


class SkillLoader
{
public:
    SkillLoader(std::string_view root);

    std::vector<Skill> getSkills();

    bool getSkillNames(std::vector<Skill> &skills);
    bool getSkillNames(const std::string &locale, std::vector<Skill> &skills);
    bool getSkillDesc(const std::string &locale, std::vector<Skill> &skills);
    bool getSkillItems(std::vector<Skill> &skills);
    bool getClassInfo(std::vector<Skill> &skills);
    bool getQuests(std::vector<Skill> &skills);
    bool getQuestLabels(std::vector<Skill> &skills);
    bool getQuestLabel(const std::string &locale, std::vector<Skill> &skills);
    bool getTraits(std::vector<Skill> &skills);
    bool getDeeds(const std::unordered_map<std::string_view, Skill*> &traits,
                  const std::unordered_map<uint32_t, Skill*> &skills);
    bool getDeedLabels(std::vector<Skill> &skills);
    bool getDeedLabel(const std::string &locale, std::vector<Skill> &skills);

    bool getAllegiance(std::vector<Skill> &skills);
    bool getAllegianceLabels(std::vector<Skill> &skills);
    bool getAllegianceLabel(const std::string &locale, std::vector<Skill> &skills);

    bool getFactions(TravelInfo &info);
    bool getFactionLabels(TravelInfo &info);
    bool getFactionLabel(const std::string &locale, TravelInfo &info);

    bool getCurrencies(TravelInfo &info);
    bool getCurrencyLabels(TravelInfo &info);
    bool getCurrencyLabel(const std::string &locale, TravelInfo &info);

    bool getVendors(TravelInfo &info);
    bool getBarters(TravelInfo &info);
    bool getNPCTitleKeys(TravelInfo &info);
    bool getNPCLabels(TravelInfo &info);
    bool getNPCLabel(const std::string &locale, TravelInfo &info);
    uint32_t getValueTableValue(const Acquire &item);

private:
    std::string m_path;
    XMLLoader m_xml;
};

#endif // SKILL_LOADER_H
