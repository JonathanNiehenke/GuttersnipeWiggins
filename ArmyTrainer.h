#pragma once
#include <BWAPI.h>

class ArmyTrainer
{
    private:
        std::vector<BWAPI::Unit> productionFacilities;
        bool isTrainingQueueEmpty(const BWAPI::Unit& Facility) const;
    public:
        void includeFacility(const BWAPI::Unit& Facility);
        void removeFacility(const BWAPI::Unit& Facility);
        int facilityCount() const;
        bool readyToTrain(const BWAPI::UnitType& unitType) const;
        void trainUnits(const BWAPI::UnitType& unitType);
};
