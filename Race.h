/* Interface to unify management of all three races */

#pragma once
#include <BWAPI.h>
#include "ArmyTrainer.h"
#include "BuildingConstructor.h"
#include "ResourceSupplier.h"
#include "TechTree.h"

class Race {
    protected:
        ResourceSupplier* resourceSupplier;
        BuildingConstructor* buildingConstructor;
        ArmyTrainer* armyTrainer;
        TechTree* techTree;
        BWAPI::UnitType centerType, workerType, supplyType, armyUnitType;
        int expectedSupplyProvided(const BWAPI::UnitType& providerType) const;
        virtual void onDestroyedBuilding(const BWAPI::Unit&) {}
    public:
        Race(const BWAPI::UnitType& armyUnitType);
        ~Race();
        BWAPI::UnitType getCenterType() const { return centerType; }
        BWAPI::UnitType getWorkerType() const { return workerType; }
        BWAPI::UnitType getSupplyType() const { return supplyType; }
        BWAPI::UnitType getArmyUnitType() const { return armyUnitType; }
        virtual void onUnitCreate(const BWAPI::Unit& unit);
        virtual void onUnitMorph(const BWAPI::Unit& unit);
        virtual void onUnitComplete(const BWAPI::Unit& unit);
        virtual void onUnitDestroy(const BWAPI::Unit& unit);
        void update();
        int expectedSupplyProvided() const;
        int potentialSupplyUsed(const BWAPI::UnitType& unitType) const;
        virtual void createSupply();
        void createWorker();
        bool readyToTrainArmyUnit();
        void trainWarriors();
        virtual void construct(const BWAPI::UnitType& buildingType);
};

