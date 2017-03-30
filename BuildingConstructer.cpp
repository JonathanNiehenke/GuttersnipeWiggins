#ifndef BUILDINGCONSTRUCTER_CPP
#define BUILDINGCONSTRUCTER_CPP
#include "BuildingConstructer.h"

using namespace BWAPI::Filter;

ConstructionPO::ConstructionPO(
    BWAPI::Unit contractor,
    BWAPI::UnitType constructable,
    BWAPI::TilePosition location)
{
    this->contractor = contractor;
    this->constructable = constructable;;
    this->location = location;
    int status = Other;
    BWAPI::Unit product = nullptr;
}

void ConstructionPO::updateStatus()
{
    // What if: product dies?
    if (product && product->isConstructing()) {
        status = Constructing;
    }
    else {
        switch (contractor->getOrder()) {
            case BWAPI::Orders::Enum::MoveToMinerals:
                status = Mining;
                break;
            case BWAPI::Orders::Enum::Move:
            case BWAPI::Orders::Enum::Guard:  // A short wait.
            case BWAPI::Orders::Enum::PlayerGuard:  // A long wait.
                status = Positioning;
                break;
            // Units are discovered occupying the same space.
            case BWAPI::Orders::Enum::ResetCollision:  
            case BWAPI::Orders::Enum::PlaceBuilding:
                status = Constructing;
                break;
            case BWAPI::Orders::Enum::IncompleteBuilding:
                product = contractor;
                status = Constructing;
                break;
            default:
                BWAPI::Broodwar << "Other order: " << contractor->getOrder()
                                << std::endl;
                status = Other;
        }
    }
}

// Would prefer to use lookup keys, but UnitTypes get the C2338 error.
ConstructionPO& BuildingConstructer::findJob(BWAPI::UnitType Constructable)
{
    for (ConstructionPO &Job: constructionJobs) {
        if (Job.constructable == Constructable) {
            return Job;
        }
    }
    throw NoJob();
}

// Auto for iterators. :)
std::vector<ConstructionPO>::iterator BuildingConstructer::findJob(
    BWAPI::Unit Product)
{
    auto endIt = constructionJobs.end(),
         It = constructionJobs.begin();
    for (; It != endIt; ++It) {
        if (It->product == Product) {
            return It;
        }
    }
    throw NoJob();
}

void BuildingConstructer::beginConstruction(BWAPI::UnitType Constructable)
{
    // ToDo: Optimally choose contractor.
    BWAPI::Unit contractorUnit = baseCenter->getClosestUnit(IsWorker);
    if (!contractorUnit) return;
    // ToDo: Choose better construction locations.
    BWAPI::TilePosition constructionLocation = BWAPI::Broodwar->getBuildLocation(
        Constructable, contractorUnit->getTilePosition());
    if (constructionLocation != BWAPI::TilePositions::Invalid)
        if (!contractorUnit->move(BWAPI::Position(constructionLocation))) {
            cmdRescuer->append(CmdRescuer::MoveCommand(
                contractorUnit, BWAPI::Position(constructionLocation)));
        }
        constructionJobs.push_back(ConstructionPO(
            contractorUnit, Constructable, constructionLocation));
}

void BuildingConstructer::drawMarker(BWAPI::Unit targetUnit)
{
    BWAPI::Broodwar->registerEvent(
        [targetUnit](BWAPI::Game*) {
            // Blue dot
            BWAPI::Broodwar->drawCircleMap(
                targetUnit->getPosition(), 8,
                BWAPI::Color(0, 0, 255), true);
        }, nullptr, 4);
}


void BuildingConstructer::continueConstruction(ConstructionPO &Job)
{
    Job.updateStatus();
    drawMarker(Job.contractor);
    switch (Job.status) {
    case Dead:
        BWAPI::Unit contractorUnit; // Prevents error C2360.
        contractorUnit = baseCenter->getClosestUnit(IsWorker);
        if (contractorUnit) {
            Job.contractor = contractorUnit;
        }
        break;
    case Mining:
        // Queued gathering just activated without placing the
        // building, because the required minerals were just spent.
        if (!Job.contractor->move(BWAPI::Position(Job.location))) {
            cmdRescuer->append(CmdRescuer::MoveCommand(
                Job.contractor, BWAPI::Position(Job.location)));
        }
        break;
    case Positioning:
        if (Job.contractor->canBuild(Job.constructable, Job.location)) {
            if (!Job.contractor->build(Job.constructable, Job.location)) {
                cmdRescuer->append(CmdRescuer::BuildCommand(
                    Job.contractor, Job.constructable, Job.location));
            }
            else {
                // Because SCV's don't take orders during construction,
                // queue the worker's return back to mining.
                BWAPI::Unit closestMineral = BWAPI::Broodwar->getClosestUnit(
                    BWAPI::Position(Job.location), IsMineralField);
                Job.contractor->gather(closestMineral, true);
            }
        }
    case Constructing:
        break;  // Prevent reissuing commands and the increasing APM.
    default:
        break;
    }
}

void BuildingConstructer::onStart(
    BWAPI::Player Self,
    BWAPI::Unit baseCenter,
    CmdRescuer::Rescuer *cmdRescuer)
{
    this->Self = Self;
    this->baseCenter = baseCenter;
    this->cmdRescuer = cmdRescuer;
}

void BuildingConstructer::constructUnit(BWAPI::UnitType Constructable)
{
    // ToDo: Enable multiple build instances of (Supply, ArmyFacility).
    try {
        continueConstruction(findJob(Constructable));
    }
    catch (NoJob) {
        if (Self->minerals() >= Constructable.mineralPrice() - 24) {
            beginConstruction(Constructable);
        }
    }
}

void BuildingConstructer::addProduct(BWAPI::Unit Product)
{
    // Because initialization of the Spawning pool happens only once
    // it is not included as a product. Fix is outside this scope.
    try {
        ConstructionPO &Job = findJob(Product->getType());
        Job.product = Product;
    }
    catch (NoJob) {
        BWAPI::Broodwar->sendText(
            "Catch: missing job containing %s!", Product->getType().c_str());
    }
}

void BuildingConstructer::removeConstruction(BWAPI::Unit Product)
{
    try {
        constructionJobs.erase(findJob(Product));
    }
    catch (NoJob)
    {
        BWAPI::Broodwar->sendText("Catch: missing job of completed product!");
    }
}

void BuildingConstructer::displayStatus(int &row)
{
    row += 10;
    BWAPI::Broodwar->drawTextScreen(3, row,
        "%d construction jobs", constructionJobs.size());
    for (ConstructionPO Job: constructionJobs) {
        row += 10;
        BWAPI::Broodwar->drawTextScreen(3, row,
            "Constructable: %s, Contractor ID: %d, Product %d, Status: %d.",
            Job.constructable.c_str(), Job.contractor->getID(),
            (Job.product ? Job.product->getID() : 0), 
            Job.status);
    }
    row += 5;
}

#endif
