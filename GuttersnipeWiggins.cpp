/* Sorry, initially a python programmer and C++ lacks a standard style
enabling my accustomed line lengths of 72 for comments and 80 for code.
*/
#include "GuttersnipeWiggins.h"

using namespace BWAPI::Filter;

typedef std::vector<BWAPI::TilePosition> locationVector;

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
locationVector MINERAL_LOCATIONS;

void GW::onStart()
{
    SELF = BWAPI::Broodwar->self();
    BWAPI::TilePosition startLocation = SELF->getStartLocation();
    BASE_CENTER = BWAPI::Broodwar->getClosestUnit(
        BWAPI::Position(startLocation), IsResourceDepot);
    BWAPI::Race myRace = SELF->getRace();
    CENTER_TYPE = myRace.getCenter();
    WORKER_TYPE = myRace.getWorker();
    MINERAL_LOCATIONS = GW::getMineralClusterLocations();
    std::sort(MINERAL_LOCATIONS.begin(), MINERAL_LOCATIONS.end(),
        compareDistanceFrom(startLocation));
}

void GW::onFrame()
{
    const int latentcy = BWAPI::Broodwar->getLatency();
    if (BWAPI::Broodwar->getFrameCount() % latentcy)
        return;
    const int centerPrice = CENTER_TYPE.mineralPrice();
    if (SELF->minerals() >= centerPrice - 48) {
        GW::constructExpansion();
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
    if (Unit->getType() == WORKER_TYPE)
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

locationVector GW::getMineralClusterLocations()
{
    // Group static minerals into "Starcraft" defined groups.
    std::map<int, BWAPI::Unitset> groupedMinerals;
    for (BWAPI::Unit Mineral: BWAPI::Broodwar->getStaticMinerals())
        groupedMinerals[Mineral->getResourceGroup()].insert(Mineral);
    // Collect the location of each of the minerals groups.
    locationVector clusterLocations;
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

int GW::getContractorTask(BWAPI::Unit contractorUnit)
{
    enum {Other, Position, Build};
    int Task;
    BWAPI::Order currentOrder = (contractorUnit
        ? contractorUnit->getOrder() : BWAPI::Orders::None);
    switch (currentOrder) {
        case BWAPI::Orders::Enum::PlaceBuilding:
        case BWAPI::Orders::Enum::ResetCollision:
            Task = Build;
            break;
        case BWAPI::Orders::Enum::Move:
        case BWAPI::Orders::Enum::Guard:  // A short wait.
        case BWAPI::Orders::Enum::PlayerGuard:  // A long wait.
            Task = Position;
            break;
        default:
            BWAPI::Broodwar->sendTextEx(true, "Current order %s, Minerals %d.",
                currentOrder.c_str(), SELF->minerals());
            Task = Other;
    }
    return Task;
}

BWAPI::TilePosition GW::getExpansionLocation(
    BWAPI::Unit centerContractor)
{
    BWAPI::TilePosition expansionLocation = BWAPI::TilePositions::Invalid;
    for (auto baseLocation: MINERAL_LOCATIONS) {
        BWAPI::Position basePosition = BWAPI::Position(baseLocation);
        if (centerContractor && centerContractor->hasPath(basePosition) &&
            !BWAPI::Broodwar->isExplored(baseLocation))
        {
            expansionLocation = BWAPI::Broodwar->getBuildLocation(
                CENTER_TYPE, baseLocation);
            break;
        }
    }
    return expansionLocation;
}


void GW::constructUnit(BWAPI::UnitType constructableType,
    BWAPI::TilePosition constructionLocation, BWAPI::Unit contractorUnit)
{
    enum {Position = 1, Build};
    switch (GW::getContractorTask(contractorUnit)) {
        case Position:
            if (contractorUnit->canBuild(
                    constructableType, constructionLocation))
            {
                contractorUnit->build(constructableType, constructionLocation);
                // Queues command to return to minerals. !Working for Zerg.
                contractorUnit->gather(
                    BWAPI::Broodwar->getClosestUnit(
                        BWAPI::Position(constructionLocation), IsMineralField),
                    true);
            }
        case Build:
            break;  // Do not reissue command;
        default:
            contractorUnit->move(BWAPI::Position(constructionLocation));
    }
}

void GW::constructExpansion()
{
    enum {Position = 1, Build};
    static BWAPI::Unit centerContractor = nullptr;
    static BWAPI::TilePosition expansionLocation = (
        BWAPI::TilePositions::Invalid);
    switch (GW::getContractorTask(centerContractor)) {
        case Position:
            bool isVisible;  // Prevents error C2360.
            isVisible = BWAPI::Broodwar->isVisible(expansionLocation);
            if (isVisible && BWAPI::Broodwar->getClosestUnit(
                    BWAPI::Position(expansionLocation), IsResourceDepot, 300))
            {
                expansionLocation = GW::getExpansionLocation(centerContractor);
                centerContractor->move(BWAPI::Position(expansionLocation));
            }
            else if (isVisible) {
                GW::constructUnit(
                    CENTER_TYPE, expansionLocation, centerContractor);
            }
        case Build:
            break;  // Do not reissue command;
        default:
            centerContractor = BASE_CENTER->getClosestUnit(IsWorker);
            expansionLocation = GW::getExpansionLocation(centerContractor);
            if (expansionLocation != BWAPI::TilePositions::Invalid)
                centerContractor->move(BWAPI::Position(expansionLocation));
    }
}
