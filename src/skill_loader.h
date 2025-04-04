#ifndef SKILL_LOADER_H
#define SKILL_LOADER_H

#include <optional>
#include <vector>
#include <string>

// #ifdef _DEBUG
// #undef _ITERATOR_DEBUG_LEVEL
// #endif

#include <map>

// #ifdef _DEBUG
// #define _ITERATOR_DEBUG_LEVEL 2
// #endif

#include "xml_loader.h"

using namespace std::literals;

constexpr auto EN = "en";
constexpr auto DE = "de";
constexpr auto FR = "fr";
constexpr auto RU = "ru";

using LCLabel = std::map<std::string, std::string, std::less<>>;

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

struct Acquire
{
    uint32_t itemId{0};
    uint32_t valueTableId{0};
    unsigned level{0};
    std::string quality;
    std::vector<Barter> barters;
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
        Hunter,
        Warden,
        Mariner,
        Racial,
        Gen,
        Rep,
        Creep,
    };

    uint32_t id;
    SearchStatus status{SearchStatus::NotFound};
    Type group{Type::Unknown}; // parseable?
    LCLabel name; // parse
    std::optional<LCLabel> desc; // parse
    std::optional<LCLabel> label; // input
    std::optional<LCLabel> zone; // input
    std::optional<LCLabel> zlabel; // input
    std::optional<LCLabel> detail; // input
    MapList mapList; // input
    std::vector<uint32_t> overlapIds; // input
    std::string tag; // input; generally empty
    std::vector<Acquire> acquire; // parse
    uint32_t factionId{0}; // parse
    unsigned factionRank{0}; // parse
    unsigned minLevel{0}; // parse
    unsigned minLevelInput{0}; // input
    std::string sortLevel; // input
    bool storeLP{false}; // input; adds acquired entry
    bool autoRep{false}; // input; parseable??? adds acquired entry
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
    std::map<Skill::Type, LCLabel> labelTags;
    std::vector<Skill> skills;
    std::vector<Currency> currencies;
    std::vector<Faction> factions;
    std::vector<RepRank> repRanks;
    std::vector<NPC> npcs;
    Utf8Map strip{{"á", "a"}, {"â", "a"}, {"ê", "e"}, {"ú", "u"},
                  {"é", "e"}, {"ó", "o"}, {"í", "i"}};
};


class SkillLoader
{
public:
    SkillLoader(std::string_view root);

    std::vector<Skill> getSkills();
    bool getFactions(TravelInfo &info);
    bool getCurrencies(TravelInfo &info);

    bool getSkillNames(std::vector<Skill> &skills);
    bool getSkillNames(const std::string &locale, std::vector<Skill> &skills);
    bool getSkillDesc(const std::string &locale, std::vector<Skill> &skills);
    bool getSkillItems(std::vector<Skill> &skills);
    bool getClassInfo(std::vector<Skill> &skill);

    bool getFactionLabels(TravelInfo &info);
    bool getFactionLabel(const std::string &locale, TravelInfo &info);

    bool getCurrencyLabels(TravelInfo &info);
    bool getCurrencyLabel(const std::string &locale, TravelInfo &info);

    bool getVendors(TravelInfo &info);
    bool getBarters(TravelInfo &info);
    bool getBartererTitleKeys(TravelInfo &info);
    bool getNPCLabels(TravelInfo &info);
    bool getNPCLabel(const std::string &locale, TravelInfo &info);
    uint32_t getValueTableValue(const Acquire &item);

private:
    std::string m_path;
    XMLLoader m_xml;
};

#endif // SKILL_LOADER_H
