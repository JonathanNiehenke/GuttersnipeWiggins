# GuttersnipeWiggins

StarCraft BW random race bot with an *overwhelming* strategy. Written in C++
    using only the [BWAPI](https://github.com/bwapi/bwapi).

### Developed by:
Jonathan Niehenke

### Contained files:

- GuttersnipeWiggins.vcxproj: Project file.
- GuttersnipeWiggins.h|cpp: Main
- Race.h|cpp: Base class for derived races.
- ProtossRace.h|cpp: Protoss race.
- TerranRace.h|cpp: Terran race.
- ZergRace.h|cpp: Zerg race.
- Cartographer.h|cpp: Log resource clusters and enemy building positions.
- ResourceLocations.h|cpp: Determine resource clusters and base locations.
- EcoBaseManager.h|cpp: Manage gathering workders.
- BuildingConstructer.h|cpp: Manage building jobs.
- SquadCommander.h|cpp: Manage army squads and micro.
- UnitTrainer.h|cpp: Manages unit production.
- CmdRescuer.h|cpp: Redundant BWAPI command execution.
- Utils.h|cpp: Compare and sort distance between positions, units, and more.
- LICENSE.md - The License.
- README.md - This file.

### Requires:

- Starcraft BW 1.16.1
- BWAPI: [Project](https://github.com/bwapi/bwapi)

### How To:

1. Install: [Visual Studio 2013](https://msdn.microsoft.com/en-us/library/dd831853(v=vs.120).aspx)
2. Install: [Starcraft Brood War](https://us.battle.net/shop/en/product/starcraft)
3. Install: [Patch 1.16.1](http://www.teamliquid.net/forum/brood-war/82826-starcraft-patch-1160)
            [Downgrade](https://us.battle.net/forums/en/starcraft/topic/20754525604)
4. Install: [BWAPI](https://github.com/bwapi/bwapi)
5. Build: `MSBuild GuttersnipeWiggins.vcxproj /p:BWAPI_DIR=[pathToBWAPI]`
6. Copy: `Debug\GuttersnipeWiggins.dll` to `Starcraft\bwapi-data\AI`
7. Execute: `BWAPI\Chaoslauncher\Chaoslauncher.exe`
8. Enable BWAPI injector: Check and select BWAPI injector [DEBUG]
9. Configure injector: Press config and replace line starting with `ai_dbg =`
    to `ai_dbg = bwapi-data/AI/GuttersnipeWiggins.dll`
10. Press start and begin a game.
