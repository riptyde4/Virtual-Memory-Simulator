#include <cstdlib>
#include <chrono>
#include "VirtualMemorySim.h"
using namespace std;

VirtualMemorySimulator::VirtualMemorySimulator(int num_frames, int mode)
{
	//initialize physical memory to -1, representing vacant
	//physical_memory size represents
	for(int i = 0; i < num_frames; i++){
		struct frame f(false, FREE_FRAME, 0);
		physical_memory.push_back(f);
	}
	reference_count = 0;
	replacement_policy  = mode;
	fault_count = 0;
	srand(time(0));
}

void VirtualMemorySimulator::start(int process_id, int size){
	struct process p1;
	//p1.pid = process_id; //this may be unnecessary
	p1.num_pages = size;
	for(int i = 0; i < p1.num_pages; i++){
		//vector pages represents a processes address space
		//each index represents a page
		//a value of -1 represents a null reference
		p1.pages.push_back(-1);
	}
	pair<int, struct process> pair1(process_id, p1);
	virtual_memory.insert(pair1);
}

// Only used for the LRU replacement policy
// This function increments the ages of all frames in physical memory
// except for the page number passed in as an argument
void VirtualMemorySimulator::incAllOtherAges(int frameToIgnore){
	for(int frame = 0; frame < physical_memory.size(); frame++){
		if(frame != frameToIgnore){
			physical_memory[frame].age++;
		}
	}	
}
void VirtualMemorySimulator::fifoReplacement(int pid, int page_number){
	if(fifo.size() != physical_memory.size()){
		cout << "ERROR: Incoherence between queue and physical memory: " << physical_memory.size() << endl;
//		cout << "Queue size: " <<  fifo.size() << " vs " << physical_memory.size() << endl;
	}
	pair<int, struct frame> element = fifo.front();
	fifo.pop();
	int location = element.first;
	struct frame target = element.second;
	for(int i = 0; i < virtual_memory[target.pid].num_pages; i++){
		if(virtual_memory[target.pid].pages[i] == location){
		       virtual_memory[target.pid].pages[i] = -1;
		}
 	}
	physical_memory[location].pid = pid;
    //physical_memory[location].valid = true; // Not needed since if a frame is to be replaced, then every frame is allocated (valid) already
	
	virtual_memory[pid].pages[page_number] = location;
	pair<int, struct frame> newP(location, physical_memory[location]);
	fifo.push(newP);	
}

// This function removes the indicated frame from its original holder 
// and gives ownership to the indicated process
void VirtualMemorySimulator::replaceFrameHolder(int frame, int pid, int page_number){


//	cout << "Physical memory before frame replacment" << endl;
//	for(int i = 0; i < physical_memory.size(); i ++){
//		cout << "physical_memory[" << i << "] is held by process " << physical_memory[i].pid << endl;
//	}

	// Remove the old reference to the frame from the previous holder
	for(int i = 0; i < virtual_memory[physical_memory[frame].pid].pages.size(); i++){
		if(virtual_memory[physical_memory[frame].pid].pages[i] == frame){
//			cout << "Frame " << frame << " is page number " << i << endl;
//			cout << "Page: " << i << " is held in frame: " << virtual_memory[physical_memory[frame].pid].pages[i] << endl; 
			virtual_memory[physical_memory[frame].pid].pages[i] = -1;
			break;
		}
		//cout << "Page: " << i << " is held in frame: " << virtual_memory[physical_memory[frame].pid].pages[i] << endl; 
	}

	// Assign the frame's PID to the new PID
	physical_memory[frame].pid = pid;

	// If LRU then update ages
	if(replacement_policy == MODE_LRU){
		physical_memory[frame].age = 0;
		incAllOtherAges(frame);
	}

	// Update the location for the new page
	virtual_memory[pid].pages[page_number] = frame;


//	cout << "Physical memory after frame replacment" << endl;
//	for(int i = 0; i < physical_memory.size(); i ++){
//		cout << "physical_memory[" << i << "] is held by process" << physical_memory[i].pid << endl;
//	}
}

