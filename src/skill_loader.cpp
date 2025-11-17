#include "skill_loader.h"

#include <ranges>
#include <unordered_map>
#include <regex>
#include <fmt/format.h>

using namespace std;
using namespace rapidxml;

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

static string fixXmlStr(string_view str)
{
    string buf;
    auto out = back_inserter(buf);
    std::regex quote("\\\\q");
    std::regex_replace(out, str.begin(), str.end(), quote, "\\\"");
    return buf;
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

        attr = node->first_attribute("description");
        if(attr)
            skill.descKey = attr->value();

        skills.emplace_back(std::move(skill));
    }

    getSkillNames(skills);
    getSkillItems(skills);
    getClassInfo(skills);
    getQuests(skills);
    getTraits(skills);

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
            skill.name[locale] = fixXmlStr(attr->value());
    }

    // disambiguate identical skill names
    for(auto &skill : skills)
    {
        if(skill.desc)
            continue;

        auto it = std::ranges::find_if(skills, [&](auto &s) {
            return skill.id != s.id &&
                   skill.name[locale] != "" &&
                   skill.name[locale] == s.name[locale];
        });

        if(it == skills.end())
            continue;

        if(!it->desc)
        {
            it->desc = std::make_optional<LCLabel>();
        }
        skill.desc = std::make_optional<LCLabel>();
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

        (*skill.desc)[locale] = fixXmlStr(attr->value());
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
        Acquire acquire;
        acquire.itemId = atoi(itemKey.data());
        if(attr = node->first_attribute("valueTableId"); attr)
            acquire.valueTableId = atoi(attr->value());
        if(attr = node->first_attribute("level"); attr)
            acquire.level = atoi(attr->value());
        if(attr = node->first_attribute("quality"); attr)
            acquire.quality = attr->value();
        skill.acquire.push_back(acquire);

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
    info.currencies.push_back({1879255991}); // Mithril Coins
    if(!getBarters(info))
        return false;

    if(!getVendors(info))
        return false;

    if(!getNPCTitleKeys(info))
        return false;

    if(!getNPCLabels(info))
        return false;

    if(!getCurrencyLabels(info))
       return false;

    return true;
}

static vector<uint32_t> getBartererId(xml_node<> *root, xml_node<> *proNode)
{
    // now that barterEntries have been parsed
    // get the profileId for the barterProfile
    // and search through barterer starting from root
    vector<uint32_t> barterIds;
    xml_attribute<> *attr = proNode->first_attribute("profileId");
    if(!attr)
        return barterIds;
    const uint32_t profileId = atoi(attr->value());
    for(xml_node<> *brtrNode = root->first_node("barterer");
            brtrNode; brtrNode = brtrNode->next_sibling("barterer"))
    {
        for(xml_node<> *bpNode = brtrNode->first_node("barterProfile");
                bpNode; bpNode = bpNode->next_sibling("barterProfile"))
        {
            attr = bpNode->first_attribute("profileId");
            if(!attr)
                continue;
            uint32_t matchProId = atoi(attr->value());
            if(matchProId != profileId)
                continue;
            attr = brtrNode->first_attribute("id");
            if(!attr)
                continue;
            barterIds.push_back(atoi(attr->value()));
        }
    }
    return barterIds;
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
        xml_attribute<> *attr = proNode->first_attribute("requiredFaction");
        string_view factionKey;
        if(attr)
            factionKey = attr->value();
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

                    auto barterIds = getBartererId(root, proNode);
                    if(barterIds.empty())
                    {
                        fmt::println("BARTER: NONE FOUND {}", skillIt->id);
                    }
                    for(auto barterId : barterIds)
                    {
                        Barter barter{barterId};
                        auto npcIt = ranges::find(info.npcs, barterId, &NPC::id);
                        if(npcIt == info.npcs.end())
                        {
                            info.npcs.push_back({barterId});
                        }
                        barter.currency.push_back(token);
                        acquireIt->barters.push_back(barter);
                    }

                    auto tokenIt = ranges::find(info.currencies, token.id, &Currency::id);
                    if(tokenIt == info.currencies.end())
                    {
                        info.currencies.push_back({token.id});
                    }

                    if(!factionKey.empty())
                    {
                        uint32_t factionId = 0;
                        uint32_t factionRank = 0;
                        unsigned i = 0;
                        string_view words{factionKey};
                        for(const auto word : std::ranges::split_view(words, ";"sv))
                        {
                            switch(i)
                            {
                            case 0: factionId = atoi(word.data()); break;
                            case 1: factionRank = atoi(word.data()); break;
                            default: break;
                            }
                            ++i;
                        }
                        if(factionId)
                        {
                            if(skillIt->factionId)
                            {
                                if(skillIt->factionId != factionId)
                                {
                                    fmt::println("BARTER: FACTION ALREADY SET {}({}): {} : {}",
                                                 skillIt->name[EN], skillIt->id, skillIt->factionId,
                                                 factionId);
                                }
                            }
                            else
                            {
                                skillIt->factionId = factionId;
                                skillIt->factionRank = factionRank;
                            }
                        }
                    }
                }
            }
        }
    }
    return true;
}

