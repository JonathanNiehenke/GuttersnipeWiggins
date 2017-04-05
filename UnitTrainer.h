#ifndef UNITTRAINER_H
#define UNITTRAINER_H
#include <BWAPI.h>
#include "CmdRescuer.h"

class UnitTrainer
{
    private:
        std::vector<BWAPI::Unit> productionFacilities;
        BWAPI::UnitType unitType;
        CmdRescuer::Rescuer *cmdRescuer;
        bool isIdle(BWAPI::Unit Facility);
    public:
        void onStart(BWAPI::UnitType, CmdRescuer::Rescuer*);
        void includeFacility(BWAPI::Unit Facility)
            {productionFacilities.push_back(Facility); }
        void removeFacility(BWAPI::Unit Facility);
        bool isAvailable()
            { return !productionFacilities.empty(); }
        int facilityCount()
            { return productionFacilities.size(); }
        bool canProduce();
        void produceSingleUnit(BWAPI::UnitType unitType);
        void produceUnits(BWAPI::UnitType unitType);
        void displayStatus(int &row);
};

#endif
