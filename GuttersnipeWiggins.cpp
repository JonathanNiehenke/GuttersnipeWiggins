/* Sorry, initially a python programmer and C++ lacks a standard style
enabling my accustomed line lengths of 72 for comments and 80 for code.
*/
#include "GuttersnipeWiggins.h"

using namespace BWAPI::Filter;

typedef std::vector<BWAPI::TilePosition> LocationVector;

class compareDistanceFrom
{
    private:
        BWAPI::Position sourcePosition = BWAPI::Positions::None;
    public:
        compareDistanceFrom(BWAPI::Position position)
            { sourcePosition = position; }
        compareDistanceFrom(BWAPI::TilePosition location)
            { sourcePosition = BWAPI::Position(location); }
        compareDistanceFrom(BWAPI::Unit unit)
            { sourcePosition = unit->getPosition(); }
        bool operator()(BWAPI::Position Pos1, BWAPI::Position Pos2);
        bool operator()(BWAPI::TilePosition tPos1, BWAPI::TilePosition tPos2);
        bool operator()(BWAPI::Unit unitV1, BWAPI::Unit unitV2);
};

bool compareDistanceFrom::operator()(BWAPI::Position Pos1, BWAPI::Position Pos2)
{
    return (sourcePosition.getApproxDistance(Pos1) <
            sourcePosition.getApproxDistance(Pos2));
}

bool compareDistanceFrom::operator()(
        BWAPI::TilePosition tPos1, BWAPI::TilePosition tPos2)
{
    return (sourcePosition.getApproxDistance(BWAPI::Position(tPos1)) <
            sourcePosition.getApproxDistance(BWAPI::Position(tPos2)));
}

bool compareDistanceFrom::operator()(BWAPI::Unit unit1, BWAPI::Unit unit2)
{
    return (sourcePosition.getApproxDistance(unit1->getPosition()) <
            sourcePosition.getApproxDistance(unit2->getPosition()));
}

LocationVector GW::getMineralClusterLocations()
{
    // Sort static minerals into "Starcraft" defined groups.
    std::map<int, std::vector<BWAPI::Unit>> groupedMinerals;
    for (BWAPI::Unit Mineral: BWAPI::Broodwar->getStaticMinerals())
        groupedMinerals[Mineral->getResourceGroup()].push_back(Mineral);
    // Collect the location of a mineral in each group.
    LocationVector clusterLocations;
    for (auto mineralGroup: groupedMinerals) {
        std::vector<BWAPI::Unit> mineralCluster = mineralGroup.second;
        // Ignore mineral clusters used as destructable terrain.
        if (mineralCluster.size() > 4) {
            clusterLocations.push_back(
                mineralCluster.back()->getTilePosition());
        }
    }
    return clusterLocations;
}

BWAPI::TilePosition START_POSITION;
LocationVector CLUSTER_LOCATIONS;

void scoutBases()
{
    BWAPI::Unit mineralScout = BWAPI::Broodwar->getClosestUnit(
        BWAPI::Position(START_POSITION), IsWorker);
    mineralScout->stop();  // Because the following orders are queued.
    for (auto mineralLocation: CLUSTER_LOCATIONS) {
        mineralScout->move(BWAPI::Position(mineralLocation), true);
    }
}

void GW::onStart()
{
    START_POSITION = BWAPI::Broodwar->self()->getStartLocation();
    CLUSTER_LOCATIONS = GW::getMineralClusterLocations();
    std::sort(CLUSTER_LOCATIONS.begin(), CLUSTER_LOCATIONS.end(),
        compareDistanceFrom(START_POSITION));
    scoutBases();
}

void GW::onFrame()
{
}

void GW::onUnitCreate(BWAPI::Unit Unit)
{
}

void GW::onUnitMorph(BWAPI::Unit Unit)
{
}

void GW::onUnitComplete(BWAPI::Unit Unit)
{
}

void GW::onUnitDestroy(BWAPI::Unit Unit)
{
}

void GW::onUnitDiscover(BWAPI::Unit Unit)
{
}

void GW::onUnitEvade(BWAPI::Unit Unit)
{
}

void GW::onUnitShow(BWAPI::Unit Unit)
{
}

void GW::onUnitHide(BWAPI::Unit Unit)
{
}

void GW::onUnitRenegade(BWAPI::Unit Unit)
{
}

void GW::onNukeDetect(BWAPI::Position target)
{
}

void GW::onSendText(std::string text)
{
}

void GW::onReceiveText(BWAPI::Player player, std::string text)
{
}

void GW::onPlayerLeft(BWAPI::Player player)
{
}

void GW::onSaveGame(std::string gameName)
{
}

void GW::onEnd(bool IsWinner)
{
}
