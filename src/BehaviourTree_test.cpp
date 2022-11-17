// BehaviorTree_test.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <list>
#include <string>
#include <cstdlib>
#include "ConcurrentStack.h"
#include "BehaviourTree.h"


// 
// http://www.gamasutra.com/blogs/ChrisSimpson/20140717/221339/Behavior_trees_for_AI_How_they_work.php

struct Door {
	int doorNumber;
};

class Building {
private:
	ConcurrentStack<Door*> doors;
public:
	explicit Building(int numDoors) { initializeBuilding(numDoors); }
	const ConcurrentStack<Door*>& getDoors() const { return doors; }
private:
	void initializeBuilding(int numDoors) {
		for (int i = 0; i < numDoors; i++)
			doors.push(new Door{ numDoors - i });
	}
};

// Acts as a storage for arbitrary variables that are interpreted and altered by the nodes.
struct DataContext {
	ConcurrentStack<Door*> doors;
	Door* currentDoor = nullptr;
	Door* usedDoor = nullptr;
};

class DoorAction : public BehaviourTree::Node {
private:
	std::string name;
	int probabilityOfSuccess;
public:
	DoorAction(const std::string& newName, int prob) :
    name(newName), probabilityOfSuccess(prob) {}
private:
	BehaviourTree::Status run() override {
		if (std::rand() % 100 < probabilityOfSuccess) {
			std::cout << name << " succeeded." << std::endl;
			return BehaviourTree::Status::SUCCESS;
		}
		std::cout << name << " failed." << std::endl;
		return BehaviourTree::Status::FAILURE;
	}
};

int main() {
	std::srand(42);

	BehaviourTree behaviorTree;
	DataContext data;

	Building building(5);  // Building with 5 doors to get in.
	BehaviourTree::Sequence sequence[3];
	BehaviourTree::Select selector;
	BehaviourTree::Invert inverter[2];
	BehaviourTree::Succeed succeeder;
	BehaviourTree::RepeatUntil untilFail("", BehaviourTree::Status::FAILURE);
	BehaviourTree::GetStack<Door> getDoorStackFromBuilding(data.doors, building.getDoors());
	BehaviourTree::Pop<Door> popFromStack(data.currentDoor, data.doors);
	BehaviourTree::SetVar<Door> setVariable(data.usedDoor, data.currentDoor);
	BehaviourTree::IsNull<Door> isNull(data.usedDoor);
	BehaviourTree::Async async;

	// Probabilities of success
	DoorAction walkToDoor("Walk to door", 99), 
		openDoor("Open door", 12), 
		unlockDoor("Unlock door", 25), 
		smashDoor("Smash door", 60), 
		walkThroughDoor("Walk through door", 85), 
		closeDoor("Close door", 100);

	// Build the tree (last diagram of
	// http://www.gamasutra.com/blogs/ChrisSimpson/20140717/221339/Behavior_trees_for_AI_How_they_work.php )
	behaviorTree.setRootChild(&sequence[0]);
	sequence[0].addChildren({ &getDoorStackFromBuilding, &untilFail, &inverter[0] });
	untilFail.setChild(&sequence[1]);
	inverter[0].setChild(&isNull);
	sequence[1].addChildren({ &popFromStack, &inverter[1] });
	inverter[1].setChild(&async);
	async.setChild(&sequence[2]);
	sequence[2].addChildren({ &walkToDoor, &selector, &walkThroughDoor, &succeeder, &setVariable });
	selector.addChildren({ &openDoor, &unlockDoor, &smashDoor });
	succeeder.setChild(&closeDoor);

	if (behaviorTree.run() == BehaviourTree::Status::SUCCESS)
		std::cout << "Congratulations!  You made it into the building!" << std::endl;
	else
		std::cout << "Sorry.  You have failed to enter the building." << std::endl;
}

