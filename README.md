GITHUB REPO: https://github.com/swethatandri/cloudsim_eec/tree/main


Structure of files:

Scheduler.cpp runs the algorithm (queue based, with load-based scheduling) the .h files remain unchanged.

result.txt shows my result for running Hour, same as the BEST file

Structure of Scheduler.cpp Initialization (Init): Allocates virtual machines (VMs) to half of the available machines during startup. Ensures a balanced initial distribution of resources to handle incoming tasks.

Selecting the Best VM (getBestVM): Chooses the most suitable VM for a task based on load and SLA requirements. Considers factors such as active tasks, available memory, and VM compatibility with the task's specifications.

Handling New Tasks (NewTask): Invokes getBestVM to identify an ideal VM for the task. If a suitable VM is found but the associated machine is overloaded, the task is added to the machine’s local queue. If no suitable VM is available, a new machine is "powered on" (moved to the powered_on list), VMs are added to it, and the task is assigned to one of the new VMs.

Periodic Checks (PeriodicCheck): Continuously monitors the system to ensure tasks in machine queues are processed when resources become available. Handles reassigning tasks from queues to VMs as spaces open up.