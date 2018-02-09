#pragma once
#include <map>
#include <BWAPI.h>

using namespace BWAPI::Filter;

class BuildingConstructor {
    private:
        struct ConstrunctionPO;
        class RadicalOffset;
        std::map<BWAPI::UnitType, ConstrunctionPO> Preparing;
        std::map<BWAPI::Unit, ConstrunctionPO> Producing;
        BWAPI::TilePosition srcLocation;
        void beginPreparation(const BWAPI::UnitType& productType);
        ConstrunctionPO createJob(const BWAPI::UnitType& productType);
        BWAPI::TilePosition getPlacement(const ConstrunctionPO& Job) const;
        BWAPI::TilePosition getPlacement(
            const BWAPI::UnitType& buildingType) const;
        bool canBuildAdjacent(
            const BWAPI::TilePosition& buildingLocation) const;
        static bool isSmall(const BWAPI::Unit& unit);
        BWAPI::TilePosition adjacentBuildLocation(
            const BWAPI::TilePosition& buildingLocation) const;
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

class BuildingConstructor::RadicalOffset {
    private:
        class iterator;
        const int radius;
    public:
        RadicalOffset(int radius) : radius(radius) {}
        BuildingConstructor::RadicalOffset::iterator begin();
        BuildingConstructor::RadicalOffset::iterator end();
};

class BuildingConstructor::RadicalOffset::iterator {
    private:
        const int radius;
        int count;
    public:
        iterator(int radius, int count) : radius(radius), count(count) {}
        void operator++();
        bool operator==(iterator other) const;
        bool operator!=(iterator other) const;
        BWAPI::TilePosition operator*() const;
};
