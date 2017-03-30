#ifndef CMDRESCUER_H
#define CMDRESCUER_H
#include <BWAPI.h>

// Wish I knew how to merge the declarations with the definitions
// without triggering LNK2005, because separating them reduces its
// readability. Together they are about 100 lines.

namespace CmdRescuer
{

    struct Command
    {
        virtual bool execute() { return true; }
    };

    // To store and recall the occasional failed Broodwar command.
    class Rescuer
    {
        std::vector<Command> Commands;
        public:
            void append(Command Cmd) { Commands.push_back(Cmd); }
            void rescue();
    };

    class BuildCommand : public Command
    {
        BWAPI::Unit contractor;
        BWAPI::UnitType constructable;
        BWAPI::TilePosition location;
        public:
            BuildCommand(BWAPI::Unit, BWAPI::UnitType, BWAPI::TilePosition);
            bool execute()
                { return contractor->build(constructable, location); }
    };

    class TrainCommand : public Command
    {
        BWAPI::Unit trainer;
        BWAPI::UnitType trainee;
        public:
            TrainCommand(BWAPI::Unit trainer, BWAPI::UnitType trainee);
            bool execute() { return trainer->train(trainee); }
    };

    class MoveCommand : public Command
    {
        BWAPI::Unit runner;
        BWAPI::Position toThere;
        bool queue;
        public:
            MoveCommand(BWAPI::Unit, BWAPI::Position, bool queue=false);
            bool execute() { return runner->move(toThere, queue); }
    };

    class GatherCommand : public Command
    {
        BWAPI::Unit miner;
        BWAPI::Unit mineral;
        bool queue;
        public:
            GatherCommand(BWAPI::Unit, BWAPI::Unit, bool queue=false);
            bool execute() { return miner->gather(mineral, queue); }
    };

}

#endif
