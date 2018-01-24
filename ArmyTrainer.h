#pragma once
#include <BWAPI.h>

class ArmyTrainer
{
    private:
        std::vector<BWAPI::Unit> productionFacilities;
        bool isTrainingQueueEmpty(BWAPI::Unit Facility);
    public:
        void includeFacility(const BWAPI::Unit& Facility) const;
        void removeFacility(const BWAPI::Unit& Facility) const;
        int facilityCount() const;
        void trainUnits(BWAPI::UnitType unitType);
};
