#include <ctime>
#include <iostream>

#include "util.hpp"

namespace Util
{
	std::clock_t debugTimerStartTicks;

	void startDebugTimer()
	{
		debugTimerStartTicks = std::clock();
	}

	void endDebugTimer(std::string function)
	{
		// Displays the time taken to execute a function
		if (DEBUG_MODE)
		{
			std::clock_t timerEndTicks = std::clock();
			std::clock_t clockTicksTaken = timerEndTicks - debugTimerStartTicks;
			double timeInSeconds = clockTicksTaken / (double) CLOCKS_PER_SEC;
			double timeInMillis = timeInSeconds * 1000.0;
			std::cout << function << " took " << timeInMillis  << "ms" << std::endl;
		}
	}
}
