#ifndef RESOURCELOCATIONS_H
#define RESOURCELOCATIONS_H
#include <cassert>
#include <BWAPI.h>
#include "Utils.h"

class ResourceBounds
{
    private:
        int topEdge, leftEdge, botEdge, rightEdge,
            topFirmEdge, leftFirmEdge, botFirmEdge, rightFirmEdge;
        static int getFirmEdge(const std::vector<int> &intCollection);
        static void drawAlongH_Bounds(int, int, int);
        static void drawAlongV_Bounds(int, int, int);
    public:
        ResourceBounds(const BWAPI::Unitset &);
        int getTopEdge() const { return topEdge; }
        int getLeftEdge() const { return leftEdge; }
        int getBotEdge() const { return botEdge; }
        int getRightEdge() const { return rightEdge; }
        int getTopFirmEdge() const { return topFirmEdge; }
        int getLeftFirmEdge() const { return leftFirmEdge; }
        int getBotFirmEdge() const { return botFirmEdge; }
        int getRightFirmEdge() const { return rightFirmEdge; }
        void draw() const;
};

class ResourceLocation
{
    private:
        std::vector<BWAPI::Unit> Minerals, Geysers;
        BWAPI::TilePosition buildLocation;
        static void drawCenterSearch(BWAPI::Position resourceLocation, int a);
        static BWAPI::TilePosition getBaseLocation(const BWAPI::Unitset&);
        static int getCoordValue(int, int, int, int, int);
        static int getXCoord(const ResourceBounds &resourceBounds);
        static int getYCoord(const ResourceBounds &resourceBounds);
    public:
        ResourceLocation(const BWAPI::Unitset &Resources);
        const std::vector<BWAPI::Unit>& getMinerals() const
            { return Minerals; }
        const std::vector<BWAPI::Unit>& getGeysers() const
            { return Geysers; }
        const BWAPI::TilePosition& getLocation() const
            { return buildLocation; }
        const BWAPI::Position getPosition() const
            { return BWAPI::Position(buildLocation); }
};

#endif
