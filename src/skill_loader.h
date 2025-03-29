#ifndef SKILL_LOADER_H
#define SKILL_LOADER_H

#include <memory>
#include <vector>
#include <string>

// #ifdef _DEBUG
// #undef _ITERATOR_DEBUG_LEVEL
// #endif

#include <map>
#include <unordered_map>

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
    uint32_t itemId{0};
    LCLabel name;
    std::unique_ptr<LCLabel> desc;
    LCLabel label; // input
    MapList mapList; // input
    std::vector<uint32_t> overlapIds; // input
    std::string tag; // input; generally empty
    uint32_t factionId{0}; // parse
    unsigned factionRank{0}; // parse
    unsigned minLevel{0}; // parse
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
    std::string keyName; // in caps; use for LUA tag
    LCLabel name;
    std::map<unsigned, LCLabel> ranks;
    bool used{false};
};

struct Currency
{
    uint32_t id{0};
    LCLabel name;
    bool used{false};
};

using FactionLabels = std::map<std::string, LCLabel, std::less<>>;
using CurrencyLabels = std::map<std::string, LCLabel, std::less<>>;

struct TravelInfo
{
    std::map<Skill::Type, LCLabel> labelTags;
    std::vector<Skill> skills;
    std::vector<Currency> currencies;
    std::vector<Faction> factions;
    CurrencyLabels currencyLabels;
};


class SkillLoader
{
public:
    SkillLoader(std::string_view root);

    std::vector<Skill> getSkills();
    std::vector<Currency> getCurrencies(CurrencyLabels &labels);
    std::vector<Faction> getFactions();

    bool getSkillNames(std::vector<Skill> &skills);
    bool getSkillNames(const std::string &locale, std::vector<Skill> &skills);
    bool getSkillDesc(const std::string &locale, std::vector<Skill> &skills);
    bool getSkillItems(std::vector<Skill> &skills);

    bool getFactionLabels(FactionLabels &labels);
    bool getFactionLabel(const std::string &locale, FactionLabels &labels);

    bool getCurrencyLabels(CurrencyLabels &labels);
    bool getCurrencyLabel(const std::string &locale, CurrencyLabels &labels);

private:
    std::string m_path;
    XMLLoader m_xml;
};

#endif // SKILL_LOADER_H
