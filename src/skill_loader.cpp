#include "skill_loader.h"

#include <array>
#include <ranges>
#include <fmt/format.h>

using namespace std;
using namespace rapidxml;

// <barterProfile profileId="1879297432" name="Cosmetic Cloaks">
//  <barterEntry>
//   <give id="1879255991" name="Mithril Coin" quantity="30"/>
//   <receive id="1879188485" name="Guide to Ost Guruth"/>
//  </barterEntry>
// </barterProfile>
constexpr string_view s_bartersFn = "barters.xml";
constexpr string_view s_skillsDetailsFn = "skillsDetails.xml";

// <trait identifier="1879180386" name="Travel to Ost Guruth" iconId="1091404797" minLevel="25" category="29" nature="1" tooltip="key:620858129:191029568" description="key:620858129:54354734">
// <skill id="1879180353" name="Return to Ost Guruth"/>
// </trait>
constexpr string_view s_traitsFn = "traits.xml";

constexpr array<uint32_t, 2> s_blacklist{
    1879064384, // Desperate Flight
    1879145101, // Desperate Flight
};

Skill::Type getGroupTypeFromName(string_view name)
{
    if(name == "Warden"sv)
        return Skill::Type::Warden;
    if(name == "Hunter"sv)
        return Skill::Type::Hunter;
    if(name == "Mariner"sv || name == "Corsair"sv)
        return Skill::Type::Mariner;
    return Skill::Type::Unknown;
}


SkillLoader::SkillLoader(std::string_view root) : m_path(root) {}

