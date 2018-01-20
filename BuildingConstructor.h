#pragma once
#include <map>
#include <vector>
#include <BWAPI.h>

using namespace BWAPI::Filter;

struct ConstrunctionPO {
    BWAPI::UnitType productType;
    BWAPI::TilePosition placement;
    BWAPI::Unit contractor;
    BWAPI::Unit product;
    ConstrunctionPO();
    ConstrunctionPO(const BWAPI::UnitType& productType);
};

class BuildingConstructor {
    private:
        std::map<BWAPI::UnitType, ConstrunctionPO> inPreparation;
        std::vector<ConstrunctionPO> inProduction;
        BWAPI::TilePosition srcPosition;
        void assignPreparation(const BWAPI::UnitType& productType);
        ConstrunctionPO createJob(const BWAPI::UnitType& productType);
        BWAPI::TilePosition getPlacement(const ConstrunctionPO& Job);
        BWAPI::Unit getContractor(const ConstrunctionPO& Job);
        BWAPI::Position toJobCenter(const ConstrunctionPO& Job);
        bool isAlreadyPreparing(const BWAPI::Unit& worker);
        bool isPrepared(const ConstrunctionPO& Job);
        void construct(const ConstrunctionPO& Job);
        void queueReturnToMining(const BWAPI::Unit& worker);
    public:
        void onStart(const BWAPI::TilePosition& srcPosition);
        void requestPreparation(const BWAPI::UnitType& productType);
        void updatePreparation();
        void promoteToProduction(const BWAPI::Unit& createdBuilding);
        void setAsComplete(const BWAPI::Unit& completedBuilding);
};
