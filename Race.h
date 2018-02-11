/* Interface to unify management of all three races */

#pragma once
#include <BWAPI.h>
#include "UnitTrainer.h"
#include "BuildingConstructor.h"
#include "ResourceSupplier.h"
#include "TechTree.h"

class Race {
    protected:
        ResourceSupplier* resourceSupplier;
        BuildingConstructor* buildingConstructor;
        UnitTrainer* unitTrainer;
        TechTree* techTree;
        BWAPI::UnitType centerType, workerType, supplyType, armyUnitType;
        void onCompleteBuilding(const BWAPI::Unit&) const;
        void onDestroyedBuilding(const BWAPI::Unit&) const;
        virtual int expectedSupplyProvided(const BWAPI::UnitType&) const;
    public:
        Race(const BWAPI::UnitType& armyUnitType);
        ~Race();
        BWAPI::UnitType getCenterType() const { return centerType; }
        BWAPI::UnitType getWorkerType() const { return workerType; }
        BWAPI::UnitType getSupplyType() const { return supplyType; }
        BWAPI::UnitType getArmyUnitType() const { return armyUnitType; }
        virtual void onUnitCreate(const BWAPI::Unit& createdUnit) const;
        virtual void onUnitMorph(const BWAPI::Unit& morphedUnit);
        virtual void onUnitComplete(const BWAPI::Unit& completedUnit);
        virtual void onUnitDestroy(const BWAPI::Unit& destroyedUnit) const;
        void update() const;
        int expectedSupplyProvided() const;
        int potentialSupplyUsed(const BWAPI::UnitType& unitType) const;
        virtual void createSupply() const;
        bool canFillLackingMiners() const;
        void createWorker() const;
        bool readyToTrainArmyUnit() const;
        void trainWarriors() const;
        virtual void construct(const BWAPI::UnitType& buildingType) const;
        virtual BWAPI::UnitType getNextRequiredBuilding(
            const BWAPI::UnitType&) const;
};

