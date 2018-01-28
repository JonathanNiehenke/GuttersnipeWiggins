#pragma once
#include "Race.h"

class ZergRace : public Race
{
    private:
        void includeLarvaProducers() const;
        void removeLarvaProducers() const;
        void onDestroyedBuilding(const BWAPI::Unit&) const;
        bool doesTechExist(const BWAPI::UnitType& buildingType) const;
    public:
        ZergRace() : Race(BWAPI::UnitTypes::Enum::Zerg_Zergling) {}
        void onUnitCreate(const BWAPI::Unit& createdUnit) const;
        void onUnitMorph(const BWAPI::Unit& morphedUnit) const;
        void createSupply() const;
        void construct(const BWAPI::UnitType& buildingType) const;
};
