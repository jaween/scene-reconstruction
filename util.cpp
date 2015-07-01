#include <ctime>
#include <iostream>
#include <map>

#include "util.hpp"

namespace Util
{
        std::map<std::string, std::clock_t> debugTimerStartTicks;

        void startDebugTimer(std::string tag)
        {
                // Starts a timer
                debugTimerStartTicks[tag] = std::clock();
        }

        void endDebugTimer(std::string tag)
        {
                // Displays the timer value
                if (DEBUG_MODE)
                {
                        if (debugTimerStartTicks.find(tag) == debugTimerStartTicks.end())
                        {
                                // No timer
                                std::cerr << "No timer with tag '" << tag << "'" << std::endl;
                        }
                        else
                        {
                                // Prints the time taken
                                std::clock_t timerEndTicks = std::clock();
                                std::clock_t clockTicksTaken = timerEndTicks - debugTimerStartTicks[tag];
                                double timeInSeconds = clockTicksTaken / (double) CLOCKS_PER_SEC;
                                double timeInMillis = timeInSeconds * 1000.0;
                                std::cout << tag << " took " << timeInMillis  << "ms" << std::endl;

                                // Removes the timer from the map
                                debugTimerStartTicks.erase(tag);
                        }
                }
        }
}
