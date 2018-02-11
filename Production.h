/* Interface to unify management of all three races */

#pragma once
#include <BWAPI.h>
#include "UnitTrainer.h"
#include "BuildingConstructor.h"
#include "ResourceSupplier.h"
#include "TechTree.h"

class Production {
    protected:
        ResourceSupplier* resourceSupplier;
        BuildingConstructor* buildingConstructor;
        UnitTrainer* unitTrainer;
        TechTree* techTree;
        BWAPI::UnitType centerType, workerType, supplyType, armyUnitType;
        virtual void onCompleteBuilding(const BWAPI::Unit&) const;
        void onDestroyedBuilding(const BWAPI::Unit&) const;
        virtual int expectedSupplyProvided(const BWAPI::UnitType&) const;
    public:
        Production(const BWAPI::UnitType& armyUnitType);
        ~Production();
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
            const BWAPI::UnitType& unitType) const;
        virtual BWAPI::UnitType facilityFor(const BWAPI::UnitType&) const;
};

class ProtossProduction : public Production
{
    private:
        bool doesPylonExist() const;
    public:
        ProtossProduction() : Production(
            BWAPI::UnitTypes::Enum::Protoss_Zealot) {}
        BWAPI::UnitType getNextRequiredBuilding(
            const BWAPI::UnitType& unitType) const;
};

class TerranProduction : public Production
{
    private:
    public:
        TerranProduction() : Production(
            BWAPI::UnitTypes::Enum::Terran_Marine) {}
};

class ZergProduction : public Production
{
    private:
        int incompleteOverlordCount = 0;
        static bool isIncompleteOverlord(const BWAPI::Unit& unit);
        int expectedSupplyProvided(const BWAPI::UnitType&) const;
        bool doesTechExist(const BWAPI::UnitType& buildingType) const;
    public:
        ZergProduction() : Production(BWAPI::UnitTypes::Enum::Zerg_Zergling) {}
        void onUnitMorph(const BWAPI::Unit& morphedUnit);
        void onUnitComplete(const BWAPI::Unit& completedUnit);
        void createSupply() const;
        void construct(const BWAPI::UnitType& buildingType) const;
        BWAPI::UnitType facilityFor(const BWAPI::UnitType& unitType) const;
};