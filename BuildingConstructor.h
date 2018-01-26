#pragma once
#include <map>
#include <BWAPI.h>

using namespace BWAPI::Filter;

class BuildingConstructor {
    private:
        struct ConstrunctionPO;
        std::map<BWAPI::UnitType, ConstrunctionPO> Preparing;
        std::map<BWAPI::Unit, ConstrunctionPO> Producing;
        BWAPI::TilePosition srcLocation;
        void beginPreparation(const BWAPI::UnitType& productType);
        ConstrunctionPO createJob(const BWAPI::UnitType& productType);
        BWAPI::TilePosition getPlacement(const ConstrunctionPO& Job);
        BWAPI::Unit getContractor(const ConstrunctionPO& Job);
        BWAPI::Position toJobCenter(const ConstrunctionPO& Job);
        bool isPrepared(const ConstrunctionPO& Job);
        void construct(const ConstrunctionPO& Job);
        void queueReturnToMining(const BWAPI::Unit& worker);
    public:
        BuildingConstructor();
        void request(const BWAPI::UnitType& productType);
        void updatePreparation();
        void onCreate(const BWAPI::Unit& createdBuilding);
        void onComplete(const BWAPI::Unit& completedBuilding);
};

struct BuildingConstructor::ConstrunctionPO {
    BWAPI::UnitType productType;
    BWAPI::TilePosition placement;
    BWAPI::Unit contractor;
    BWAPI::Unit product;
    ConstrunctionPO();
    ConstrunctionPO(const BWAPI::UnitType& productType);
};
