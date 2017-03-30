#ifndef CMDRESCUER_CPP
#define CMDRESCUER_CPP
#include "CmdRescuer.h"

void CmdRescuer::Rescuer::rescue()
{
    // Curious if this can be done without the lambda.
    Commands.erase(remove_if(
        Commands.begin(), Commands.end(), [](Command Cmd) {
            BWAPI::Broodwar << "Recalling failed Command" << std::endl;
            return Cmd.execute(); }),
        Commands.end());
}

CmdRescuer::BuildCommand::BuildCommand(
    BWAPI::Unit contractor,
    BWAPI::UnitType constructable,
    BWAPI::TilePosition location)
{
    this->contractor = contractor;
    this->constructable = constructable;
    this->location = location;
}

CmdRescuer::TrainCommand::TrainCommand(
    BWAPI::Unit trainer,
    BWAPI::UnitType trainee)
{
    this->trainer = trainer;
    this->trainee = trainee;
}

CmdRescuer::MoveCommand::MoveCommand(
    BWAPI::Unit runner, BWAPI::Position toThere, bool queue)
{
    this->runner = runner;
    this->toThere = toThere;
    this->queue = queue;
}

CmdRescuer::GatherCommand::GatherCommand(
    BWAPI::Unit miner, BWAPI::Unit mineral, bool queue)
{
    this->miner = miner;
    this->mineral = mineral;
    this->queue = queue;
}

#endif
