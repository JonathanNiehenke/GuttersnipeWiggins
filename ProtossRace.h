#pragma once
#include "Race.h"

class ProtossRace : public Race
{
    private:
        bool doesPylonExist() const;
    public:
        ProtossRace() : Race(BWAPI::UnitTypes::Enum::Protoss_Zealot) {}
        void construct(const BWAPI::UnitType& buildingType) const;
        BWAPI::UnitType getNextRequiredBuilding(
            const BWAPI::UnitType& unitType) const;
};
