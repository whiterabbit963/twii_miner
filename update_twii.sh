#!/bin/bash

if [[ ! -v PROJECTS_PATH ]]; then
    PROJECTS_PATH=/c/projects
fi

twii_path="$HOME/Documents/The Lord of the Rings Online/Plugins/TravelWindowII"

cd "${PROJECTS_PATH}/lotro-data"
git pull -r
cd "${PROJECTS_PATH}/lotro-items-db"
git pull -r
exe_path="${PROJECTS_PATH}/builds/twii_miner_Static_MSVC2019_64bit-Debug"
cd $exe_path
./twii_miner -path "${PROJECTS_PATH}"
cp "${exe_path}/skill_input.toml" "${twii_path}/data/"
cp "${exe_path}/LocaleData.lua" "${exe_path}/SkillData.lua" "${twii_path}/src/"

