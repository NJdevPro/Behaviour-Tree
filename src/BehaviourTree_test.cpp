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

typedef BehaviourTree BT;

// Acts as a storage for arbitrary variables that are interpreted and altered by the nodes.
struct DataContext {
	ConcurrentStack<Door*> doors;
	Door* currentDoor = nullptr;
	Door* usedDoor = nullptr;
};

class DoorAction : public BT::Node {
private:
	std::string name;
	int probabilityOfSuccess;
public:
	DoorAction(const std::string& newName, int prob) :
    name(newName), probabilityOfSuccess(prob) {}
private:
	BT::Status run() override {
		if (std::rand() % 100 < probabilityOfSuccess) {
			std::cout << name << " succeeded." << std::endl;
			return BT::Status::SUCCESS;
		}
		std::cout << name << " failed." << std::endl;
		return BT::Status::FAILURE;
	}
};

int main() {
	std::srand(42);

	BT behaviorTree;
	DataContext data;

	Building building(5);  // Building with 5 doors to get in.
	BT::Sequence sequence[3];
	BT::Select selector;
	BT::Invert inverter[2];
	BT::Succeed succeeder;
	BT::RepeatUntil untilFail("", BT::Status::FAILURE);
	BT::GetStack<Door> getDoorStackFromBuilding(data.doors, building.getDoors());
	BT::Pop<Door> popFromStack(data.currentDoor, data.doors);
	BT::SetVar<Door> setVariable(data.usedDoor, data.currentDoor);
	BT::IsNull<Door> isNull(data.usedDoor);
	BT::Async async;

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

	if (behaviorTree.run() == BT::Status::SUCCESS)
		std::cout << "Congratulations!  You made it into the building!" << std::endl;
	else
		std::cout << "Sorry.  You have failed to enter the building." << std::endl;
}

