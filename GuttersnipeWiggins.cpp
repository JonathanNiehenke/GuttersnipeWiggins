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

BWAPI::Player SELF;
BWAPI::Unit BASE_CENTER;
BWAPI::UnitType CENTER_TYPE, WORKER_TYPE;
LocationVector BASE_LOCATIONS;

void GW::onStart()
{
    SELF = BWAPI::Broodwar->self();
    BWAPI::TilePosition startLocation = SELF->getStartLocation();
    BASE_CENTER = BWAPI::Broodwar->getClosestUnit(
        BWAPI::Position(startLocation), IsResourceDepot);
    BWAPI::Race myRace = SELF->getRace();
    CENTER_TYPE = myRace.getCenter();
    WORKER_TYPE = myRace.getWorker();
    BASE_LOCATIONS = GW::getMineralClusterLocations();
    std::sort(BASE_LOCATIONS.begin(), BASE_LOCATIONS.end(),
        compareDistanceFrom(startLocation));
    scoutBases();
}

BWAPI::TilePosition GW::getExpansionLocation(BWAPI::Unit centerContractor)
{
    for (auto baseLocation: BASE_LOCATIONS) {
        BWAPI::Position basePosition = BWAPI::Position(baseLocation);
        if (centerContractor && centerContractor->hasPath(basePosition) &&
            !BWAPI::Broodwar->getClosestUnit(basePosition, IsResourceDepot, 300))
        {
            return BWAPI::Broodwar->getBuildLocation(
                    CENTER_TYPE, BWAPI::TilePosition(baseLocation));
        }
    }
    return BWAPI::TilePositions::Invalid;
}

void GW::onFrame()
{
    const int centerPrice = CENTER_TYPE.mineralPrice();
    static bool buildingExpansion = false;
    if (SELF->minerals() >= centerPrice && !buildingExpansion) {
        BWAPI::Unit centerContractor = BASE_CENTER->getClosestUnit(
            IsWorker && IsOwned);
        BWAPI::TilePosition expansionLocation = GW::getExpansionLocation(
            centerContractor);
        if (expansionLocation != BWAPI::TilePositions::Invalid) {
            buildingExpansion = centerContractor->build(
                CENTER_TYPE, expansionLocation);
        }
        else {
            BWAPI::Broodwar->sendTextEx(true, "Invalid location");
        }
        BWAPI::Broodwar->sendTextEx(true, "buildingExpansion: %s",
            buildingExpansion ? "true" : "false");
    }
}

void GW::onUnitCreate(BWAPI::Unit Unit)
{
}

void GW::onUnitMorph(BWAPI::Unit Unit)
{
}

void GW::onUnitComplete(BWAPI::Unit Unit)
{
    BWAPI::UnitType unitType = Unit->getType();
    if (unitType == WORKER_TYPE && Unit->getOrder() != BWAPI::Orders::Stop)
        Unit->gather(Unit->getClosestUnit(IsMineralField));
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

LocationVector GW::getMineralClusterLocations()
{
    // Sort static minerals into "Starcraft" defined groups.
    std::map<int, BWAPI::Unitset> groupedMinerals;
    for (BWAPI::Unit Mineral: BWAPI::Broodwar->getStaticMinerals())
        groupedMinerals[Mineral->getResourceGroup()].insert(Mineral);
    // Collect the location of a mineral in each group.
    LocationVector clusterLocations;
    for (auto mineralGroup: groupedMinerals) {
        BWAPI::Unitset mineralCluster = mineralGroup.second;
        // Ignore mineral clusters used as destructible terrain.
        if (mineralCluster.size() > 4) {
            clusterLocations.push_back(
                BWAPI::TilePosition(mineralCluster.getPosition()));
        }
    }
    return clusterLocations;
}

void GW::scoutBases()
{
    BWAPI::Unit mineralScout = BASE_CENTER->getClosestUnit(IsWorker);
    mineralScout->stop();  // Because the following orders are queued.
    for (auto baseLocation: BASE_LOCATIONS) {
        BWAPI::Position targetPosition = BWAPI::Position(baseLocation);
        // Ignore terrain inaccessible positions.
        if (mineralScout->hasPath(targetPosition))
            mineralScout->move(targetPosition, true);
    }
}