void VirtualMemorySimulator::randomReplacement(int pid, int page_number){
	// Generate a random index within the size of physical memory
	int randIndex = rand() % physical_memory.size();

	// Switch the frame holder to the new one
	replaceFrameHolder(randIndex, pid, page_number);
}

void VirtualMemorySimulator::LRUReplacement(int pid, int page_number){

	// Find the frame with the oldest age
	int frame = 0;
	int age = 0;
	for(int i = 0 ; i < physical_memory.size(); i ++){
		if(physical_memory[i].age > age){
			age = physical_memory[i].age;
			frame = i;
		}
	}

	// Switch the frame holder to the new one
	replaceFrameHolder(frame, pid, page_number);
}

int VirtualMemorySimulator::reference(int pid, int page_number1){

	// TODO: Check if the page is a valid page (within bounds of num_pages and num_frames)
	reference_count++;
	// Check if the page is not resident
	bool resident = false;
	struct process *p = &virtual_memory[pid];
	int page_number = page_number1 - 1;

	if(page_number1 < 1 || page_number1 > p->num_pages){
		cerr << "Invalid page reference" << endl;
		return IGNORE_REF;
	} 

	if(p->pages[page_number] != -1){
		resident = true;
	}

	// If the page number is not resident, check for available memory
	if(!resident){
		fault_count++;

		for(int frame = 0; frame < physical_memory.size(); frame++){
			if(!physical_memory[frame].valid){
				// Found a free frame of memory, allocate it
				p->pages[page_number] = frame;
				physical_memory[frame].valid = true;
				physical_memory[frame].pid = pid;
				if(replacement_policy == MODE_LRU){
					physical_memory[frame].age = 0;
					incAllOtherAges(frame);
				}
				else if(replacement_policy == MODE_RANDOM){
					// Do nothing
				}
				else if(replacement_policy == MODE_QUEUE){
					//place filled frame onto FIFO queue
					fifo.push(pair<int, struct frame>(frame, physical_memory[frame]));
//					cout << "Queue size is now " << fifo.size() << endl;
				}
				return PAGE_SUCCESS;
			}
		}
		// If we get to this point, there are no frames available in physical memory
		// Implement replacement policies here --------------------------------
		if(replacement_policy == MODE_RANDOM){
			//call random replacement
			randomReplacement(pid, page_number);
		}
		else if(replacement_policy == MODE_LRU){
			//call LRU replacement
			LRUReplacement(pid, page_number);
		}
		else if(replacement_policy == MODE_QUEUE){
			fifoReplacement(pid, page_number);
		}

		// --------------------------------------------------------------------

		return PAGE_FAULT;
	}
	else{

		// If the LRU Replacement policy is selected, set this page age to 0
		// and increment all other page ages
		if(replacement_policy == MODE_LRU){
			physical_memory[p->pages[page_number]].age = 0;
			incAllOtherAges(page_number);
		}
		return PAGE_RESIDENT;
	}
}

void VirtualMemorySimulator::terminate(int process_id){

	// Remove the process from the map
	struct process p = virtual_memory[process_id];
	if(replacement_policy == MODE_QUEUE){
		int count = 0;
		int size = fifo.size();
		while(count < size){
			pair<int, struct frame> p1 = fifo.front();
			fifo.pop();
			if(p1.second.pid != process_id){
				fifo.push(p1);
			}
			count++;
		}
	}
	for(int i = 0; i < p.num_pages; i++){
		if(p.pages[i] != -1){
			physical_memory[p.pages[i]].valid = false;
			physical_memory[p.pages[i]].pid = FREE_FRAME;
			physical_memory[p.pages[i]].age = 0;
		}
	}
	virtual_memory.erase(process_id);
}

int VirtualMemorySimulator::getFaultCount(){
	return fault_count;
}
int VirtualMemorySimulator::getReferenceCount(){
	return reference_count;
}
