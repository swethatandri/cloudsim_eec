//
//  Scheduler.cpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/20/24.
//


#include "Scheduler.hpp"
#include <thread>
#include <chrono>


static bool migrating = false;
static unsigned active_machines = 16;


void Scheduler::Init() {
   SimOutput("Scheduler::Init(): Total number of machines is " + to_string(Machine_GetTotal()), 3);
   SimOutput("Scheduler::Init(): Initializing scheduler", 1);


   // Start with a limited number of active machines
   unsigned initial_active_machines = active_machines / 2; // Half of active_machines
   for (unsigned i = 0; i < initial_active_machines; i++) {
       vms.push_back(VM_Create(LINUX, X86));
       machines.push_back(MachineId_t(i));
       VM_Attach(vms[i], machines[i]);
   }


   // Set the remaining machines to low-power state (S5)
   for (unsigned i = initial_active_machines; i < Machine_GetTotal(); i++) {
       machines.push_back(MachineId_t(i));
       Machine_SetState(MachineId_t(i), S5);
   }


   SimOutput("Scheduler::Init(): Energy-efficient initialization complete.", 3);
}


void Scheduler::MigrationComplete(Time_t time, VMId_t vm_id) {
   // Retrieve information about the migrated VM
   VMInfo_t vmInfo = VM_GetInfo(vm_id);
   MachineId_t newMachineId = vmInfo.machine_id;


   // Update any internal data structures to reflect the VM's new location
   for (size_t i = 0; i < vms.size(); i++) {
       if (vms[i] == vm_id) {
           SimOutput("MigrationComplete(): VM " + to_string(vm_id) +
                     " successfully migrated to Machine " + to_string(newMachineId), 3);
           break;
       }
   }


   // Mark the VM as available to receive new tasks
   SimOutput("MigrationComplete(): VM " + to_string(vm_id) + " is now available for new tasks.", 3);
}


void Scheduler::NewTask(Time_t now, TaskId_t task_id) {
    // Retrieve task requirements
    VMType_t vmType = RequiredVMType(task_id);
    CPUType_t cpuType = RequiredCPUType(task_id);
    unsigned memory = GetTaskMemory(task_id);

    SimOutput("NewTask(): Task " + to_string(task_id) + " arriving at time " + to_string(now), 3);

    // Attempt to find an existing VM that can handle the task
    for (size_t i = 0; i < vms.size(); i++) {
        VMInfo_t vmInfo = VM_GetInfo(vms[i]);
        MachineInfo_t machineInfo = Machine_GetInfo(vmInfo.machine_id);

        if (vmInfo.vm_type == vmType &&
            machineInfo.cpu == cpuType &&
            machineInfo.memory_size - machineInfo.memory_used >= memory) {
            VM_AddTask(vms[i], task_id, HIGH_PRIORITY);
            SimOutput("NewTask(): Task " + to_string(task_id) + " added to VM " + to_string(vms[i]), 3);
            return;
        }
    }

    // No suitable VM found: Activate a new machine and create a VM
    for (auto &machine : machines) {
        MachineInfo_t machineInfo = Machine_GetInfo(machine);

        if (machineInfo.s_state == S5) { // Machine is in sleep mode
            SimOutput("NewTask(): Machine " + to_string(machine) + " is in sleep mode. Attempting to power on.", 3);
            Machine_SetState(machine, S0); // Power on the machine

            // Wait until the machine is fully powered on
            while (machineInfo.s_state != S0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                machineInfo = Machine_GetInfo(machine);
            }
            SimOutput("NewTask(): Machine " + to_string(machine) + " is now active.", 3);
        }

        if (machineInfo.s_state == S0) { // Confirm the machine is active
            VMId_t newVm = VM_Create(vmType, cpuType);
            try {
                VM_Attach(newVm, machine); // Attach the new VM to the machine
                vms.push_back(newVm);
                VM_AddTask(newVm, task_id, HIGH_PRIORITY);
                SimOutput("NewTask(): Task " + to_string(task_id) + " added to new VM " + to_string(newVm) +
                          " on machine " + to_string(machine), 3);
                return;
            } catch (const std::exception &e) {
                SimOutput("NewTask(): Error during VM_Attach: " + string(e.what()), 0);
                throw;
            }
        }
    }

    // If all else fails, log an error
    SimOutput("NewTask(): Error - Task " + to_string(task_id) + " could not be placed!", 0);
}


