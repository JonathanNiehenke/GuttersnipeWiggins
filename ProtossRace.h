#pragma once
#include "Race.h"

class ProtossRace : public Race
{
    private:
        void onDestroyedBuilding(const BWAPI::Unit& destroyedBuilding);
    public:
        ProtossRace() : Race(
            BWAPI::UnitTypes::Enum::Protoss_Nexus,
            BWAPI::UnitTypes::Enum::Protoss_Probe,
            BWAPI::UnitTypes::Enum::Protoss_Pylon,
            BWAPI::UnitTypes::Enum::Protoss_Zealot) {}
        void ProtossRace::onUnitCreate(const BWAPI::Unit& createdUnit)
        void ProtossRace::construct(const BWAPI::UnitType& buildingType)
};

