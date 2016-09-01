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


class EcoBase
{
    private:
        // About 3 miners per 2 minerals + 1 miner per 5 minerals.
        const float minersPerMineral = 1.625;  // 13 per 8.
        BWAPI::Unit Center = nullptr;
        BWAPI::UnitType workerType = BWAPI::UnitTypes::None;
        std::vector<BWAPI::Unit> Minerals;
        int minerCap = 0, minerCount = 0, mineralIndex = 0;
    public:
        EcoBase(BWAPI::Unit baseCenter);
        BWAPI::Unit getCenter()
            { return Center; }
        int getMinerCount()
            { return minerCount; }
        int getMinerCap()
            { return minerCap; }
        bool belowCap()
            { return minerCount < minerCap; }
        BWAPI::Unit getNextMineral();
        void releaveMiner()
            { --minerCount; }
        void removeMineral(BWAPI::Unit Mineral);
};

EcoBase::EcoBase(BWAPI::Unit baseCenter)
{
    Center = baseCenter;
    auto mineralFields = Center->getUnitsInRadius(380, IsMineralField);
    Minerals.assign(mineralFields.begin(), mineralFields.end());
    std::sort(Minerals.begin(), Minerals.end(), compareDistanceFrom(Center));
    minerCap = Minerals.size() * minersPerMineral;
}

BWAPI::Unit EcoBase::getNextMineral()
{
    ++minerCount;
    return Minerals[mineralIndex++ % Minerals.size()];
}

void EcoBase::removeMineral(BWAPI::Unit Mineral)
{
    auto itEnd = Minerals.end(),
         itFound = find(Minerals.begin(), itEnd, Mineral);
    if (itFound != itEnd) {
        Minerals.erase(itFound);
    minerCap = Minerals.size() * minersPerMineral;
    }
}

namespace Production
{
    
    bool isIdle(BWAPI::Unit Facility)
    {
        // Zerg hatchery is always idle so determine with larva.
        // Negate larva for Protoss and Terran with producesLarva.
        return (Facility->isIdle() && !(Facility->getType().producesLarva() &&
                Facility->getLarva().empty()));
    }

    bool canProduce(std::vector<EcoBase*> EcoBases)
    {
        for (EcoBase *Base: EcoBases) {
            if (Base->belowCap() && isIdle(Base->getCenter()))
                return true;
        }
        return false;
    }

    // For overlords.
    void produceSingleUnit(std::vector<EcoBase*> EcoBases,
        BWAPI::UnitType unitType)
    {
        for (EcoBase *Base: EcoBases) {
            BWAPI::Unit Center = Base->getCenter();
            if (Base->belowCap() && isIdle(Center)) {
                Center->train(unitType);
                break;
            }
        }
    }

    void produceUnits(std::vector<EcoBase*> EcoBases, BWAPI::UnitType unitType)
    {
        for (EcoBase *Base: EcoBases) {
            BWAPI::Unit Center = Base->getCenter();
            if (Base->belowCap() && isIdle(Center))
                Center->train(unitType);
        }
    }

    EcoBase* findCreator(std::vector<EcoBase*> EcoBases, BWAPI::Unit Creator)
    {
        for (EcoBase* Base: EcoBases) {
            if (Base->getCenter() == Creator)
                return Base;
        }
    return nullptr;
    }
}

BWAPI::Player SELF;
BWAPI::Unit BASE_CENTER;
BWAPI::UnitType CENTER_TYPE, WORKER_TYPE, SUPPLY_TYPE;
locationVector MINERAL_LOCATIONS;
std::vector<EcoBase*> ECO_BASES;
std::map<BWAPI::Unit, BWAPI::Unit> UNIT_CREATOR;

void GW::onStart()
{
    SELF = BWAPI::Broodwar->self();
    BWAPI::TilePosition startLocation = SELF->getStartLocation();
    BASE_CENTER = BWAPI::Broodwar->getClosestUnit(
        BWAPI::Position(startLocation), IsResourceDepot);
    BWAPI::Race myRace = SELF->getRace();
    CENTER_TYPE = myRace.getCenter();
    WORKER_TYPE = myRace.getWorker();
    SUPPLY_TYPE = myRace.getSupplyProvider();
    MINERAL_LOCATIONS = GW::getMineralClusterLocations();
    std::sort(MINERAL_LOCATIONS.begin(), MINERAL_LOCATIONS.end(),
        compareDistanceFrom(startLocation));
    ECO_BASES.push_back(new EcoBase(BASE_CENTER));
}

void GW::onFrame()
{
    const int latentcy = BWAPI::Broodwar->getLatency(),
              centerPrice = CENTER_TYPE.mineralPrice();
    GW::displayState();
    if (BWAPI::Broodwar->getFrameCount() % latentcy)
        return;
    if (SELF->supplyTotal() - SELF->supplyUsed() <= 3)
        GW::constructUnit(SUPPLY_TYPE);
    else if (Production::canProduce(ECO_BASES))
        Production::produceUnits(ECO_BASES, WORKER_TYPE);
    else if (SELF->minerals() >= centerPrice - 48) {
        GW::constructExpansion();
    }
}

