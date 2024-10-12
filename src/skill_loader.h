#ifndef SKILL_LOADER_H
#define SKILL_LOADER_H

#include <memory>
#include <vector>
#include <string>
#include <map>
#include "xml_loader.h"

using namespace std::literals;

constexpr auto EN = "en"sv;
constexpr auto DE = "de"sv;
constexpr auto FR = "fr"sv;
constexpr auto RU = "ru"sv;

using LCLabel = std::map<std::string_view, std::string>;

struct MapLoc
{
    enum class Region
    {
        Eriador, Rhovanion, Rohan, Mordor, Haradwaith
    };
    Region region;
    int x, y;
};

struct Acquire
{
    bool autoLevel{false};
};

struct Reputation
{
    std::string faction;
    std::string rank;
};

enum class SkillCategory : uint32_t
{
    Hunter = 48,
    Creep = 97,
    Travel = 102,
};

struct Skill
{
    enum class Type
    {
        Hunter,
        Warden,
        Mariner,
        Gen,
        Rep,
        Creep,
    };

    Type group; // parseable?
    uint32_t id;
    LCLabel name;
    std::unique_ptr<LCLabel> desc;
    LCLabel label; // dev-input
    std::vector<MapLoc> mapLoc; // dev-input
    std::vector<uint32_t> overlapIds; // dev-input
    std::string tag; // dev-input; generally empty
    Reputation rep; // parse
    unsigned minLevel; // parse
    float sortLevel; // dev-input
    bool storeLP{false}; // dev-input; adds acquired entry
    bool autoRep{false}; // dev-input; parseable??? adds acquired entry
    bool autoLevel{false}; // dev-input; parseable??? acquired on minLevel for the class

    SkillCategory cat;
    std::string descKey;
};

class SkillLoader
{
public:
    SkillLoader(std::string_view root);

    std::vector<Skill> getSkills();
    bool getSkillNames(std::vector<Skill> &skills);
    bool getSkillNames(std::string_view locale, std::vector<Skill> &skills);
    bool getSkillDesc(std::string_view locale, std::vector<Skill> &skills);

private:
    std::string m_path;
    XMLLoader m_xml;
};

#endif // SKILL_LOADER_H
