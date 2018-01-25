#pragma once
#include "Race.h"


class TerranRace : public Race
{
    private:
        void onDestroyedBuilding(const BWAPI::Unit& destroyedBuilding);
    public:
        TerranRace() : Race(BWAPI::UnitTypes::Enum::Terran_Marine) {}
        void onUnitCreate(const BWAPI::Unit& createdUnit);
};
