//
//  Scheduler.cpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/20/24.
//


#include "Scheduler.hpp"
#include <thread>
#include <chrono>
#include <map>



static bool migrating = false;
static unsigned active_machines = 16;
bool is_initializing = true;

std::map<MachineId_t, std::vector<VMInfo_t>> machine_to_vms;

// Add a VM to a machine
void addVMToMachine(MachineInfo_t& machine, const VMInfo_t& vm) {
    machine_to_vms[machine.machine_id].push_back(vm);
    machine.active_vms++;  // Update active VM count
}

// Retrieve VMs for a specific machine
std::vector<VMInfo_t>& getVMsForMachine(const MachineInfo_t& machine) {
    return machine_to_vms[machine.machine_id];
}

VMId_t getBestVM(TaskInfo_t task_info, const std::vector<VMId_t>& vms){

    // chatGPT helped me write this!
    SLAType_t sla = task_info.required_sla;

    VMId_t best_vm = -1;
    size_t best_vm_load = (sla == SLA0) ? std::numeric_limits<size_t>::max() : 0;

    VMType_t vmType = RequiredVMType(task_info.task_id);
    CPUType_t cpuType = RequiredCPUType(task_info.task_id);
    unsigned memory = GetTaskMemory(task_info.task_id);

    

    for (VMId_t vm : vms) {
        VMInfo_t vm_info = VM_GetInfo(vm);
        MachineInfo_t machine_info = Machine_GetInfo(vm_info.machine_id);

        

         cout << "VM " << vm
       << ": active_tasks = [";
     for (const auto& task : vm_info.active_tasks) {
         cout << task << " ";
       }
        cout << "], memory_used = " << machine_info.memory_used
        << "/" << machine_info.memory_size
        << ", type = " << vm_info.vm_type
         << ", machine = " << machine_info.machine_id << endl;
          

        if (vm_info.vm_type == vmType &&
           machine_info.cpu == cpuType &&
           machine_info.memory_size - machine_info.memory_used >= memory) {
            size_t load = vm_info.active_tasks.size();
            if ((sla == SLA0 && load < best_vm_load) ||
                (sla != SLA0 && load > best_vm_load)) {
                best_vm = vm;
                best_vm_load = load;
            }
        }
        
    }
   

    return best_vm;

}


void Scheduler::Init() {

 active_machines = Machine_GetTotal();
 int tasks = GetNumTasks();

 cout << "total number of tasks: " << tasks << endl;
 cout << "scheduler initialized " << endl;
  
  // Start with a limited number of active machines
  unsigned initial_active_machines = active_machines / 2; // Half of active_machines
  for (unsigned i = 0; i < initial_active_machines; i++) {

      MachineInfo_t machine = Machine_GetInfo(MachineId_t(i));

      machines.push_back(MachineId_t(i));
      cout << "about to create 4 VMs for machine " << i << endl;
      for(unsigned j = 0; j < machine.num_cpus/2; j++){

        cout << "creating a VM" <<endl;

        VMId_t newVm = VM_Create(LINUX, machine.cpu);
        VMInfo_t newVMinfo = VM_GetInfo(newVm);
        vms.push_back(newVm);

        cout << "attaching a VM" <<endl;

        VM_Attach(newVm, machines[i]);
        addVMToMachine(machine, newVMinfo);

      }
      
      
      cout << "i = " << i + 1 << endl;
      cout << "num cpus: " << machine.num_cpus << endl;
      cout << "machine id: " << machine.machine_id << endl;
      cout << "active tasks: " << machine.active_tasks << endl;
      cout << "active VMs: " << machine.active_vms << endl;
      cout << "gpu boolean: " << machine.gpus << endl;
      cout << "energy consumed:  " << machine.energy_consumed << endl;
      cout << "S state:  " << machine.s_state << endl;
      cout << "P state:  " << machine.p_state << endl;

      is_initializing = false;
   
    
  }



  // Set the remaining machines to low-power state (S5)
  for (unsigned i = initial_active_machines; i < Machine_GetTotal(); i++) {
      machines.push_back(MachineId_t(i));
      Machine_SetState(MachineId_t(i), S5);


       MachineInfo_t machine = Machine_GetInfo(MachineId_t(i));


      cout << "i = " << i + 1 << endl;
      cout << "num cpus: " << machine.num_cpus << endl;
      cout << "machine id: " << machine.machine_id << endl;
      cout << "active tasks: " << machine.active_tasks << endl;
      cout << "active VMs: " << machine.active_vms << endl;
      cout << "gpu boolean: " << machine.gpus << endl;
      cout << "energy consumed:  " << machine.energy_consumed << endl;
      cout << "S state:  " << machine.s_state << endl;
      cout << "P state:  " << machine.p_state << endl;


      cout << "S state:  " << machine.s_state << endl;
  }




  SimOutput("Scheduler::Init(): Energy-efficient initialization complete.", 3);
}




