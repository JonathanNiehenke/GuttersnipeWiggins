/* Sorry, initially a python programmer and C++ lacks a standard style
enabling my accustomed line lengths of 72 for comments and 80 for code.
*/
#include "GuttersnipeWiggins.h"

using namespace BWAPI::Filter;

typedef std::pair<BWAPI::Position, BWAPI::Unitset> PositionedUnits;

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
        compareDistanceFrom(BWAPI::Unitset units)
            { sourcePosition = units.getPosition(); }
        bool operator()(BWAPI::Position Pos1, BWAPI::Position Pos2);
        bool operator()(BWAPI::TilePosition tPos1, BWAPI::TilePosition tPos2);
        bool operator()(BWAPI::Unit u1, BWAPI::Unit u2);
        bool operator()(BWAPI::Unitset u1, BWAPI::Unitset u2);
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

bool compareDistanceFrom::operator()(BWAPI::Unit u1, BWAPI::Unit u2)
{
    return (sourcePosition.getApproxDistance(u1->getPosition()) <
            sourcePosition.getApproxDistance(u2->getPosition()));
}

bool compareDistanceFrom::operator()(BWAPI::Unitset u1, BWAPI::Unitset u2)
{
    return (sourcePosition.getApproxDistance(u1.getPosition()) <
            sourcePosition.getApproxDistance(u2.getPosition()));
}


class EcoBase
{
    private:
        // About 3 miners per 2 minerals + 1 miner per 5 minerals.
        const float minersPerMineral = 1.625;  // 13 per 8.
        BWAPI::Unit Center = nullptr;
        std::vector<BWAPI::Unit> Minerals;
        int minerCap = 0, minerCount = 0, mineralIndex = 0;
    public:
        EcoBase(BWAPI::Unit center, BWAPI::Unitset mineralCluster);
        BWAPI::Unit getCenter()
            { return Center; }
        int getMinerCount()
            { return minerCount; }
        int getMinerCap()
            { return minerCap; }
        bool belowCap()
            { return minerCount < minerCap; }
        void assignMiner(BWAPI::Unit minerUnit);
        void releaseMiner(BWAPI::Unit minerUnit);
        void removeMineral(BWAPI::Unit Mineral);
};

EcoBase::EcoBase(BWAPI::Unit center, BWAPI::Unitset mineralCluster)
{
    Center = center;
    Minerals.assign(mineralCluster.begin(), mineralCluster.end());
    std::sort(Minerals.begin(), Minerals.end(), compareDistanceFrom(center));
    minerCap = Minerals.size() * minersPerMineral;
}

void EcoBase::assignMiner(BWAPI::Unit minerUnit)
{
    ++minerCount;
    minerUnit->gather(Minerals[mineralIndex++ % Minerals.size()]);
}

void EcoBase::releaseMiner(BWAPI::Unit minerUnit)
{
    --minerCount;
    minerUnit->stop();
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

}

BWAPI::Player SELF;
BWAPI::Unit BASE_CENTER;
BWAPI::UnitType CENTER_TYPE, WORKER_TYPE, SUPPLY_TYPE;
std::vector<PositionedUnits> MAP_MINERALS;
std::unordered_map<BWAPI::Unit, BWAPI::Unit> UNIT_CREATOR;
std::vector<EcoBase*> MY_BASES;

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
    MAP_MINERALS = GW::getMapMinerals();
    auto startPosition = BWAPI::Position(startLocation);
    std::sort(MAP_MINERALS.begin(), MAP_MINERALS.end(),
        [startPosition](PositionedUnits pu1, PositionedUnits pu2)
        {
            return (startPosition.getApproxDistance(pu1.first) <
                    startPosition.getApproxDistance(pu2.first));
        });
    // Workaround cause initial workers may complete before the BASE_CENTER.
    MY_BASES.push_back(new EcoBase(BASE_CENTER, MAP_MINERALS.front().second));
}

void GW::onFrame()
{
    const int latentcy = BWAPI::Broodwar->getLatency(),
              centerPrice = CENTER_TYPE.mineralPrice();
    GW::displayState();
    if (BWAPI::Broodwar->getFrameCount() % latentcy)
        return;
    if (SELF->supplyTotal() - SELF->supplyUsed() <= 3) {
        if (SUPPLY_TYPE.isBuilding())
            GW::constructUnit(SUPPLY_TYPE);
        else
            Production::produceSingleUnit(MY_BASES, SUPPLY_TYPE);
    }
    else if (Production::canProduce(MY_BASES))
        Production::produceUnits(MY_BASES, WORKER_TYPE);
    else if (SELF->minerals() >= centerPrice - 100) {
        GW::constructExpansion();
    }
}

void GW::onUnitCreate(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() != SELF)
        return;
    if (Unit->getType() == WORKER_TYPE)
        UNIT_CREATOR[Unit] = Unit->getClosestUnit(IsResourceDepot, 0);
}

void GW::onUnitMorph(BWAPI::Unit Unit)
{
    const BWAPI::UnitType Egg = BWAPI::UnitTypes::Zerg_Egg;
    if (Unit->getPlayer() != SELF)
        return;
    if (Unit->getType() == Egg && Unit->getBuildType() == WORKER_TYPE) {
        UNIT_CREATOR[Unit] = Unit->getClosestUnit(IsResourceDepot, 64);
    }
}

