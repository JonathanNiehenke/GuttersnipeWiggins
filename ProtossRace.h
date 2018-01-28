#pragma once
#include "Race.h"

class ProtossRace : public Race
{
    private:
        void onDestroyedBuilding(const BWAPI::Unit& destroyedBuilding) const;
        bool doesPylonExist() const;
    public:
        ProtossRace() : Race(BWAPI::UnitTypes::Enum::Protoss_Zealot) {}
        void onUnitCreate(const BWAPI::Unit& createdUnit) const;
        void construct(const BWAPI::UnitType& buildingType) const;
        BWAPI::UnitType getNextRequiredBuilding(
            const BWAPI::UnitType& unitType) const;
};
