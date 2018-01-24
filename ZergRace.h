#pragma once
#include "Race.h"

class ZergRace : public Race
{
    private:
        void includeLarvaProducers();
        void removeLarvaProducers();
        bool doesTechExist(const BWAPI::UnitType& buildingType);
        void onDestroyedBuilding(const BWAPI::Unit& destroyedBuilding);
    public:
        void onUnitCreate(const BWAPI::Unit& createdUnit);
        void onMorph(const BWAPI::Unit& morphedUnit);
        void createSupply();
        void construct(const BWAPI::UnitType& buildingType);
};