void GW::onUnitComplete(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() != SELF)
        return;
    BWAPI::UnitType unitType = Unit->getType();
    if (unitType == WORKER_TYPE) {
        BWAPI::Unit cUnit = UNIT_CREATOR[Unit];
        auto foundIt = std::find_if(MY_BASES.begin(), MY_BASES.end(),
            [cUnit](EcoBase *Base){ return Base->getCenter() == cUnit; });
        if (foundIt != MY_BASES.end())
            (*foundIt)->assignMiner(Unit);
        else // Should only be initial units.
            MY_BASES.front()->assignMiner(Unit);
    }
    // Excludes Lair and Hive which I assume are resource depots.
    // Also don't duplicated BASE_CENTER EcoBase, done at onStart.
    else if (unitType == CENTER_TYPE && Unit != BASE_CENTER) {
        // Append to MY_BASES after finding nearest mineralCluster.
        for (PositionedUnits mineralCluster: MAP_MINERALS) {
            BWAPI::Unit center = BWAPI::Broodwar->getClosestUnit(
                mineralCluster.first, IsResourceDepot, 300);
            if (center == Unit) {
                MY_BASES.push_back(new EcoBase(Unit, mineralCluster.second));
                break;
            }
        }
    }
    UNIT_CREATOR.erase(Unit);
}

void GW::onUnitDestroy(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() == SELF && Unit->getType().isResourceDepot()) {
        // Includes potential Lair or Hive with isResourceDepot.
        auto foundIt = std::find_if(MY_BASES.begin(), MY_BASES.end(),
            [Unit](EcoBase *Base){ return Base->getCenter() == Unit; });
        if (foundIt != MY_BASES.end()) {
            delete *foundIt;
            MY_BASES.erase(foundIt);
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
    for (EcoBase *Base: MY_BASES) {
        delete Base;
        Base = nullptr;
    }
}

std::vector<PositionedUnits> GW::getMapMinerals()
{
    // Group static minerals into "Starcraft" defined groups.
    std::map<int, BWAPI::Unitset> groupedMinerals;
    for (BWAPI::Unit Mineral: BWAPI::Broodwar->getStaticMinerals())
        groupedMinerals[Mineral->getResourceGroup()].insert(Mineral);
    std::vector<PositionedUnits> mapMinerals;
    for (auto mineralGroup: groupedMinerals) {
        BWAPI::Unitset mineralCluster = mineralGroup.second;
        // Ignore mineral clusters possibly used as terrain.
        if (mineralCluster.size() > 4)
            mapMinerals.push_back(std::make_pair(
                mineralCluster.getPosition(), mineralCluster));
    }
    return mapMinerals;
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
    //! Function may cause performance lag.
    const int indexMax = MAP_MINERALS.size();
    static int expandIndex = 0;
    auto expansionLocation = BWAPI::TilePositions::Invalid;
    if (!centerContractor)
        return expansionLocation;
    // Wrap around iteration of MAP_MINERALS positons.
    int indexCycle = expandIndex + indexMax;
    BWAPI::Position avgMineralPosition;
    BWAPI::TilePosition avgMineralLocation;
    do {
        avgMineralPosition = MAP_MINERALS[expandIndex % indexMax].first;
        avgMineralLocation = BWAPI::TilePosition(avgMineralPosition);
        ++expandIndex;
    }
    while (BWAPI::Broodwar->isVisible(avgMineralLocation) &&
           !centerContractor->hasPath(avgMineralPosition) &&
           expandIndex < indexCycle);
    // If all avgMineralPositions are unacceptable.
    if (expandIndex != indexCycle) {
        expansionLocation = BWAPI::Broodwar->getBuildLocation(
            CENTER_TYPE, avgMineralLocation, 24);
    }
    ++expandIndex;  // Prevent immediate duplication of return.
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
            bool isVisible;  // Prevents error C2360 by not initalizing.
            isVisible = BWAPI::Broodwar->isVisible(expansionLocation);
            if (isVisible && BWAPI::Broodwar->getClosestUnit(
                    BWAPI::Position(expansionLocation), IsResourceDepot, 300))
            {
                expansionLocation = GW::getExpansionLocation(centerContractor);
                if (expansionLocation != BWAPI::TilePositions::Invalid)
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
            if (expansionLocation != BWAPI::TilePositions::Invalid) {
                MY_BASES.front()->releaseMiner(centerContractor);
                centerContractor->move(BWAPI::Position(expansionLocation));
            }
    }
}

void GW::displayState()
{
    BWAPI::Broodwar->drawTextScreen(3, 3, "APM %d, FPS %d, avgFPS %f",
        BWAPI::Broodwar->getAPM(), BWAPI::Broodwar->getFPS(),
        BWAPI::Broodwar->getAverageFPS());
    int row = 15;
    for (EcoBase *Base: MY_BASES) {
        BWAPI::Broodwar->drawTextScreen(3, row,
            "ID %d  minerCount %d   minerCap %d   belowCap %s",
            Base->getCenter()->getID(), Base->getMinerCount(),
            Base->getMinerCap(), (Base->belowCap() ? "true" : "false"));
        row += 10;
    }
}