std::vector<Skill> SkillLoader::getSkills()
{
    string skillPath = fmt::format("{}\\lotro-data\\lore\\skills.xml", m_path);
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

    getSkillNames(skills);
    getSkillItems(skills);

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

bool SkillLoader::getSkillNames(const string &locale, vector<Skill> &skills)
{
    string fp = fmt::format("{}\\lotro-data\\lore\\labels\\{}\\skills.xml", m_path, locale);
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

bool SkillLoader::getSkillDesc(const string &locale, vector<Skill> &skills)
{
    string fp = fmt::format("{}\\lotro-data\\lore\\labels\\{}\\skills.xml", m_path, locale);
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

// <item key="1879501345" name="Muster at Utug-bûr" icon="1090531744-1090519043-1090531745" level="5" category="ITEM" class="105" binding="BIND_ON_ACQUIRE" unique="true"
//   quality="RARE" minLevel="140" requiredClass="Warden" requiredFaction="1879489736;3" description="key:621104289:54354734" valueTableId="1879094316">
// <stats/>
// <grants type="SKILL" id="1879501344"/>
// <effect type="ON_USE" id="1879090911" name="GrantSkillEffect"/>
// </item>
bool SkillLoader::getSkillItems(std::vector<Skill> &skills)
{
    string fp = fmt::format("{}\\lotro-items-db\\items.xml", m_path);
    if(!m_xml.load(fp))
        return false;

    xml_node<> *root = m_xml.doc().first_node("items");
    if(!root)
        return false;

    for(xml_node<> *node = root->first_node("item");
            node; node = node->next_sibling("item"))
    {
        xml_attribute<> *attr = node->first_attribute("key");
        if(!attr)
            continue;

        std::string_view itemKey = attr->value();
        // TODO: capture other attr values
        xml_node<> *grants = node->first_node("grants");
        if(!grants)
            continue;

        attr = grants->first_attribute("id");
        if(!attr)
            continue;
        uint32_t key = atoi(attr->value());
        auto it = std::ranges::find(skills, key, &Skill::id);
        if(it == skills.end())
            continue;

        auto &skill = *it;
        skill.itemId = key;
        if(attr = node->first_attribute("minLevel"); attr)
            skill.minLevel = atoi(attr->value());
        if(attr = node->first_attribute("requiredClass"); attr)
            skill.group = getGroupTypeFromName(attr->value());
        if(attr = node->first_attribute("requiredFaction"); attr)
        {
            unsigned i = 0;
            string_view words{attr->value()};
            for(const auto word : std::ranges::split_view(words, ";"sv))
            {
                // TODO: add method to lookup factionId's and ranks
                switch(i)
                {
                case 0: skill.factionId = atoi(word.data()); break;
                case 1: skill.factionRank = atoi(word.data()); break;
                default: break;
                }
                ++i;
            }
        }
        if(skill.group == Skill::Type::Unknown)
        {
            if(skill.cat == SkillCategory::Creep)
            {
                skill.group = Skill::Type::Creep;
            }
            else
            {
                skill.group = Skill::Type::Rep;
            }
            // TODO: hardcode the Skill::Type::Gen && Creep
        }

        //fmt::println("{} {}", skill.id, skill.name[locale]);
    }
    return true;
}


bool SkillLoader::getFactionLabels(FactionLabels &labels)
{
    if(!getFactionLabel(EN, labels))
        return false;
    if(!getFactionLabel(DE, labels))
        return false;
    if(!getFactionLabel(FR, labels))
        return false;
    if(!getFactionLabel(RU, labels))
        return false;
    return true;
}

bool SkillLoader::getFactionLabel(const std::string &locale, FactionLabels &labels)
{
    string fp = fmt::format("{}\\lotro-data\\lore\\labels\\{}\\factions.xml", m_path, locale);
    if(!m_xml.load(fp))
        return false;

    xml_node<> *root = m_xml.doc().first_node("labels");
    if(!root)
        return false;

    for(xml_node<> *node = root->first_node("label");
            node; node = node->next_sibling("label"))
    {
        xml_attribute<> *attr = node->first_attribute("key");
        if(!attr)
            continue;
        string_view key = attr->value();
        attr = node->first_attribute("value");
        if(!attr)
            continue;
        auto &lcItem = labels.insert({string{key}, {}}).first->second;
        lcItem.insert({locale, attr->value()});
    }
    return true;
}

// <factions>
//     <faction id="1879091345" key="SHIRE" name="The Mathom Society" category="Eriador" lowestTier="1" initialTier="3" highestTier="7" currentTierProperty="Reputation_Faction_Shire_Mathoms_CurrentTier" currentReputationProperty="Reputation_Faction_Shire_Mathoms_EarnedReputation" description="key:620792414:54354734">
//     <level tier="1" key="ENEMY" name="key:620879993:74285329" lotroPoints="0" requiredReputation="0"/>
//     <level tier="2" key="OUTSIDER" name="key:620879993:74285330" lotroPoints="0" requiredReputation="10000"/>
//     <level tier="3" key="NEUTRAL" name="key:620879993:74285331" lotroPoints="0" requiredReputation="20000"/>
//     <level tier="4" key="ACQUAINTANCE" name="key:620879993:74285332" lotroPoints="5" requiredReputation="30000" deedKey="Known_to_the_Mathom_Society"/>
//     <level tier="5" key="FRIEND" name="key:620879993:74285333" lotroPoints="10" requiredReputation="50000" deedKey="Friend_to_the_Mathom_Society"/>
//     <level tier="6" key="ALLY" name="key:620879993:74285334" lotroPoints="15" requiredReputation="75000" deedKey="Ally_of_the_Mathom_Society"/>
//     <level tier="7" key="KINDRED" name="key:620879993:74285335" lotroPoints="20" requiredReputation="105000" deedKey="Kindred_with_the_Mathom_Society"/>
// </faction>
std::vector<Faction> SkillLoader::getFactions()
{
    std::vector<Faction> factions;
    FactionLabels factionLabels;
    if(!getFactionLabels(factionLabels))
        return {};

    string fp = fmt::format("{}\\lotro-data\\lore\\factions.xml", m_path);
    if(!m_xml.load(fp))
        return {};

    xml_node<> *root = m_xml.doc().first_node("factions");
    if(!root)
        return {};

    for(xml_node<> *node = root->first_node("faction");
            node; node = node->next_sibling("faction"))
    {
        Faction faction;
        xml_attribute<> *attr = node->first_attribute("id");
        if(!attr)
            continue;
        string_view factionId = attr->value();
        faction.id = atoi(factionId.data());

        attr = node->first_attribute("key");
        if(!attr)
            continue;
        faction.keyName = attr->value();

        attr = node->first_attribute("name");
        if(!attr)
            continue;
        string_view name = attr->value();
        auto nameLabels = factionLabels.find(factionId);
        if(nameLabels == factionLabels.end())
            continue;
        faction.name = nameLabels->second;

        for(xml_node<> *level = node->first_node("level");
                level; level = level->next_sibling("level"))
        {
            attr = level->first_attribute("tier");
            if(!attr)
                continue;
            unsigned rank = atoi(attr->value());
            attr = level->first_attribute("name");
            if(!attr)
                continue;
            string_view labelKey = attr->value();
            auto tierLabels = factionLabels.find(labelKey);
            if(tierLabels == factionLabels.end())
                continue;
            faction.ranks.insert({rank, tierLabels->second});
        }
        factions.push_back(faction);
    }
    return factions;
}

bool SkillLoader::getCurrencyLabels(CurrencyLabels &labels)
{
    if(!getCurrencyLabel(EN, labels))
        return false;
    if(!getCurrencyLabel(DE, labels))
        return false;
    if(!getCurrencyLabel(FR, labels))
        return false;
    if(!getCurrencyLabel(RU, labels))
        return false;
    return true;
}

bool SkillLoader::getCurrencyLabel(const string &locale, CurrencyLabels &labels)
{
    string fp = fmt::format("{}\\lotro-data\\lore\\labels\\{}\\items.xml", m_path, locale);
    if(!m_xml.load(fp))
        return false;

    xml_node<> *root = m_xml.doc().first_node("labels");
    if(!root)
        return false;

    for(xml_node<> *node = root->first_node("label");
         node; node = node->next_sibling("label"))
    {
        xml_attribute<> *attr = node->first_attribute("key");
        if(!attr)
            continue;
        string_view key = attr->value();
        if(key.starts_with("key"sv))
            continue;
        attr = node->first_attribute("value");
        if(!attr)
            continue;
        //string keyId = atoi(key.data());
        auto it = labels.find(key);
        if(it == labels.end())
            it = labels.insert({string{key}, {}}).first;
        auto &lcItem = it->second;
        lcItem.insert({locale, attr->value()});
    }
    return true;
}

// <paperItem identifier="1879416779" name="Silver Coin of Gundabad" itemClass="27" category="15" free="true" iconId="1092667064" cap="500"/>
std::vector<Currency> SkillLoader::getCurrencies(CurrencyLabels &currencyLabels)
{
    std::vector<Currency> currency;
    if(!getCurrencyLabels(currencyLabels))
        return {};

    string fp = fmt::format("{}\\lotro-data\\lore\\paperItems.xml", m_path);
    if(!m_xml.load(fp))
        return {};

    xml_node<> *root = m_xml.doc().first_node("paperItems");
    if(!root)
        return {};

    for(xml_node<> *node = root->first_node("paperItem");
            node; node = node->next_sibling("paperItem"))
    {
        Currency token;
        xml_attribute<> *attr = node->first_attribute("identifier");
        if(!attr)
            continue;
        string_view key = attr->value();
        token.id = atoi(key.data());

        auto lcLabel = currencyLabels.find(key);
        if(lcLabel == currencyLabels.end())
            continue;
        token.name = lcLabel->second;
        currency.push_back(token);
    }

    return currency;
}
