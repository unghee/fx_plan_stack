#include "periodictask.h"
#include <chrono>
#include <thread>
#include <iostream>

void PeriodicTask::runPeriodicTask()
{
    runPeriodicThread = 1;
    std::chrono::high_resolution_clock::time_point nextWake {std::chrono::high_resolution_clock::now()};

    while(runPeriodicThread)
    {
        periodicTask();

        bool goToSleep;
        {
            std::lock_guard<std::mutex> lk(conditionMutex);
            goToSleep = this->goToLongSleep();
        }

        if(!goToSleep && taskPeriod > 0)
        {
            nextWake += std::chrono::milliseconds{taskPeriod};
            std::this_thread::sleep_until(nextWake);
        }
        else
        {
            std::unique_lock<std::mutex> lk(conditionMutex);
//            std::cout << "Thread: " << std::this_thread::get_id() << " going into long sleep" << std::endl;
            wakeCV.wait(lk, [this]{return (!runPeriodicThread || this->wakeFromLongSleep());});
            nextWake = std::chrono::high_resolution_clock::now();
//            std::cout << "Thread: " << std::this_thread::get_id() << " woke after long sleep" << std::endl;
        }
    }

    return;
}
void PeriodicTask::quitPeriodicTask()
{
    {
        std::lock_guard<std::mutex> l(conditionMutex);
        runPeriodicThread = 0;
    }
    wakeCV.notify_all();
}