void Scheduler::MigrationComplete(Time_t time, VMId_t vm_id) {
  // Retrieve information about the migrated VM
  cout << "migration complete " << endl;
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
   cout << "entered newtask " << endl;
   VMType_t vmType = RequiredVMType(task_id);
   CPUType_t cpuType = RequiredCPUType(task_id);
   unsigned memory = GetTaskMemory(task_id);

   TaskInfo_t taskInfo = GetTaskInfo(task_id);


   VMId_t selectedVm = -1;  // Initialize to an invalid value
   size_t minTasks = 100000;
   MachineInfo_t selectedMachine = Machine_GetInfo(0);

   selectedVm = getBestVM(taskInfo, vms);

   /*


   // Attempt to find an existing VM that can handle the task
   for (size_t i = 0; i < vms.size(); i++) {
       VMInfo_t vmInfo = VM_GetInfo(vms[i]);
  
       MachineInfo_t machineInfo = Machine_GetInfo(vmInfo.machine_id);

        cout << "VM " << vms[i]
       << ": active_tasks = [";
     for (const auto& task : vmInfo.active_tasks) {
         cout << task << " ";
       }
        cout << "], memory_used = " << machineInfo.memory_used
        << "/" << machineInfo.memory_size
        << ", type = " << vmInfo.vm_type
         << ", machine = " << machineInfo.machine_id << endl;


       if (vmInfo.vm_type == vmType &&
           machineInfo.cpu == cpuType &&
           machineInfo.memory_size - machineInfo.memory_used >= memory) {

           size_t activeTasks = vmInfo.active_tasks.size();

           if(activeTasks < minTasks){

               selectedVm = vms[i];
               selectedMachine = machineInfo;
               minTasks = activeTasks;

           }

           cout << "min tasks is now " << minTasks << endl;
           cout << "selected VM is now " << selectedVm << endl;
          
       }
   }

   */


   if(selectedVm != -1){
           MachineInfo_t machineInfo = Machine_GetInfo(VM_GetInfo(selectedVm).machine_id);
           cout << "before adding Task " << task_id << ": memory used = "
             << machineInfo.memory_used << "/" << machineInfo.memory_size << endl;

           if(taskInfo.required_sla == SLA0){
               

               VM_AddTask(selectedVm, task_id, HIGH_PRIORITY);

           }

           else if(taskInfo.required_sla == SLA1){

            VM_AddTask(selectedVm, task_id, MID_PRIORITY);

           }

           else{

              VM_AddTask(selectedVm, task_id, LOW_PRIORITY);

           }

           cout << "task " << task_id << " with the SLA of " << taskInfo.required_sla << " was placed in VM " << selectedVm << " due to load logic " << endl;

           
           //machineInfo.memory_size -= memory;

           cout << "NewTask(): Task " << to_string(task_id) << " added to VM " << to_string(selectedVm) << " at machine " << selectedMachine.machine_id << endl;

           cout << "after adding Task " << task_id << ": memory used = "
             << machineInfo.memory_used << "/" << machineInfo.memory_size << endl;
           return;
       }


   // No suitable VM found: Activate a new machine and create a VM
   for (auto &machine : machines) {
       MachineInfo_t machineInfo = Machine_GetInfo(machine);

    

       if (machineInfo.s_state == S5) { // Machine is in sleep mode
           cout << "NewTask(): Machine " << to_string(machine) << " is in sleep mode. Attempting to power on." <<endl;
           Machine_SetState(machine, S0); // Power on the machine


           // Wait until the machine is fully powered on
           while (machineInfo.s_state != S0) {
               std::this_thread::sleep_for(std::chrono::milliseconds(50));
               machineInfo = Machine_GetInfo(machine);
           }
           cout << "NewTask(): Machine " << to_string(machine) << " is now active." <<endl;
       }


       if (machineInfo.s_state == S0) { // Confirm the machine is active
           VMId_t newVm = VM_Create(vmType, cpuType);
           try {
               VM_Attach(newVm, machine); // Attach the new VM to the machine
               vms.push_back(newVm);
    
               VM_AddTask(newVm, task_id, HIGH_PRIORITY);
               cout <<"NewTask(): Task " << to_string(task_id) << " added to new VM " << to_string(newVm) +
                         " on machine " << to_string(machine) << endl;
               return;
           } catch (const std::exception &e) {
               cout << "NewTask(): Error during VM_Attach: " << string(e.what()) << endl;
               throw;
           }
       }
   }


   // If all else fails, log an error
   SimOutput("NewTask(): Error - Task " + to_string(task_id) + " could not be placed!", 0);
}




