#include "flexseastack/stack_util.h"
#include "flexseastack/flexsea_config.h"

#include <thread>
static std::thread *periodicThread = nullptr;

CommManager* setupFlexsea()
{
    initFlexSEAStack_minimalist(FLEXSEA_PLAN_1);

    CommManager* cm = new CommManager();
    periodicThread = new std::thread(&CommManager::runPeriodicTask, cm);
    return cm;
}

void cleanupFlexsea(CommManager* cm)
{
    if(!cm) return;
    cm->quitPeriodicTask();
    delete cm;

    if(!periodicThread) return;
    periodicThread->join();
    delete periodicThread;
    periodicThread=nullptr;

}
