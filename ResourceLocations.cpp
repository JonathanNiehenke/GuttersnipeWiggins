#ifndef RESOURCELOCATIONS_CPP
#define RESOURCELOCATIONS_CPP
#include "ResourceLocations.h"

int ResourceBounds::getFirmEdge(const std::vector<int> &intCollection)
{
    int Mode = -1, modeCount = 0, Target = -1, targetCount = 0;
    for (int Current: intCollection) {
        if (Current == Target) {
            ++targetCount;
        }
        else {
            if (targetCount > 2) {
                // Assuming it is more than the resource ends.
                return Target;
            }
            else if (targetCount > modeCount) {
                Mode = Target;
                modeCount = targetCount;
            }
            Target = Current;
            targetCount = 1;
        }
    }
    return targetCount > modeCount ? Target : Mode;  // For last target
}

void ResourceBounds::drawAlongH_Bounds(int commonX, int minY, int maxY)
{
    BWAPI::Broodwar->registerEvent(
        [commonX, minY, maxY](BWAPI::Game*) {
            BWAPI::Broodwar->drawLineMap(
                commonX, minY, commonX, maxY, BWAPI::Color(255, 0, 0));
        }, nullptr, -1);
}

void ResourceBounds::drawAlongV_Bounds(int commonY, int minX, int maxX)
{
    BWAPI::Broodwar->registerEvent(
        [commonY, minX, maxX](BWAPI::Game*) {
            BWAPI::Broodwar->drawLineMap(
                minX, commonY, maxX, commonY, BWAPI::Color(255, 0, 0));
        }, nullptr, -1);
}

void ResourceBounds::draw() const
{
    drawAlongH_Bounds(leftFirmEdge, topEdge, botEdge);
    drawAlongH_Bounds(rightFirmEdge, topEdge, botEdge);
    drawAlongV_Bounds(topFirmEdge, leftEdge, rightEdge);
    drawAlongV_Bounds(botFirmEdge, leftEdge, rightEdge);
}

ResourceBounds::ResourceBounds(const BWAPI::Unitset &Resources)
{
    const int size = Resources.size(), centerTop = 96, centerLeft = 128;
    std::vector<int> Top, Left, Bottom, Right;
    Top.reserve(size);
    Left.reserve(size);
    Bottom.reserve(size);
    Right.reserve(size);
    for (BWAPI::Unit Resource: Resources) {
        Top.push_back(Resource->getTop());
        Left.push_back(Resource->getLeft());
        Bottom.push_back(Resource->getBottom());
        Right.push_back(Resource->getRight());
    }
    std::sort(Top.begin(), Top.end(), std::less<int>());
    std::sort(Left.begin(), Left.end(), std::less<int>());
    std::sort(Bottom.begin(), Bottom.end(), std::greater<int>());
    std::sort(Right.begin(), Right.end(), std::greater<int>());
    topEdge = Top.front();
    leftEdge = Left.front();
    botEdge = Bottom.front();
    rightEdge = Right.front();
    topFirmEdge = getFirmEdge(Top);
    leftFirmEdge = getFirmEdge(Left);
    botFirmEdge = getFirmEdge(Bottom);
    rightFirmEdge = getFirmEdge(Right);
}

void ResourceLocation::drawCenterSearch(BWAPI::Position resourceLocation, int a)
{
    // Live debugging info.
    BWAPI::Broodwar->registerEvent(
        [resourceLocation, a](BWAPI::Game*){
            BWAPI::Broodwar->drawCircleMap(resourceLocation, 8,
                BWAPI::Color(0, a, 0), true);
        },  nullptr, -1);
}

int ResourceLocation::getCoordValue(
    int firmEdge1, int Edge1, int firmEdge2, int Edge2, int centerAdj)
{
    assert(Edge1 < Edge2);
    assert(Edge1 <= firmEdge1);
    assert(firmEdge2 <= Edge2);
    const int resourceOffset = 96;
    if (firmEdge1 - Edge1 > Edge2 - firmEdge2)
        return firmEdge1 - resourceOffset - centerAdj;
    else if (Edge2 - firmEdge2 > firmEdge1 - Edge1)
        // +1 is to get from the bottom of the tile to the next tile
        return firmEdge2 + resourceOffset + 1;
    return firmEdge1 + 64 + resourceOffset;
}

int ResourceLocation::getXCoord(const ResourceBounds &bounds)
{
    return getCoordValue(bounds.getLeftFirmEdge(), bounds.getLeftEdge(),
                     bounds.getRightFirmEdge(), bounds.getRightEdge(), 128);
}

int ResourceLocation::getYCoord(const ResourceBounds &bounds)
{
    return getCoordValue(bounds.getTopFirmEdge(), bounds.getTopEdge(),
                         bounds.getBotFirmEdge(), bounds.getBotEdge(), 96);
}

BWAPI::TilePosition ResourceLocation::getBaseLocation(
    const BWAPI::Unitset &Resources)
{
    ResourceBounds resourceBounds(Resources);
    resourceBounds.draw();
    BWAPI::TilePosition baseLocation = BWAPI::TilePosition(
        BWAPI::Point<int, 1>(getXCoord(resourceBounds),
                             getYCoord(resourceBounds)));
    return baseLocation;
}

ResourceLocation::ResourceLocation(const BWAPI::Unitset &Resources)
{
    for (BWAPI::Unit Resource: Resources) {
        (Resource->getType().isMineralField()
         ? Minerals : Geysers).push_back(Resource);
    }
    buildLocation = getBaseLocation(Resources);
    Utils::Position fromBuild = Utils::Position::fromLocation(buildLocation);
    std::sort(Minerals.begin(), Minerals.end(), fromBuild.compareUnits());
    std::sort(Geysers.begin(), Geysers.end(), fromBuild.compareUnits());
}

#endif
