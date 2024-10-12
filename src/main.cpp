#include <locale>
#include <fmt/format.h>
#include "skill_loader.h"

using namespace std;

// TODO: move to a toml config file
std::vector<uint32_t> skillIds{
    // hunter
    0x7000A2C1,
    0x70003F42,
    0x70003F41,
    0x7000A2C3,
    0x70003F43,
    0x7000A2C4,
    0x7000A2C2,
    0x70003F44,
    0x70017C82,
    0x7000A2C5,
    0x7000A2C6,
    0x70017C81,
    0x70017C7A,

    0x7001F459,
    0x700235EF,
    0x7002A93F,
    0x7002C62C,

    0x7002E754,
    0x7002E756,
    0x7003198E,
    0x70036B5D,
    0x7003DC71,
    0x7003DC72,
    0x70041197,
    0x70043A63,
    0x70044985,
    0x700459AF,
    0x70046CBB,
    0x70047077,
    0x70047074,
    0x70047BFA,
    0x70047C1D,
    0x7004AE1E,
    0x7004D73B,
    0x7004FACC,
    0x7004FACB,
    0x70052F07,
    0x70052F08,
    0x700551F4,
    0x7005762D,

    0x70058571,
    0x70059D0C,
    0x70059D16,
    0x7005AA91,
    0x7005AA95,
    0x7005D487,
    0x7005D47D,
    0x70060EA6,
    0x7006133F,
    0x7006323C,
    0x700634AA,
    0x700634A7,
    0x70064AC8,
    0x70064F4C,
    0x700658EA,
    0x70068711,

    0x70068713,
    0x70068717,

    0x70068718,
    0x70068719,
    0x700697EF,
    0x7006A9BF,


    //Warden" ,

    0x70014786,
    0x70014798,
    0x7001478E,
    0x70014791,
    0x700237D4,
    0x7001819E,

    0x7001F45C,
    0x700235EB,
    0x7002A90A,
    0x7002C646,

    0x700303DF,
    0x700303DD,
    0x7003198D,
    0x70036B5B,
    0x7003DC7A,
    0x7003DC79,
    0x70041198,
    0x70043A66,
    0x70044982,
    0x700459AA,
    0x70046CBF,
    0x70047075,
    0x70047076,
    0x70047BFC,
    0x70047C23,
    0x7004AE1F,
    0x7004D73A,
    0x7004FACA,
    0x7004FACD,
    0x70052F0A,
    0x70052F06,
    0x700551F2,
    0x70057635,

    0x70058572,
    0x70059D09,
    0x70059D10,
    0x7005AA8F,
    0x7005AA8C,
    0x7005D48A,
    0x7005D488,
    0x70060EA5,
    0x7006133E,
    0x70063242,
    0x700634B6,
    0x700634AD,
    0x70064ACB,
    0x70064F4D,
    0x700658E8,
    0x7006870C,
    0x7006870F,
    0x70068710,
    0x70068712,

    0x70068715,

    0x700697F3,
    0x7006A9C2,


    //Mariner",

    0x70066100,
    0x70066101,
    0x70066105,
    0x70066109,
    0x7006610C,
    0x7006610E,
    0x7006610F,
    0x70066117,
    0x7006611A,
    0x7006611B,
    0x7006611C,
    0x7006611E,
    0x70066120,
    0x70066121,
    0x700687BB,
    0x700687BD,

    0x700687C0,
    0x700687C1,

    0x700687C3,
    0x7006A9C4,


    //Reputati,

    0x7001BF91,
    0x7001BF90,
    0x700364B1,
    0x70023262,
    0x70023263,
    0x70020441,
    0x7001F374,
    0x70021FA2,
    0x7002C647,
    0x7002C65D,

    0x70031A46,
    0x70036B5E,
    0x7003DC81,
    0x7004128F,

    0x7003DC82,
    0x700411AC,
    0x70043A6A,
    0x7004497E,
    0x700459A9,
    0x70046CC0,
    0x70047080,
    0x7004707D,
    0x70047BF4,
    0x70047C1B,
    0x7004AE1D,
    0x7004B8C2,
    0x7004B8C3,
    0x7004B8C4,
    0x7004B8C5,
    0x7004D738,
    0x7004FAC3,
    0x7004FAC5,
    0x70052F12,
    0x70052F04,
    0x700551F8,
    0x70057629,

    0x7005856F,
    0x70059D0E,
    0x70059D12,
    0x7005AA90,
    0x7005AA92,
    0x7005A596,
    0x7005D47C,
    0x7005D484,
    0x70060EA8,
    0x70061340,
    0x7006323D,
    0x700634A4,
    0x700634AE,
    0x700634A5,
    0x70064ACA,
    0x70064F47,
    0x7005B38E,
    0x700658EB,
    0x700686FE,
    0x700686FF,
    0x70068700,

    0x70068701,
    0x70068702,
    0x70068703,

    0x70068704,
    0x700697F2,
    0x7006A9C1,

    // racial
    0x700062F6,
    0x700062C8,
    0x70006346,
    0x7000631F,
    0x70041A22,
    0x70048C8C,
    0x70053C0F,
    0x70066D31,

    // general
    0x700256BA,
    0x70025792,
    0x70025793,
    0x70025794,
    0x70025795,
    0x70025796,
    0x7002FF62,
    0x7002FF61,
    0x7002FF60,
    0x7002FF5F,
    0x7002FF63,
    0x7000D046,
    0x70046EE4,
    0x7000D047,
    0x70057C36,

    // creep
    0x70028BAF,
    0x70028BB0,
    0x70028BB1,
    0x70028BB2,
    0x70028BB3,
    0x70028BB4,
    0x70028BB5,
    0x70028BB6,
    0x70028BB7,
    0x70028BB9,
    0x70028BBC,
    0x70028BBD,
    0x70028BBE,
    0x70028BBF,
    0x70028BC0,
    0x70028BC1,
    0x70028BC2,
    0x7002A7B3,
    0x7002A7B4,
    0x7002A7B5,
    0x7002A7B6,
    0x7002A7B7,
    0x7002A7B8,
    0x7002A7B9,
    0x7002FF5E,
    0x7002FF64,
    0x7002FF66,
    0x7002FF67,
    0x7002FF68,
    0x70037B69,
    //0x70017A8D, // Desperate Flight
    0x70003F3D,
    //0x70003F40, // Desperate Flight
};