void Scheduler::PeriodicCheck(Time_t now) {
    SimOutput("PeriodicCheck(): Performing periodic system maintenance at time " + to_string(now), 4);

    for (size_t i = 0; i < vms.size(); i++) {
        VMInfo_t vmInfo = VM_GetInfo(vms[i]);
        MachineInfo_t machineInfo = Machine_GetInfo(vmInfo.machine_id);

        if (machineInfo.active_tasks == 0 && machineInfo.s_state == S0) {
            // Shut down unused machines
            SimOutput("PeriodicCheck(): Shutting down idle machine " + to_string(machineInfo.machine_id), 3);
            Machine_SetState(machineInfo.machine_id, S5);
        }
    }

    // Check for underutilized machines
    for (auto &machine : machines) {
        MachineInfo_t machineInfo = Machine_GetInfo(machine);

        if (machineInfo.active_vms == 0 && machineInfo.s_state == S0) {
            // Shut down unused machine
            SimOutput("PeriodicCheck(): No active VMs on machine " + to_string(machine) + ". Shutting down.", 3);
            Machine_SetState(machine, S5);
        }
    }
}
void Scheduler::Shutdown(Time_t time) {
   SimOutput("SimulationComplete(): Finalizing simulation.", 4);


   cout << "Simulation complete at time " << time << endl;
   cout << "Total Energy Consumed: " << Machine_GetClusterEnergy() << " KW-Hour" << endl;
   cout << "SLA Compliance Rates:" << endl;
   cout << "SLA0: " << GetSLAReport(SLA0) << "%" << endl;
   cout << "SLA1: " << GetSLAReport(SLA1) << "%" << endl;
   cout << "SLA2: " << GetSLAReport(SLA2) << "%" << endl;


   // Shut down all remaining VMs
   for (auto &vm : vms) {
       VM_Shutdown(vm);
   }


   SimOutput("Shutdown(): All resources deallocated. Goodbye!", 4);
}


void Scheduler::TaskComplete(Time_t now, TaskId_t task_id) {
   // Do any bookkeeping necessary for the data structures
   // Decide if a machine is to be turned off, slowed down, or VMs to be migrated according to your policy
   // This is an opportunity to make any adjustments to optimize performance/energy
   SimOutput("Scheduler::TaskComplete(): Task " + to_string(task_id) + " is complete at " + to_string(now), 4);
}


// Public interface below


static Scheduler Scheduler;


void InitScheduler() {
   SimOutput("InitScheduler(): Initializing scheduler", 4);
   Scheduler.Init();
}


void HandleNewTask(Time_t time, TaskId_t task_id) {
   SimOutput("HandleNewTask(): Received new task " + to_string(task_id) + " at time " + to_string(time), 4);
   Scheduler.NewTask(time, task_id);
}


void HandleTaskCompletion(Time_t time, TaskId_t task_id) {
   SimOutput("HandleTaskCompletion(): Task " + to_string(task_id) + " completed at time " + to_string(time), 4);
   Scheduler.TaskComplete(time, task_id);
}


void MemoryWarning(Time_t time, MachineId_t machine_id) {
   // The simulator is alerting you that machine identified by machine_id is overcommitted
   SimOutput("MemoryWarning(): Overflow at " + to_string(machine_id) + " was detected at time " + to_string(time), 0);
}


void MigrationDone(Time_t time, VMId_t vm_id) {
   // The function is called on to alert you that migration is complete
   SimOutput("MigrationDone(): Migration of VM " + to_string(vm_id) + " was completed at time " + to_string(time), 4);
   Scheduler.MigrationComplete(time, vm_id);
   migrating = false;
}


void SchedulerCheck(Time_t time) {
   // This function is called periodically by the simulator, no specific event
   SimOutput("SchedulerCheck(): SchedulerCheck() called at " + to_string(time), 4);
   Scheduler.PeriodicCheck(time);
   static unsigned counts = 0;
   counts++;
   if(counts == 10) {
       migrating = true;
       VM_Migrate(1, 9);
   }
}


void SimulationComplete(Time_t time) {
   // This function is called before the simulation terminates Add whatever you feel like.
   cout << "SLA violation report" << endl;
   cout << "SLA0: " << GetSLAReport(SLA0) << "%" << endl;
   cout << "SLA1: " << GetSLAReport(SLA1) << "%" << endl;
}

void StateChangeComplete(Time_t time, MachineId_t machine_id) {
    SimOutput("StateChangeComplete(): Machine " + to_string(machine_id) +
              " state change completed at time " + to_string(time), 4);
}

void SLAWarning(Time_t time, TaskId_t task_id) {
    SimOutput("SLAWarning(): Task " + to_string(task_id) +
              " encountered an SLA warning at time " + to_string(time), 4);
}