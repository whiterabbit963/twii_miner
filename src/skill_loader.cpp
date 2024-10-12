#include "skill_loader.h"

#include <array>
#include <fmt/format.h>

using namespace std;
using namespace rapidxml;

constexpr string_view s_itemPath = "C:\\projects\\lotro-items-db";
constexpr string_view s_itemsFn = "items.xml";

// <barterProfile profileId="1879297432" name="Cosmetic Cloaks">
//  <barterEntry>
//   <give id="1879255991" name="Mithril Coin" quantity="30"/>
//   <receive id="1879188485" name="Guide to Ost Guruth"/>
//  </barterEntry>
// </barterProfile>
constexpr string_view s_bartersFn = "barters.xml";
constexpr string_view s_skillsFn = "skills.xml";
constexpr string_view s_skillsDetailsFn = "skillsDetails.xml";

// <trait identifier="1879180386" name="Travel to Ost Guruth" iconId="1091404797" minLevel="25" category="29" nature="1" tooltip="key:620858129:191029568" description="key:620858129:54354734">
// <skill id="1879180353" name="Return to Ost Guruth"/>
// </trait>
constexpr string_view s_traitsFn = "traits.xml";

constexpr array<uint32_t, 2> s_blacklist{
    1879064384, // Desperate Flight
    1879145101, // Desperate Flight
};


SkillLoader::SkillLoader(std::string_view root) : m_path(root) {}

std::vector<Skill> SkillLoader::getSkills()
{
    string skillPath = fmt::format("{}\\lore\\{}", m_path, s_skillsFn);
    if(!m_xml.load(skillPath))
        return {};

    xml_node<> *root = m_xml.doc().first_node("skills");
    if(!root)
    {
        fmt::println("missing skills tag");
        return {};
    }

    vector<Skill> skills;
    for(xml_node<> *node = root->first_node("travelSkill");
            node; node = node->next_sibling("travelSkill"))
    {
        Skill skill;
        xml_attribute<> *attr = node->first_attribute("category");
        if(attr)
            skill.cat = static_cast<SkillCategory>(atoi(attr->value()));

        attr = node->first_attribute("identifier");
        if(attr)
            skill.id = atoi(attr->value());

        attr = node->first_attribute("description");
        if(attr)
            skill.descKey = attr->value();

        skills.emplace_back(std::move(skill));
    }

    // acquire creep & hunter travel skills
    for(xml_node<> *node = root->first_node("skill");
            node; node = node->next_sibling("skill"))
    {
        Skill skill;
        xml_attribute<> *attr = node->first_attribute("category");
        if(!attr)
            continue;

        skill.cat = static_cast<SkillCategory>(atoi(attr->value()));
        if(skill.cat != SkillCategory::Creep && skill.cat != SkillCategory::Hunter)
            continue;

        attr = node->first_attribute("identifier");
        if(!attr)
            continue;

        skill.id = atoi(attr->value());
        if(std::ranges::find(s_blacklist, skill.id) != s_blacklist.end())
            continue;

        attr = node->first_attribute("description");
        if(attr)
            skill.descKey = attr->value();

        skills.emplace_back(std::move(skill));
    }

    return skills;
}

bool SkillLoader::getSkillNames(vector<Skill> &skills)
{
    if(!getSkillNames(EN, skills))
        return false;
    if(!getSkillNames(DE, skills))
        return false;
    if(!getSkillNames(FR, skills))
        return false;
    if(!getSkillNames(RU, skills))
        return false;

    // NOTE: these steps are broken up since
    //       some languages have different
    //       sets of identical names
    if(!getSkillDesc(EN, skills))
        return false;
    if(!getSkillDesc(DE, skills))
        return false;
    if(!getSkillDesc(FR, skills))
        return false;
    if(!getSkillDesc(RU, skills))
        return false;
    return true;
}

bool SkillLoader::getSkillNames(string_view locale, vector<Skill> &skills)
{
    string fp = fmt::format("{}\\lore\\labels\\{}\\{}", m_path, locale, s_skillsFn);
    if(!m_xml.load(fp))
        return false;

    xml_node<> *root = m_xml.doc().first_node("labels");
    if(!root)
        return false;
    xml_attribute<> *locAttr = root->first_attribute("locale");
    if(!locAttr)
        return false;
    if(locAttr->value() != locale)
        return false;

    for(xml_node<> *node = root->first_node("label");
            node; node = node->next_sibling("label"))
    {
        xml_attribute<> *attr = node->first_attribute("key");
        if(!attr)
            continue;

        uint32_t key = atoi(attr->value());
        auto it = std::ranges::find(skills, key, &Skill::id);
        if(it == skills.end())
            continue;

        auto &skill = *it;
        attr = node->first_attribute("value");
        if(attr)
            skill.name[locale] = attr->value();
    }

    // disambiguate identical skill names
    for(auto &skill : skills)
    {
        if(skill.desc)
            continue;

        auto it = std::ranges::find_if(skills, [&](auto &s) {
            return skill.id != s.id && skill.name[locale] == s.name[locale];
        });

        if(it == skills.end())
            continue;

        if(!it->desc)
        {
            it->desc = std::make_unique<LCLabel>();
            //fmt::println("{:08X} {}", it->id, it->name[locale]);
        }
        skill.desc = std::make_unique<LCLabel>();
        //fmt::println("{:08X} {}", skill.id, skill.name[locale]);
    }

    return true;
}

bool SkillLoader::getSkillDesc(string_view locale, vector<Skill> &skills)
{
    string fp = fmt::format("{}\\lore\\labels\\{}\\{}", m_path, locale, s_skillsFn);
    if(!m_xml.load(fp))
        return false;

    xml_node<> *root = m_xml.doc().first_node("labels");
    if(!root)
        return false;
    xml_attribute<> *locAttr = root->first_attribute("locale");
    if(!locAttr)
        return false;
    if(locAttr->value() != locale)
        return false;

    for(xml_node<> *node = root->first_node("label");
            node; node = node->next_sibling("label"))
    {
        xml_attribute<> *attr = node->first_attribute("key");
        if(!attr)
            continue;

        std::string_view key = attr->value();
        auto it = std::ranges::find(skills, key, &Skill::descKey);
        if(it == skills.end())
            continue;

        auto &skill = *it;
        if(!skill.desc)
            continue;

        attr = node->first_attribute("value");
        if(!attr)
            continue;

        (*skill.desc)[locale] = attr->value();
        //fmt::println("{} {}", skill.id, skill.name[locale]);
    }
    return true;
}