int main(int argc, const char **argv)
{
#if WIN32
    // force UTF8 as the default multi-byte locale and not ANSI
    locale::global(locale(locale{}, locale(".utf8"), locale::ctype));
#endif

    SkillLoader loader("C:\\projects\\lotro-data");
    vector<Skill> skills = loader.getSkills();
    loader.getSkillNames(skills);

    // verification
    fmt::println("Skills: {}:{}", skills.size(), skillIds.size());
    for(auto &skill : skills)
    {
        if(!std::any_of(skillIds.begin(), skillIds.end(), [&skill](auto &id) { return skill.id == id; }))
        {
            fmt::println("{}", skill.name[EN]);
        }
        if(skill.name.size() != 4)
            fmt::println("{}", skill.id);
    }
    for(auto &id : skillIds)
    {
        if(!std::any_of(skills.begin(), skills.end(), [&id](auto &s) { return s.id == id; }))
        {
            fmt::println("{}", id);
        }
    }
    fmt::println("\n\n");

    // print new skills
    unsigned next = 0;
    for(auto &skill : skills)
    {
        if(!std::any_of(skillIds.begin(), skillIds.end(), [&skill](auto &id) { return skill.id == id; }))
        {
            fmt::println("id=\"0x{:08X}\",", skill.id);
            fmt::println("EN={{ name=\"{}\", }},", skill.name[EN]);
            fmt::println("DE={{ name=\"{}\", }},", skill.name[DE]);
            fmt::println("FR={{ name=\"{}\", }},", skill.name[FR]);
            fmt::println("RU={{ name=\"{}\", }},", skill.name[RU]);
            fmt::println("map={{{{MapType.HARADWAITH, -1, -1}}}},");
            fmt::println("minLevel=150,");
            fmt::println("level=150.{}", ++next);
            fmt::println("");
        }
    }

    return 0;
}
