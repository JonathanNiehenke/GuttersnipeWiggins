#pragma once
#include "Race.h"


class TerranRace : public Race
{
    private:
    public:
        TerranRace() : Race(BWAPI::UnitTypes::Enum::Terran_Marine) {}
};