// -- lotro-data/lore/NPCs.xml
// <NPC id="1879450946" name="Quartermaster" gender="MALE"
//      title="key:621066979:229802005"/>
// -- lotro-data/lore/labels/en/npc.xml
// name: <label key="1879450946" value="Quartermaster"/>
// title: <label key="key:621066979:229802005" value="Dúnedain of Cardolan"/>
bool SkillLoader::getNPCTitleKeys(TravelInfo &info)
{
    string fp = fmt::format("{}\\lotro-data\\lore\\NPCs.xml", m_path);
    if(!m_xml.load(fp))
        return false;

    xml_node<> *root = m_xml.doc().first_node("NPCs");
    if(!root)
        return false;
    for(xml_node<> *node = root->first_node("NPC");
            node; node = node->next_sibling("NPC"))
    {
        xml_attribute<> *attr = node->first_attribute("id");
        if(!attr)
            return false;
        uint32_t npcId = atoi(attr->value());
        if(!npcId)
            return false;

        auto it = ranges::find(info.npcs, npcId, &NPC::id);
        if(it != info.npcs.end())
        {
            attr = node->first_attribute("title");
            if(attr)
            {
                it->titleKey = attr->value();
            }
        }
    }
    return true;
}

bool SkillLoader::getNPCLabels(TravelInfo &info)
{
    if(!getNPCLabel(EN, info))
        return false;
    if(!getNPCLabel(DE, info))
        return false;
    if(!getNPCLabel(FR, info))
        return false;
    if(!getNPCLabel(RU, info))
        return false;
    return true;
}

bool SkillLoader::getNPCLabel(const std::string &locale, TravelInfo &info)
{
    string fp = fmt::format("{}\\lotro-data\\lore\\labels\\{}\\npc.xml", m_path, locale);
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
        if(key.starts_with("key"))
        {
            auto it = ranges::find(info.npcs, key, &NPC::titleKey);
            if(it != info.npcs.end())
            {
                attr = node->first_attribute("value");
                if(attr)
                {
                    it->title[locale] = fixXmlStr(attr->value());
                }
            }
        }
        else
        {
            uint32_t npcId = atoi(attr->value());
            auto it = ranges::find(info.npcs, npcId, &NPC::id);
            if(it != info.npcs.end())
            {
                attr = node->first_attribute("value");
                if(attr)
                {
                    it->name[locale] = fixXmlStr(attr->value());
                }
            }
        }
    }
    return true;
}

