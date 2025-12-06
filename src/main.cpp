#include <locale>
#include <fmt/format.h>
#include "skill_loader.h"
#include "skill_input.h"
#include "skill_output.h"

using namespace std;

int main(int argc, const char **argv)
{
#if WIN32
    // force UTF8 as the default multi-byte locale and not ANSI
    locale::global(locale(locale{}, locale(".utf8"), locale::ctype));
#endif

    TravelInfo info;
    SkillLoader loader("C:\\projects"); // TODO: make an input arg
    info.skills = loader.getSkills();
    if(!loadSkillInputs(info))
    {
        return -1;
    }
    printNewSkills(info);
    if(!mergeSkillInputs(info, info.inputs))
    {
        return -1;
    }
    if(!loader.getCurrencies(info))
    {
        return -1;
    }
    if(!loader.getFactions(info))
    {
        return -1;
    }

    outputSkillDataFile(info);
    outputLocaleDataFile(info);
    return 0;
}
