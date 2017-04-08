#ifndef RACE_H
#define RACE_H
#include <BWAPI.h>
#include "BuildingConstructer.h"
#include "Cartographer.h"
#include "CmdRescuer.h"
#include "EcoBaseManager.h"
#include "SquadCommander.h"
#include "UnitTrainer.h"

// An idea to increase performance and readabliliy by using
// polymorphism. This would eliminate race checks, clearly join
// their similarities and separate their differences. 

// To reduce the parameters list of Race and derived constructors.
struct Core
{
    BuildingConstructer *buildingConstructer;
    Cartographer *cartographer;
    CmdRescuer::Rescuer *cmdRescuer;
    EcoBaseManager *ecoBaseManager;
    SquadCommander *squadCommander;
    UnitTrainer *unitTrainer;
    Core(BuildingConstructer*, Cartographer*, CmdRescuer::Rescuer*,
         EcoBaseManager*, SquadCommander*, UnitTrainer*);
};


class Race
{
    private:
    public:
        BWAPI::UnitType
            centerType, workerType, supplyType, armyTechType, armyUnitType;
        BuildingConstructer *buildingConstructer;
        Cartographer *cartographer;
        CmdRescuer::Rescuer *cmdRescuer;
        EcoBaseManager *ecoBaseManager;
        SquadCommander *squadCommander;
        UnitTrainer *unitTrainer;
        Race(BWAPI::UnitType, BWAPI::UnitType, BWAPI::UnitType,
             BWAPI::UnitType, BWAPI::UnitType, Core&);
        virtual void onUnitCreate(BWAPI::Unit unit);
        virtual void onUnitMorph(BWAPI::Unit unit) {};
        virtual void onCenterComplete(BWAPI::Unit Unit);
        virtual void onUnitComplete(BWAPI::Unit unit) {}
        virtual void onUnitDestroy(BWAPI::Unit unit) {}
        virtual void createWorkers()
            { ecoBaseManager->produceUnits(workerType); }
        virtual void createWarriors()
            { unitTrainer->produceUnits(armyUnitType); }
        virtual void createSupply()
            { buildingConstructer->constructUnit(supplyType); }
        virtual void createFacility()
            { buildingConstructer->constructUnit(armyTechType); }
        virtual void createCenter()
            { buildingConstructer->constructExpansion(centerType); }
};

#endif