static Barter *getVendorInfo(string_view sellListId, xml_node<> *root, Acquire &acquire)
{
    for(xml_node<> *node = root->first_node("vendor");
            node; node = node->next_sibling("vendor"))
    {
        for(xml_node<> *sellNode = node->first_node("sellList");
                sellNode; sellNode = sellNode->next_sibling("sellList"))
        {
            xml_attribute<> *attr = sellNode->first_attribute("sellListId");
            if(!attr)
                continue;
            string_view sellId = attr->value();
            if(sellListId == sellId)
            {
                attr = node->first_attribute("id");
                if(!attr)
                    continue;
                uint32_t vendorId = atoi(attr->value());
                attr = node->first_attribute("sellFactor");
                if(!attr)
                    continue;
                acquire.barters.push_back({vendorId, atof(attr->value())});
                return &acquire.barters.back();
            }
        }
    }
    return nullptr;
}

uint32_t SkillLoader::getValueTableValue(const Acquire &item)
{
    XMLLoader xml;
    string fp = fmt::format("{}\\lotro-data\\lore\\valueTables.xml", m_path);
    if(!xml.load(fp))
        return false;

    xml_node<> *root = xml.doc().first_node("valueTables");
    if(!root)
        return false;
    for(xml_node<> *node = root->first_node("valueTable");
            node; node = node->next_sibling("valueTable"))
    {
        xml_attribute<> *attr = node->first_attribute("id");
        if(!attr)
            continue;
        uint32_t id = atoi(attr->value());
        if(id != item.valueTableId)
            continue;
        double factor = 0;
        for(xml_node<> *quality = node->first_node("quality");
                quality; quality = quality->next_sibling("quality"))
        {
            attr = quality->first_attribute("key");
            if(!attr)
                continue;
            string_view key = attr->value();
            if(key == item.quality)
            {
                attr = quality->first_attribute("factor");
                if(attr)
                    factor = atof(attr->value());
                break;
            }
        }
        for(xml_node<> *base = node->first_node("baseValue");
                base; base = base->next_sibling("baseValue"))
        {
            attr = base->first_attribute("level");
            if(!attr)
                continue;
            uint32_t level = atoi(attr->value());
            if(level == item.level)
            {
                attr = base->first_attribute("value");
                if(attr)
                    return factor * atof(attr->value());
                return 0;
            }
        }
    }
    return 0;
}

bool SkillLoader::getVendors(TravelInfo &info)
{
    string fp = fmt::format("{}\\lotro-data\\lore\\vendors.xml", m_path);
    if(!m_xml.load(fp))
        return false;

    xml_node<> *root = m_xml.doc().first_node("vendors");
    if(!root)
        return false;
    for(xml_node<> *node = root->first_node("sellList");
            node; node = node->next_sibling("sellList"))
    {
        for(xml_node<> *sellNode = node->first_node("sellEntry");
                sellNode; sellNode = sellNode->next_sibling("sellEntry"))
        {
            xml_attribute<> *attr = sellNode->first_attribute("id");
            if(!attr)
                continue;
            uint32_t itemId = atoi(attr->value());
            for(auto &skill : info.skills)
            {
                for(auto &item : skill.acquire)
                {
                    if(item.itemId == itemId)
                    {
                        attr = node->first_attribute("sellListId");
                        if(!attr)
                            return false;
                        Barter *vendor = getVendorInfo(attr->value(), root, item);
                        if(!vendor)
                            return false;

                        vendor->buyAmt = getValueTableValue(item);
                        uint32_t bartererId = vendor->bartererId;
                        auto npcIt = ranges::find(info.npcs, bartererId, &NPC::id);
                        if(npcIt == info.npcs.end())
                        {
                            info.npcs.push_back({bartererId});
                        }
                    }
                }
            }
        }
    }
    return true;
}

