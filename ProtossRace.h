#ifndef PROTOSSRACE_H
#define PROTOSSRACE_H
#include "Race.h"

class ProtossRace : public Race
{
    private:
        bool readyForArmyTech();
    public:
        ProtossRace(Core &core) : Race(
            BWAPI::UnitTypes::Enum::Protoss_Nexus,
            BWAPI::UnitTypes::Enum::Protoss_Probe,
            BWAPI::UnitTypes::Enum::Protoss_Pylon,
            BWAPI::UnitTypes::Enum::Protoss_Gateway,
            BWAPI::UnitTypes::Enum::Protoss_Zealot,
            core) {}
        void onUnitCreate(BWAPI::Unit Unit);
        void onUnitComplete(BWAPI::Unit Unit);
        void onUnitDestroy(BWAPI::Unit Unit);
};

#endif
