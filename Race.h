#ifndef RACE_H
#define RACE_H
#include <BWAPI.h>
#include "BuildingConstructer.h"
#include "Cartographer.h"
#include "CmdRescuer.h"
#include "EcoBaseManager.h"
#include "SquadCommander.h"
#include "UnitTrainer.h"

// An idea to increase performance and readability by using
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
        bool readyToExpand();
    public:
        BWAPI::UnitType
            centerType, workerType, supplyType, armyTechType, armyUnitType;
        BuildingConstructer *buildingConstructer;
        Cartographer *cartographer;
        CmdRescuer::Rescuer *cmdRescuer;
        EcoBaseManager *ecoBaseManager;
        SquadCommander *squadCommander;
        UnitTrainer *unitTrainer;
        BWAPI::Player Self;
        Race(BWAPI::UnitType, BWAPI::UnitType, BWAPI::UnitType,
             BWAPI::UnitType, BWAPI::UnitType, Core&);
        virtual void onUnitCreate(BWAPI::Unit unit) {}
        virtual void onUnitMorph(BWAPI::Unit unit) {}
        virtual void onCenterComplete(BWAPI::Unit Unit);
        virtual void onUnitComplete(BWAPI::Unit unit) {}
        virtual void onUnitDestroy(BWAPI::Unit unit) {}
        void createWorkers()
            { ecoBaseManager->produceUnits(workerType); }
        void createWarriors()
            { unitTrainer->produceUnits(armyUnitType); }
        virtual void createSupply()  // Overrides to train Overlord
            { buildingConstructer->constructUnit(supplyType); }
        virtual void createFacility()  // Overrides to morph Hatcheries
            { buildingConstructer->constructUnit(armyTechType); }
        void createCenter()
            { buildingConstructer->constructExpansion(centerType); }
        virtual int getAvailableSupply();  // Overrides overlord count
        int getUnitBuffer(BWAPI::UnitType unitType);
        virtual bool needsSupply();  // Zerg overrides army buffer calc
        virtual bool readyForArmyTech();  // Overrides for pylon
        void manageProduction();
        void manageStructures();
        void manageAttackGroups();
        void onCompleteWorkaround(BWAPI::Unit workerUnit);
        void scout(std::set<BWAPI::TilePosition> scoutLocations);
        virtual void displayStatus();  // Zerg overrides overlord count
};

#endif