bool SkillLoader::getQuests(std::vector<Skill> &skills)
{
    string fp = fmt::format("{}\\lotro-data\\lore\\quests.xml", m_path);
    if(!m_xml.load(fp))
        return false;

    xml_node<> *root = m_xml.doc().first_node("quests");
    if(!root)
        return false;
    for(xml_node<> *node = root->first_node("quest");
            node; node = node->next_sibling("quest"))
    {
        for(xml_node<> *rewardNode = node->first_node("rewards");
                rewardNode; rewardNode = rewardNode->next_sibling("rewards"))
        {
            for(xml_node<> *objNode = rewardNode->first_node("object");
                    objNode; objNode = objNode->next_sibling("object"))
            {
                xml_attribute<> *attr = objNode->first_attribute("id");
                if(!attr)
                    continue;
                bool found = false;
                uint32_t itemId = atoi(attr->value());
                for(auto &skill : skills)
                {
                    auto it = ranges::find(skill.acquire, itemId, &Acquire::itemId);
                    if(it == skill.acquire.end())
                        continue;

                    if(attr = node->first_attribute("id"); attr)
                        it->questId = atoi(attr->value());
                    attr = node->first_attribute("rawName");
                    if(!attr)
                        return false;
                    it->questNameKey = attr->value();
                    found = true;
                    break;
                }
                if(!found)
                {
                    //<object id="1879088537" name="Tattered Map to Glân Vraig"/>
                    if(itemId == 1879088537)
                    {
                        auto it = ranges::find(skills, 0x7005B38E, &Skill::id);
                        if(it != skills.end())
                        {
                            it->acquire.push_back(Acquire{itemId});
                            auto &acquire = it->acquire.back();
                            if(attr = node->first_attribute("id"); attr)
                                acquire.questId = atoi(attr->value());
                            if(attr = node->first_attribute("rawName"); attr)
                                acquire.questNameKey = attr->value();
                        }
                    }
                }
            }
        }
    }

    getQuestLabels(skills);
    return true;
}

bool SkillLoader::getQuestLabels(std::vector<Skill> &skills)
{
    if(!getQuestLabel(EN, skills))
        return false;
    if(!getQuestLabel(DE, skills))
        return false;
    if(!getQuestLabel(FR, skills))
        return false;
    if(!getQuestLabel(RU, skills))
        return false;
    return true;
}

bool SkillLoader::getQuestLabel(const string &locale, std::vector<Skill> &skills)
{
    string fp = fmt::format("{}\\lotro-data\\lore\\labels\\{}\\quests.xml", m_path, locale);
    if(!m_xml.load(fp))
        return false;

    xml_node<> *root = m_xml.doc().first_node("labels");
    if(!root)
        return false;

    std::unordered_map<std::string_view, Acquire*> acquireInfo;
    for(auto &skill : skills)
    {
        for(auto &acquire : skill.acquire)
        {
            acquireInfo.insert({acquire.questNameKey, &acquire});
        }
    }
    for(xml_node<> *node = root->first_node("label");
            node; node = node->next_sibling("label"))
    {
        xml_attribute<> *attr = node->first_attribute("key");
        if(!attr)
            continue;

        string_view key = attr->value();
        if(!key.starts_with("key"))
            continue;
        auto it = acquireInfo.find(key);
        if(it == acquireInfo.end())
            continue;

        attr = node->first_attribute("value");
        if(!attr)
            return false;

        it->second->questName[locale] = attr->value();
    }
    return true;
}

bool SkillLoader::getAllegianceLabel(const string &locale,
                                     std::vector<Skill> &skills)
{
    XMLLoader xml;
    string fp = fmt::format("{}\\lotro-data\\lore\\labels\\{}\\allegiances.xml", m_path, locale);
    if(!xml.load(fp))
        return false;

    xml_node<> *root = xml.doc().first_node("labels");
    if(!root)
        return false;
    for(xml_node<> *node = root->first_node("label");
            node; node = node->next_sibling("label"))
    {
        xml_attribute<> *attr = node->first_attribute("key");
        if(!attr)
            return false;
        string_view key = attr->value();
        if(key.starts_with("key"))
            continue;
        uint32_t allegianceId = atoi(key.data());
        for(auto &skill : skills)
        {
            if(!skill.allegiance)
                continue;
            if(skill.allegiance->id != allegianceId)
                continue;
            attr = node->first_attribute("value");
            if(!attr)
                return false;
            skill.allegiance->name[locale] = attr->value();
        }
    }
    return true;
}

