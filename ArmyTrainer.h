#pragma once
#include <BWAPI.h>

class ArmyTrainer
{
    private:
        std::vector<BWAPI::Unit> productionFacilities;
        bool isTrainingQueueEmpty(BWAPI::Unit Facility) const;
    public:
        void includeFacility(const BWAPI::Unit& Facility);
        void removeFacility(const BWAPI::Unit& Facility);
        int facilityCount() const;
        void trainUnits(const BWAPI::UnitType& unitType);
};
