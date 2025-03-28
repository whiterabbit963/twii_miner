#ifndef SKILL_OUTPUT_H
#define SKILL_OUTPUT_H

#include "skill_loader.h"

struct TravelInfo;
void outputSkillDataFile(const TravelInfo &info);
std::string_view getGroupName(Skill::Type type);
Skill::Type getSkillType(std::string_view name);

#endif // SKILL_OUTPUT_H
