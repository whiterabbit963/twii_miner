#include "skill_loader.h"

#include <array>
#include <ranges>
#include <fmt/format.h>

using namespace std;
using namespace rapidxml;

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
    getClassInfo(skills);

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
            it->desc = std::make_optional<LCLabel>();
            //fmt::println("{:08X} {}", it->id, it->name[locale]);
        }
        skill.desc = std::make_optional<LCLabel>();
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

static void loadClassSkillInfo(xml_node<> *root, std::vector<Skill> &skills)
{
    for(xml_node<> *node = root->first_node("classSkill");
            node; node = node->next_sibling("classSkill"))
    {
        xml_attribute<> *attr = node->first_attribute("skillId");
        if(!attr)
            continue;
        uint32_t skillId = atoi(attr->value());
        auto it = ranges::find(skills, skillId, &Skill::id);
        if(it == skills.end())
            continue;
        attr = node->first_attribute("minLevel");
        if(!attr)
            continue;
        unsigned minLevel = atoi(attr->value());
        if(!minLevel)
            continue;
        it->minLevel = minLevel;
        it->autoLevel = true;
    }
}

bool SkillLoader::getClassInfo(std::vector<Skill> &skills)
{
    string skillPath = fmt::format("{}\\lotro-data\\lore\\classes.xml", m_path);
    if(!m_xml.load(skillPath))
        return false;

    xml_node<> *root = m_xml.doc().first_node("classes");
    if(!root)
    {
        return false;
    }

    for(xml_node<> *node = root->first_node("class");
            node; node = node->next_sibling("class"))
    {
        xml_attribute<> *attr = node->first_attribute("key");
        if(!attr)
            continue;

        if(attr->value() == "Hunter"sv ||
                attr->value() == "Warden"sv ||
                attr->value() == "Corsair"sv)
        {
            loadClassSkillInfo(node, skills);
        }
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
        uint32_t itemId = atoi(itemKey.data());
        skill.acquire.push_back({itemId});

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
        }

        //fmt::println("{} {}", skill.id, skill.name[locale]);
    }
    return true;
}

bool SkillLoader::getFactionLabels(TravelInfo &info)
{
    if(!getFactionLabel(EN, info))
        return false;
    if(!getFactionLabel(DE, info))
        return false;
    if(!getFactionLabel(FR, info))
        return false;
    if(!getFactionLabel(RU, info))
        return false;
    return true;
}

bool SkillLoader::getFactionLabel(const std::string &locale, TravelInfo &info)
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

        attr = node->first_attribute("value");
        if(!attr)
            continue;

        string_view value = attr->value();
        if(key.starts_with("key"))
        {
            auto it = ranges::find(info.repRanks, key, &RepRank::key);
            if(it != info.repRanks.end())
            {
                it->name[locale] = value;
            }
        }
        else
        {
            uint32_t factionId = atoi(key.data());
            auto it = ranges::find(info.factions, factionId, &Faction::id);
            if(it != info.factions.end())
            {
                it->name[locale] = attr->value();
            }
        }
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
bool SkillLoader::getFactions(TravelInfo &info)
{
    string fp = fmt::format("{}\\lotro-data\\lore\\factions.xml", m_path);
    if(!m_xml.load(fp))
        return false;

    xml_node<> *root = m_xml.doc().first_node("factions");
    if(!root)
        return false;

    for(xml_node<> *node = root->first_node("faction");
            node; node = node->next_sibling("faction"))
    {
        xml_attribute<> *attr = node->first_attribute("id");
        if(!attr)
            continue;

        Faction faction;
        string_view factionId = attr->value();
        faction.id = atoi(factionId.data());
        auto skillIt = std::ranges::find(info.skills, faction.id, &Skill::factionId);
        if(skillIt == info.skills.end())
            continue;

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
            string labelKey = attr->value();
            faction.ranks.insert({rank, labelKey});

            auto it = ranges::find(info.repRanks, labelKey, &RepRank::key);
            if(it == info.repRanks.end())
            {
                for(auto &skill : info.skills)
                {
                    if(skill.factionId == faction.id && skill.factionRank == rank)
                    {
                        info.repRanks.push_back(RepRank{labelKey, {}});
                        break;
                    }
                }
            }
        }
        info.factions.push_back(faction);
    }

    if(!getFactionLabels(info))
        return false;

    return true;
}