void Scheduler::PeriodicCheck(Time_t now) {

    /*

  cout << "entered periodic check" << endl;

    if (is_initializing) {
        cout << "initializing, so no periodic check at this time" << endl;
        return;
    }

    for (auto &machine : machines) {
        MachineInfo_t machineInfo = Machine_GetInfo(machine);

        // Skip central machines
        if (machineInfo.machine_id == 0 || machineInfo.machine_id == 16) {
            continue;
        }

        // Check for idle VMs and perform migration
        for (auto &vm : getVMsForMachine(machineInfo)) {
            if (vm.active_tasks.size() == 0) {
                MachineId_t target_machine = (machine <= 15) ? 0 : 16;
                VM_Migrate(vm.vm_id, target_machine);
                cout << "migrated a VM from machine " << machine << " to machine " << target_machine << endl;
            }
        }

        // Shut down the machine if no active VMs remain
        if (machineInfo.active_vms == 0) {
            Machine_SetState(machine, S5); // S5 = Powered down state
            SimOutput("PeriodicCheck(): Shutting down machine " + to_string(machine), 3);
        }
    }

 */


}
void Scheduler::Shutdown(Time_t time) {
   cout << "shutting down... " << endl;
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
   cout << "task id " << task_id << " complete " << endl;
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
   cout << "HandleNewTask(): Received new task "
         << task_id
         << " at time "
         << time
         << endl;
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
  bool allWorkDone = true;
 
  static unsigned counts = 0;
  counts++;
  /*
  if(counts == 100) {
      migrating = true;
      VM_Migrate(1, 9);
  }
  */

}




void SimulationComplete(Time_t time) {
  // This function is called before the simulation terminates Add whatever you feel like.
  cout << "SLA violation report" << endl;
  cout << "SLA0: " << GetSLAReport(SLA0) << "%" << endl;
  cout << "SLA1: " << GetSLAReport(SLA1) << "%" << endl;
  cout << "SLA2: " << GetSLAReport(SLA2) << "%" << endl;     // SLA3 do not have SLA violation issues
  cout << "Total Energy " << Machine_GetClusterEnergy() << "KW-Hour" << endl;
  cout << "Simulation run finished in " << double(time)/1000000 << " seconds" << endl;
   SimOutput("SimulationComplete(): Simulation finished at time " + to_string(time), 4);
  
   Scheduler.Shutdown(time);
}


void StateChangeComplete(Time_t time, MachineId_t machine_id) {
   SimOutput("StateChangeComplete(): Machine " + to_string(machine_id) +
             " state change completed at time " + to_string(time), 4);
}


void SLAWarning(Time_t time, TaskId_t task_id) {
   SimOutput("SLAWarning(): Task " + to_string(task_id) +
             " encountered an SLA warning at time " + to_string(time), 4);
}