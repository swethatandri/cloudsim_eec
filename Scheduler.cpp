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
#include <algorithm>
#include <set>
#include <ctime>
#include <queue>
#include <unordered_map>

static bool migrating = false;
static unsigned active_machines = 16;
bool is_initializing = true;

static int total_tasks = 0;
static int completed_tasks = 0;

std::map<MachineId_t, std::queue<TaskId_t>> task_queues;


std::map<MachineId_t, std::vector<VMInfo_t>> machine_to_vms;
static std::set<VMId_t> migrating_vms;

static std::set<MachineId_t> transitioning_machines;

std::unordered_map<TaskId_t, VMId_t> task_to_vm_map;


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

  //cout << "entered get Best VM" << endl;
  SLAType_t sla = task_info.required_sla;


  VMId_t best_vm = -1;
  size_t best_vm_load = (sla == SLA0) ? std::numeric_limits<size_t>::max() : 0;

  VMType_t vmType = RequiredVMType(task_info.task_id);
  CPUType_t cpuType = RequiredCPUType(task_info.task_id);
  unsigned memory = GetTaskMemory(task_info.task_id);


  for (VMId_t vm : vms) {
      VMInfo_t vm_info = VM_GetInfo(vm);
      MachineInfo_t machine_info = Machine_GetInfo(vm_info.machine_id);


   if (!(migrating_vms.find(vm) != migrating_vms.end())) {


      if (vm_info.active_tasks.size() <=10 && vm_info.vm_type == vmType &&
         machine_info.cpu == cpuType &&
         machine_info.memory_size - machine_info.memory_used >= memory && task_queues[machine_info.machine_id].size() < 3) {

      
          size_t load = vm_info.active_tasks.size();
         
          if ((sla == SLA0 && load < best_vm_load) ||
              (sla != SLA0 && load >= best_vm_load)) {
              best_vm = vm;
              //cout << "best VM is: " << vm << endl;
              best_vm_load = load;
          }
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


   unsigned initial_active_machines = active_machines / 2; // Half of active_machines
   for (unsigned i = 0; i < initial_active_machines; i++) {
       MachineInfo_t machine = Machine_GetInfo(MachineId_t(i));
       machines.push_back(MachineId_t(i));
      
       //cout << "about to create 4 VMs for machine " << i << endl;
       for (unsigned j = 0; j < machine.num_cpus / 2; j++) {
           //cout << "creating a VM" << endl;
           VMId_t newVm = VM_Create(LINUX, machine.cpu);
           VMInfo_t newVMinfo = VM_GetInfo(newVm);
           vms.push_back(newVm);


           //cout << "attaching a VM" << endl;
           VM_Attach(newVm, machines[i]);
           machine_to_vms[machine.machine_id].push_back(newVMinfo);
           addVMToMachine(machine, newVMinfo);
       }
      
       //cout << "Machine " << i << " initialized with VMs." << endl;
       //cout << "S state: " << machine.s_state << endl;
   }


   for (unsigned i = initial_active_machines; i < Machine_GetTotal(); i++) {
    //cout << "not using machine: " << i << endl;
   MachineId_t machine_id = MachineId_t(i);


   // Skip machines already transitioning
   if (transitioning_machines.find(machine_id) != transitioning_machines.end()) {
       //cout << "Machine " << machine_id << " is already transitioning. Skipping." << endl;
       continue;
   }


   // Add to transitioning machines and set state to S5
   transitioning_machines.insert(machine_id);
   //Machine_SetState(machine_id, S5);


   //cout << "Simulating state change for machine " << machine_id << endl;
   std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate delay
}


   SimOutput("Scheduler::Init(): Energy-efficient initialization complete.", 3);
}


void Scheduler::MigrationComplete(Time_t time, VMId_t vm_id) {
//cout << "MigrationComplete(): Migration of VM " << vm_id << " completed at time " << time << endl;


  // Retrieve the VM and its new machine information
  VMInfo_t vmInfo = VM_GetInfo(vm_id);
  MachineId_t newMachineId = vmInfo.machine_id;



  // Find the old machine
  MachineId_t oldMachineId = -1;
  for (auto &[machine, vms] : machine_to_vms) {
      auto it = std::find_if(vms.begin(), vms.end(), [vm_id](const VMInfo_t &vm) {
          return vm.vm_id == vm_id;
      });
      if (it != vms.end()) {
          oldMachineId = machine;
          vms.erase(it);
          break;
      }
  }



  if (oldMachineId != -1) {
      // Update memory usage for the old machine
      MachineInfo_t oldMachineInfo = Machine_GetInfo(oldMachineId);
      for (const auto &task : vmInfo.active_tasks) {
          TaskInfo_t taskInfo = GetTaskInfo(task);
          oldMachineInfo.memory_used -= taskInfo.required_memory;
      }
      cout << "MigrationComplete(): Updated memory for old machine " << oldMachineId
           << " after migration. New memory used: " << oldMachineInfo.memory_used << "/" << oldMachineInfo.memory_size << endl;
  } else {
      cout << "MigrationComplete(): Error - Old machine not found for VM " << vm_id << endl;
  }




  // Update memory usage for the new machine
  MachineInfo_t newMachineInfo = Machine_GetInfo(newMachineId);
  for (const auto &task : vmInfo.active_tasks) {
      TaskInfo_t taskInfo = GetTaskInfo(task);
      newMachineInfo.memory_used += taskInfo.required_memory;
  }




  // Add the VM to the new machine's VM list
  machine_to_vms[newMachineId].push_back(vmInfo);
  migrating_vms.erase(vm_id);




  cout << "MigrationComplete(): Updated memory for new machine " << newMachineId
       << ". New memory used: " << newMachineInfo.memory_used << "/" << newMachineInfo.memory_size << endl;




  // Mark the VM as available for new tasks
  SimOutput("MigrationComplete(): VM " + to_string(vm_id) + " is now available for new tasks.", 3);
}



void Scheduler::NewTask(Time_t now, TaskId_t task_id) {
    //cout << "NewTask(): Received task " << task_id << " at time " << now << endl;

    TaskInfo_t task_info = GetTaskInfo(task_id);
    VMId_t selected_vm = getBestVM(task_info, vms);
    //cout << "NewTask(): Best VM selected: " << selected_vm << endl;

    if (selected_vm != -1) {
        MachineInfo_t machine_info = Machine_GetInfo(VM_GetInfo(selected_vm).machine_id);

        if (machine_info.active_tasks < machine_info.num_cpus) {
            try {
                VM_AddTask(selected_vm, task_id, HIGH_PRIORITY);
                task_to_vm_map[task_id] = selected_vm;
                machine_info.active_tasks++;
            
            } catch (const std::exception &e) {
                cout << "NewTask(): Error assigning task " << task_id 
                     << " to VM " << selected_vm << ": " << e.what() << endl;
            }
        } else {
          
            task_queues[machine_info.machine_id].push(task_id);
        }
    } else {
       

        MachineId_t selected_machine = -1;

        for (auto &machine : transitioning_machines) {
            MachineInfo_t machine_info = Machine_GetInfo(machine);

            if ((machine_info.s_state == S5 || machine_info.s_state == S0)) {
            

                if(machine_info.cpu == task_info.required_cpu){

                transitioning_machines.erase(machine);
                machines.push_back(machine);
                Machine_SetState(machine, S0);
                selected_machine = machine;
                break;

                }
            }
        }

        if (selected_machine == -1) {
            
            return;
        }

       
        while (Machine_GetInfo(selected_machine).s_state != S0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }


        MachineInfo_t machine_info = Machine_GetInfo(selected_machine);
        for (unsigned j = 0; j < machine_info.num_cpus / 2; j++) {
            try {
                VMId_t new_vm = VM_Create(task_info.required_vm, machine_info.cpu);
                VMInfo_t new_vm_info = VM_GetInfo(new_vm);
                vms.push_back(new_vm);

                VM_Attach(new_vm, selected_machine);
                addVMToMachine(machine_info, new_vm_info);

        
            } catch (const std::exception &e) {
                cout << "NewTask(): Error creating VM for machine " << selected_machine 
                     << ": " << e.what() << endl;
            }
        }

        bool task_assigned = false;
        for (auto &vm : machine_to_vms[selected_machine]) {
            VMInfo_t vm_info = VM_GetInfo(vm.vm_id);
            if (vm_info.active_tasks.size() < 10) {
                try {
                    VM_AddTask(vm.vm_id, task_id, HIGH_PRIORITY);
                    task_to_vm_map[task_id] = vm.vm_id;
                  
                    task_assigned = true;
                    break;
                } catch (const std::exception &e) {
                    cout << "NewTask(): Error assigning task " << task_id 
                         << " to VM " << vm.vm_id << ": " << e.what() << endl;
                }
            }
        }

        if (!task_assigned) {
           
            task_queues[selected_machine].push(task_id);
        }
    }
}



 

void Scheduler::PeriodicCheck(Time_t now) {
    // << "Entered PeriodicCheck at time " << now << endl;

    for (auto &machine : machines) {
        MachineInfo_t machine_info = Machine_GetInfo(machine);

        if (machine_info.active_tasks < machine_info.num_cpus && !task_queues[machine].empty()) {
            TaskId_t next_task = task_queues[machine].front();
            task_queues[machine].pop();

            for (auto &vm : getVMsForMachine(machine_info)) {
                if (VM_GetInfo(vm.vm_id).active_tasks.size() < 10) {
                    VM_AddTask(vm.vm_id, next_task, MID_PRIORITY);
                    machine_info.active_tasks++;
                    //cout << "Task " << next_task << " dequeued and assigned to VM " << vm.vm_id << endl;
                    break;
                }
            }
        }
    }
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
    //cout << "TaskComplete(): Task " << task_id << " complete at " << now << endl;
    completed_tasks++;
    //cout << "completed tasks: " << completed_tasks;

    // Find the VM associated with the task
    if (task_to_vm_map.find(task_id) == task_to_vm_map.end()) {
        cout << "Error: Task " << task_id << " not found in task_to_vm_map." << endl;
        return;
    }

    VMId_t vm_id = task_to_vm_map[task_id];
    VMInfo_t vm_info = VM_GetInfo(vm_id);
    MachineId_t machine_id = vm_info.machine_id;
    MachineInfo_t machine_info = Machine_GetInfo(machine_id);

    // Remove task from VM's active task list
    auto &active_tasks = vm_info.active_tasks;
   
    // Check if VM is now idle
    if (active_tasks.empty()) {
        //cout << "VM " << vm_id << " is now idle." << endl;

    
    }

    // Check if machine has tasks queued and assign to an available VM
    if (!task_queues[machine_id].empty() && machine_info.active_tasks < machine_info.num_cpus) {
        TaskId_t next_task = task_queues[machine_id].front();
        task_queues[machine_id].pop();
        VM_AddTask(vm_id, next_task, MID_PRIORITY);
        active_tasks.push_back(next_task);
        task_to_vm_map[next_task] = vm_id;
        machine_info.active_tasks++;
       // cout << "Task " << next_task << " dequeued and assigned to VM " << vm_id << endl;
    }
}




// Public interface below


static Scheduler Scheduler;


void InitScheduler() {
SimOutput("InitScheduler(): Initializing scheduler", 4);
Scheduler.Init();
}


void HandleNewTask(Time_t time, TaskId_t task_id) {
 
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
   MachineInfo_t machine = Machine_GetInfo(machine_id);


   // Remove from transitioning set
   transitioning_machines.erase(machine_id);


   if (machine.s_state == S5) {
       cout << "Machine " << machine_id << " is now powered down." << endl;
   } else if (machine.s_state == S0) {
       //cout << "Machine " << machine_id << " is now active." << endl;
   }
}


void SLAWarning(Time_t time, TaskId_t task_id) {
 SimOutput("SLAWarning(): Task " + to_string(task_id) +
           " encountered an SLA warning at time " + to_string(time), 4);
}