bool SkillLoader::getCurrencyLabels(TravelInfo &info)
{
    if(!getCurrencyLabel(EN, info))
        return false;
    if(!getCurrencyLabel(DE, info))
        return false;
    if(!getCurrencyLabel(FR, info))
        return false;
    if(!getCurrencyLabel(RU, info))
        return false;
    return true;
}

bool SkillLoader::getCurrencyLabel(const string &locale, TravelInfo &info)
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

        uint32_t tokenId = atoi(key.data());
        auto tokenIt = ranges::find(info.currencies, tokenId, &Currency::id);
        if(tokenIt != info.currencies.end())
        {
            tokenIt->name[locale] = attr->value();
        }
    }
    return true;
}

// <paperItem identifier="1879416779" name="Silver Coin of Gundabad" itemClass="27" category="15" free="true" iconId="1092667064" cap="500"/>
bool SkillLoader::getCurrencies(TravelInfo &info)
{
    if(!getBarters(info))
        return false;

    if(!getCurrencyLabels(info))
       return false;

    return true;
}

// <barterProfile profileId="1879501119" name="Temple of Utug-bûr Rewards">
// ...
// <barterEntry>
// <give id="1879496354" name="Cold-iron Token"/>
// <receive id="1879501345" name="Muster at Utug-bûr"/>
// </barterEntry>
// ...
// </barterProfile>
bool SkillLoader::getBarters(TravelInfo &info)
{
    string fp = fmt::format("{}\\lotro-data\\lore\\barters.xml", m_path);
    if(!m_xml.load(fp))
        return false;

    xml_node<> *root = m_xml.doc().first_node("barterers");
    if(!root)
        return false;

    info.currencies.reserve(300);
    for(xml_node<> *proNode = root->first_node("barterProfile");
            proNode; proNode = proNode->next_sibling("barterProfile"))
    {
        for(xml_node<> *brtrNode = proNode->first_node("barterEntry");
                brtrNode; brtrNode = brtrNode->next_sibling("barterEntry"))
        {
            for(xml_node<> *recvNode = brtrNode->first_node("receive");
                    recvNode; recvNode = recvNode->next_sibling("receive"))
            {
                xml_attribute<> *recvAttr = recvNode->first_attribute("id");
                if(!recvAttr)
                    continue;
                uint32_t itemId = atoi(recvAttr->value());
                std::vector<Acquire>::iterator acquireIt{};
                auto skillIt = ranges::find_if(info.skills, [&acquireIt, itemId](auto &skill)
                {
                    acquireIt = ranges::find(skill.acquire, itemId, &Acquire::itemId);
                    return acquireIt != skill.acquire.end();
                });
                if(skillIt == info.skills.end())
                {
                    continue;
                }
                acquireIt->currency.push_back({});
                auto &tokenList = acquireIt->currency.back();
                for(xml_node<> *giveNode = brtrNode->first_node("give");
                        giveNode; giveNode = giveNode->next_sibling("give"))
                {
                    xml_attribute<> *giveAttr = giveNode->first_attribute("id");
                    if(!giveAttr)
                        continue;
                    Token token;
                    token.amt = 1;
                    token.id = atoi(giveAttr->value());

                    giveAttr = giveNode->first_attribute("quantity");
                    if(giveAttr)
                    {
                        token.amt = atoi(giveAttr->value());
                    }

                    tokenList.push_back(token);

                    auto tokenIt = ranges::find(info.currencies, token.id, &Currency::id);
                    if(tokenIt == info.currencies.end())
                    {
                        info.currencies.push_back({token.id});
                    }
                }
            }
        }
    }
    return true;
}

#if 0
bool SkillLoader::getBarterLabels(TravelInfo &info)
{
    if(!getBarterLabel(EN, info))
        return false;
    if(!getBarterLabel(FR, info))
        return false;
    if(!getBarterLabel(DE, info))
        return false;
    if(!getBarterLabel(RU, info))
        return false;
    return true;
}

bool SkillLoader::getBarterLabel(const std::string &locale, TravelInfo &info)
{
    string fp = fmt::format("{}\\lotro-data\\lore\\labels\\{}\\barterers.xml", m_path, locale);
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
        string_view value = attr->value();


        for(auto &skill : info.skills)
        {

        }
    }
    return true;
}
#endif
