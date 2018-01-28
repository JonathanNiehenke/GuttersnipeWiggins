#pragma once
#include "Race.h"

class ProtossRace : public Race
{
    private:
        void onDestroyedBuilding(const BWAPI::Unit& destroyedBuilding);
        bool doesPylonExist() const;
    public:
        ProtossRace() : Race(BWAPI::UnitTypes::Enum::Protoss_Zealot) {}
        void ProtossRace::onUnitCreate(const BWAPI::Unit& createdUnit);
        void ProtossRace::construct(const BWAPI::UnitType& buildingType);
        BWAPI::UnitType getNextRequiredBuilding(
            const BWAPI::UnitType&) const;
};

