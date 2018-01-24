/* Interface to unify management of all three races */

#pragma once
#include <BWAPI.h>
#include "ResourceSupplier.h"
#include "ArmyTrainer.h"
#include "BuildingConstructor.h"

class Race {
    private:
        ResourceSupplier* resourceSupplier;
        BuildingConstructor* buildingConstructor;
        ArmyTrainer* armyTrainer;
        BWAPI::UnitType centerType, workerType, supplyType, armyUnitType;
        int expectedSupplyProvided(const BWAPI::UnitType& providerType) const;
    public:
        Race(BWAPI::UnitType, BWAPI::UnitType, BWAPI::UnitType,
             BWAPI::UnitType);
        BWAPI::UnitType getCenterType() const { return centerType; }
        BWAPI::UnitType getWorkerType() const { return workerType; }
        BWAPI::UnitType getSupplyType() const { return supplyType; }
        BWAPI::UnitType getArmyUnitType() const { return armyUnitType; }
        virtual void onUnitCreate(const BWAPI::Unit& unit);
        virtual void onUnitMorph(const BWAPI::Unit& unit);
        virtual void onUnitComplete(const BWAPI::Unit& unit);
        virtual void onUnitDestroy(const BWAPI::Unit& unit) = 0;
        int expectedSupplyProvided() const;
        int potentialSupplyUsed(const BWAPI::UnitType& unitType) const;
        virtual void createSupply();
        void createWorker();
        virtual void trainArmyUnit(const BWAPI::UnitType& unitType);
        virtual void construct(const BWAPI::UnitType& buildingType);
};

