#pragma once
#include <BWAPI.h>

class UnitTrainer
{
    private:
        std::vector<BWAPI::Unit> productionFacilities;
        bool isTrainingQueueEmpty(const BWAPI::Unit& Facility) const;
    public:
        void includeFacility(const BWAPI::Unit& Facility);
        void removeFacility(const BWAPI::Unit& Facility);
        bool readyToTrain(const BWAPI::UnitType& unitType) const;
        void trainUnits(const BWAPI::UnitType& unitType);
};