void GW::onUnitCreate(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() != SELF)
        return;
    BWAPI::Unit unitCreator = GW::getUnitCreator(Unit);
    if (unitCreator) {
        UNIT_CREATOR[Unit] = unitCreator;
    }
}

void GW::onUnitMorph(BWAPI::Unit Unit)
{
}

void GW::onUnitComplete(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() != SELF)
        return;
    BWAPI::UnitType unitType = Unit->getType();
    BWAPI::Unit unitCreator = UNIT_CREATOR[Unit];
    if (unitType == WORKER_TYPE) {
        EcoBase *Base = Production::findCreator(ECO_BASES, unitCreator);
        Unit->gather(Base->getNextMineral());
    }
    else if (unitType.isResourceDepot() && Unit != BASE_CENTER) {
        ECO_BASES.push_back(new EcoBase(Unit));
    }
    UNIT_CREATOR.erase(Unit);
}

void GW::onUnitDestroy(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() == SELF && Unit->getType().isResourceDepot()) {
        EcoBase *Base = Production::findCreator(ECO_BASES, Unit);
        delete Base;
        auto itEnd = ECO_BASES.end(),
             itFound = find(ECO_BASES.begin(), itEnd, Base);
        if (itFound != itEnd) {
            ECO_BASES.erase(find(ECO_BASES.begin(), itEnd, Base));
        }
    }
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
    for (EcoBase *Base: ECO_BASES) {
        delete Base;
        Base = nullptr;
    }
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

BWAPI::Unit GW::getUnitCreator(BWAPI::Unit developingUnit)
{
    BWAPI::TilePosition developingLocation = developingUnit->getTilePosition();
    BWAPI::UnitType creatorType = developingUnit->getType().whatBuilds().first;
    BWAPI::Unitset creatorUnits = BWAPI::Broodwar->getUnitsOnTile(
        developingLocation, GetType == creatorType);
    return creatorUnits.size() == 1 ? *creatorUnits.begin() : nullptr;
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
            Task = Other;
    }
    return Task;
}

BWAPI::TilePosition GW::getExpansionLocation(BWAPI::Unit centerContractor)
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
    BWAPI::TilePosition constructionLocation, BWAPI::Unit contractorUnit,
    int Task = -1)
{
    enum {Unexceptable = -1, Position = 1, Build};
    if (Task == Unexceptable)
        Task = GW::getContractorTask(contractorUnit);
    switch (Task) {
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

void GW::constructUnit(BWAPI::UnitType constructableType)
{
    enum {Position = 1, Build};
    static BWAPI::Unit contractorUnit = nullptr;
    static BWAPI::TilePosition constructionLocation = BWAPI::TilePositions::None;
    int Task = GW::getContractorTask(contractorUnit);
    switch (Task) {
        case Position:
            GW::constructUnit(
                constructableType, constructionLocation, contractorUnit, Task);
        case Build:
            break;  // Do not reissue command;
        default:
            contractorUnit = BASE_CENTER->getClosestUnit(IsWorker);
            BWAPI::TilePosition desiredLocation = (
                !(constructionLocation == BWAPI::TilePositions::None &&
                    contractorUnit)
                ? constructionLocation : contractorUnit->getTilePosition());
            constructionLocation = BWAPI::Broodwar->getBuildLocation(
                constructableType, desiredLocation);
            if (constructionLocation != BWAPI::TilePositions::Invalid)
                contractorUnit->move(BWAPI::Position(constructionLocation));
    }
}
    
void GW::constructExpansion()
{
    enum {Position = 1, Build};
    static BWAPI::Unit centerContractor = nullptr;
    static BWAPI::TilePosition expansionLocation = (
        BWAPI::TilePositions::Invalid);
    int Task = GW::getContractorTask(centerContractor);
    switch (Task) {
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
                    CENTER_TYPE, expansionLocation, centerContractor, Task);
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

void GW::displayState()
{
    int row = 15;
    BWAPI::Broodwar->drawTextScreen(3, 3, "APM %d, FPS %d, avgFPS %f",
        BWAPI::Broodwar->getAPM(), BWAPI::Broodwar->getFPS(),
        BWAPI::Broodwar->getAverageFPS());
    for (auto pair: UNIT_CREATOR) {
        BWAPI::Unit Creation = pair.first;
        BWAPI::Unit Creator = pair.second;
        if (Creator) {
            BWAPI::Broodwar->drawTextScreen(3, row, "CREATION %s, Creator %s",
                Creation->getType().c_str(), Creator->getType().c_str());
        }
        else {
            BWAPI::Broodwar->drawTextScreen(3, row, "Creator na, Creation %s",
                Creation->getType().c_str());
        }
        row += 10;
    }
    row += 5;
    for (EcoBase *Base: ECO_BASES) {
        BWAPI::Broodwar->drawTextScreen(3, row,
            "ID %d  minerCount %d  minerCap %d  belowCap %s",
            Base->getCenter()->getID(), Base->getMinerCount(),
            Base->getMinerCap(), (Base->belowCap() ? "true" : "false"));
        row += 10;
    }
}