bool SkillLoader::getAllegianceLabels(std::vector<Skill> &skills)
{
    if(!getAllegianceLabel(EN, skills))
        return false;
    if(!getAllegianceLabel(DE, skills))
        return false;
    if(!getAllegianceLabel(FR, skills))
        return false;
    if(!getAllegianceLabel(RU, skills))
        return false;
    return true;
}

bool SkillLoader::getAllegiance(std::vector<Skill> &skills)
{
    XMLLoader xml;
    string fp = fmt::format("{}\\lotro-data\\lore\\allegiances.xml", m_path);
    if(!xml.load(fp))
        return false;

    xml_node<> *root = xml.doc().first_node("allegiances");
    if(!root)
        return false;
    for(xml_node<> *node = root->first_node("allegiance");
            node; node = node->next_sibling("allegiance"))
    {
        xml_attribute<> *attr = node->first_attribute("travelSkillId");
        if(!attr)
            return false;
        uint32_t skillId = atoi(attr->value());
        auto it = ranges::find(skills, skillId, &Skill::id);
        if(it == skills.end())
        {
            fmt::println("MISSING ALLEGIANCE SKILL {}", skillId);
            continue;
        }

        attr = node->first_attribute("id");
        if(!attr)
            return false;
        uint32_t allegianceId = atoi(attr->value());
        it->allegiance = Allegiance{allegianceId};

        if(attr = node->first_attribute("minLevel"); attr)
        {
            it->minLevel = atoi(attr->value());
        }
        if(it->acquireDeed)
        {
            auto &deed = it->acquireDeed->name.at(EN);
            const std::regex attr(".*Allegiance Level ([0-9]+)");
            std::smatch match;
            if(std::regex_match(deed, match, attr))
            {
                string number = match[1].str();
                it->allegiance->rank = atoi(number.c_str());
            }
        }
    }
    return true;
}

static Skill *getTraitDeed(const unordered_map<string_view, Skill*> &traits,
                           xml_node<> *node)
{
    for(xml_node<> *traitNode = node->first_node("trait");
            traitNode; traitNode = traitNode->next_sibling("trait"))
    {
        xml_attribute<> *attr = traitNode->first_attribute("id");
        if(!attr)
            return nullptr;
        auto it = traits.find(attr->value());
        if(it != traits.end())
            return it->second;
    }
    return nullptr;
}

static Skill *getItemDeed(const unordered_map<uint32_t, Skill*> &items,
                          xml_node<> *node)
{
    for(xml_node<> *itemNode = node->first_node("object");
            itemNode; itemNode = itemNode->next_sibling("object"))
    {
        xml_attribute<> *attr = itemNode->first_attribute("id");
        if(!attr)
            return nullptr;
        auto it = items.find(atoi(attr->value()));
        if(it != items.end())
            return it->second;
    }
    return nullptr;
}

