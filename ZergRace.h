#pragma once
#include "Race.h"

class ZergRace : public Race
{
    private:
        int incompleteOverlordCount = 0;
        static bool isIncompleteOverlord(const BWAPI::Unit& unit);
        virtual int expectedSupplyProvided(const BWAPI::UnitType&) const;
        bool doesTechExist(const BWAPI::UnitType& buildingType) const;
    public:
        ZergRace() : Race(BWAPI::UnitTypes::Enum::Zerg_Zergling) {}
        void onUnitMorph(const BWAPI::Unit& morphedUnit);
        void onUnitComplete(const BWAPI::Unit& completedUnit);
        void createSupply() const;
        void construct(const BWAPI::UnitType& buildingType) const;
};
