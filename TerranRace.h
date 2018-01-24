#pragma once
#include "Race.h"


class TerranRace : public Race
{
    private:
        void onDestroyedBuilding(const BWAPI::Unit& destroyedBuilding);
    public:
        TerranRace() : Race(
            BWAPI::UnitTypes::Enum::Terran_Command_Center,
            BWAPI::UnitTypes::Enum::Terran_SCV,
            BWAPI::UnitTypes::Enum::Terran_Supply_Depot,
            BWAPI::UnitTypes::Enum::Terran_Marine) {}
        void ProtossRace::onUnitCreate(const BWAPI::Unit& createdUnit);
};