bool SkillLoader::getDeeds(const unordered_map<string_view, Skill*> &traits,
                           const unordered_map<uint32_t, Skill*> &skills)
{
    XMLLoader xml;
    string fp = fmt::format("{}\\lotro-data\\lore\\deeds.xml", m_path);
    if(!xml.load(fp))
        return false;

    xml_node<> *root = xml.doc().first_node("deeds");
    if(!root)
        return false;
    for(xml_node<> *node = root->first_node("deed");
            node; node = node->next_sibling("deed"))
    {
        for(xml_node<> *rewardNode = node->first_node("rewards");
                rewardNode; rewardNode = rewardNode->next_sibling("rewards"))
        {
            auto *skill = getTraitDeed(traits, rewardNode);
            if(!skill)
                skill = getItemDeed(skills, rewardNode);
            if(skill)
            {
                if(skill->acquireDeed)
                {
                    fmt::println("ALREADY HAS A DEED {}", skill->id);
                }
                xml_attribute<> *attr = node->first_attribute("id");
                if(attr)
                {
                    uint32_t deedId = atoi(attr->value());
                    skill->acquireDeed = Deed{deedId};
                    attr = node->first_attribute("minLevel");
                    if(attr)
                    {
                        unsigned minLevel = skill->minLevel = atoi(attr->value());
                        if(skill->minLevel)
                        {
                            if(skill->minLevel != minLevel)
                            {
                                fmt::println("DEED: minLevel already set!!!");
                            }
                        }
                        else
                        {
                            skill->minLevel = minLevel;
                        }
                    }
                    break;
                }
            }
            break;
        }
    }
    return true;
}

bool SkillLoader::getTraits(std::vector<Skill> &skills)
{
    string fp = fmt::format("{}\\lotro-data\\lore\\traits.xml", m_path);
    if(!m_xml.load(fp))
        return false;

    unordered_map<uint32_t, Skill*> skillHash;
    for(auto &skill : skills)
    {
        skillHash.insert({skill.id, &skill});
    }
    unordered_map<uint32_t, Skill*> items;
    for(auto &skill : skills)
    {
        for(auto &acquire : skill.acquire)
        {
            items.insert({acquire.itemId, &skill});
        }
    }
    unordered_map<string_view, Skill*> traits;
    xml_node<> *root = m_xml.doc().first_node("traits");
    if(!root)
        return false;
    for(xml_node<> *node = root->first_node("trait");
            node; node = node->next_sibling("trait"))
    {
        for(xml_node<> *skillNode = node->first_node("skill");
                skillNode; skillNode = skillNode->next_sibling("skill"))
        {
            xml_attribute<> *attr = skillNode->first_attribute("id");
            if(!attr)
                break;
            uint32_t skillId = atoi(attr->value());
            auto it = skillHash.find(skillId);
            if(it != skillHash.end())
            {
                attr = node->first_attribute("identifier");
                if(attr)
                    traits.insert({attr->value(), it->second});
            }
            break;
        }
    }
    getDeeds(traits, items);
    getDeedLabels(skills);
    getAllegiance(skills);
    getAllegianceLabels(skills);
    return true;
}

bool SkillLoader::getDeedLabels(std::vector<Skill> &skills)
{
    if(!getDeedLabel(EN, skills))
        return false;
    if(!getDeedLabel(DE, skills))
        return false;
    if(!getDeedLabel(FR, skills))
        return false;
    if(!getDeedLabel(RU, skills))
        return false;
    return true;
}

bool SkillLoader::getDeedLabel(const string &locale, std::vector<Skill> &skills)
{
    string fp = fmt::format("{}\\lotro-data\\lore\\labels\\{}\\deeds.xml", m_path, locale);
    if(!m_xml.load(fp))
        return false;

    xml_node<> *root = m_xml.doc().first_node("labels");
    if(!root)
        return false;

    std::unordered_map<uint32_t, Deed*> deedInfo;
    for(auto &skill : skills)
    {
        auto &deed = skill.acquireDeed;
        if(!deed)
            continue;
        deedInfo.insert({deed->id, &deed.value()});
    }
    for(xml_node<> *node = root->first_node("label");
            node; node = node->next_sibling("label"))
    {
        xml_attribute<> *attr = node->first_attribute("key");
        if(!attr)
            continue;

        string_view key = attr->value();
        if(key.starts_with("key"))
            continue;
        auto it = deedInfo.find(atoi(key.data()));
        if(it == deedInfo.end())
            continue;

        attr = node->first_attribute("value");
        if(!attr)
            return false;

        it->second->name[locale] = attr->value();
    }
    return true;
}
