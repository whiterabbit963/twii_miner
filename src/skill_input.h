#ifndef SKILL_INPUT_H
#define SKILL_INPUT_H

#include "skill_loader.h"

class TravelInfo;
bool loadSkillInputs(TravelInfo &info);
bool mergeSkillInputs(TravelInfo &info,
                      std::map<unsigned, Skill> &skillInputs);

#endif // SKILL_INPUT_H
