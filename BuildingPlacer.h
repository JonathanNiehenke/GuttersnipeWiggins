#pragma once
#include <BWAPI.h>

class BuildingPlacer {
    private:
        class RadicalOffset;
        int columnWidth, rowHeight;
        BWAPI::TilePosition srcLocation;
    public:
        BuildingPlacer(int columnWidth, int rowHeight);
        BWAPI::TilePosition getPlacement(
            const BWAPI::UnitType& buildingType) const;
        BWAPI::TilePosition getGasPlacement() const;
};

class BuildingPlacer::RadicalOffset {
    private:
        class iterator;
        const int radius;
    public:
        RadicalOffset(int radius) : radius(radius) {}
        BuildingPlacer::RadicalOffset::iterator begin();
        BuildingPlacer::RadicalOffset::iterator end();
};

class BuildingPlacer::RadicalOffset::iterator {
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
