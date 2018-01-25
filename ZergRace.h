#pragma once
#include "Race.h"

class ZergRace : public Race
{
    private:
        void includeLarvaProducers();
        void removeLarvaProducers();
        void onDestroyedBuilding(const BWAPI::Unit& destroyedBuilding);
        bool doesTechExist(const BWAPI::UnitType& buildingType) const;
    public:
        ZergRace() : Race(BWAPI::UnitTypes::Enum::Zerg_Zergling) {}
        void onUnitCreate(const BWAPI::Unit& createdUnit);
        void onMorph(const BWAPI::Unit& morphedUnit);
        void createSupply();
        void construct(const BWAPI::UnitType& buildingType);
};


