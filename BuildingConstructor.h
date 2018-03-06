#pragma once
#include <map>
#include <BWAPI.h>
#include "BuildingPlacer.h"

using namespace BWAPI::Filter;

class BuildingConstructor {
    protected:
        struct ConstructionPO;
        std::map<BWAPI::UnitType, ConstructionPO> Preparing;
        std::map<BWAPI::Unit, ConstructionPO> Producing;
        BuildingPlacer* buildingPlacer;
        BWAPI::TilePosition srcLocation;
        void beginConstructionPreparation(ConstructionPO& Job) const;
        BWAPI::Unit getContractor(const ConstructionPO& Job) const;
        static BWAPI::Position toJobCenter(const ConstructionPO& Job);
        bool isPrepared(const ConstructionPO& Job);
        void construct(ConstructionPO& Job);
        void queueReturnToMining(const BWAPI::Unit& worker);
        static bool isObstructed(const ConstructionPO& Job);
        static bool isPreparing(const ConstructionPO& Job);
    public:
        BuildingConstructor(BuildingPlacer* buildingPlacer);
        virtual ~BuildingConstructor();
        virtual void request(const BWAPI::UnitType& productType);
        virtual void updatePreparation();
        void onCreate(const BWAPI::Unit& createdBuilding);
        void onComplete(const BWAPI::Unit& completedBuilding);
};


class MorphingConstructor : public BuildingConstructor {
    private:
        void beginMorphingPreparation(ConstructionPO& Job) const;
    public:
        MorphingConstructor(BuildingPlacer* buildingPlacer)
            : BuildingConstructor(buildingPlacer) {}
        void request(const BWAPI::UnitType& productType);
        void updatePreparation();
};

class AddonConstructor : public BuildingConstructor {
    private:
        void beginAddonPreparation(ConstructionPO& Job) const;
    public:
        AddonConstructor(BuildingPlacer* buildingPlacer)
            : BuildingConstructor(buildingPlacer) {}
        void request(const BWAPI::UnitType& productType);
        void updatePreparation();
};

struct BuildingConstructor::ConstructionPO {
    BWAPI::UnitType productType;
    BWAPI::TilePosition placement;
    BWAPI::Unit contractor;
    BWAPI::Unit product;
    ConstructionPO();
    ConstructionPO(const BWAPI::UnitType& productType);
};
