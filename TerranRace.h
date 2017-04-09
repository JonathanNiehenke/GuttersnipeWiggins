#ifndef TERRANRACE_H
#define TERRANRACE_H
#include "Race.h"

class TerranRace : public Race
{
    private:
    public:
        TerranRace(Core &core) : Race(
            BWAPI::UnitTypes::Enum::Terran_Command_Center,
            BWAPI::UnitTypes::Enum::Terran_SCV,
            BWAPI::UnitTypes::Enum::Terran_Supply_Depot,
            BWAPI::UnitTypes::Enum::Terran_Barracks,
            BWAPI::UnitTypes::Enum::Terran_Marine,
            core) {}
        void onUnitCreate(BWAPI::Unit Unit);
        void onUnitComplete(BWAPI::Unit Unit);
        void onUnitDestroy(BWAPI::Unit Unit);
};

#endif

