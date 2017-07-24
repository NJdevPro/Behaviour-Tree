#pragma once

#include <iostream>
#include <list>
#include <vector>
#include <stack>
#include <initializer_list>
#include <string>
#include <cstdlib>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <future>

/// A C++11 Implementation of the Behavior Tree design pattern
/// 
/// http://www.gamasutra.com/blogs/ChrisSimpson/20140717/221339/Behavior_trees_for_AI_How_they_work.php
/// http://guineashots.com/2014/07/25/an-introduction-to-behavior-trees-part-1/
/// https://takinginitiative.wordpress.com/2014/02/17/synchronized-behavior-trees/

class BehaviourTree {


public:
	enum class Status {
		ERROR = -1,
		FAILURE = 0,
		SUCCESS = 1,
		RUNNING = 2,
		NOTRUN = 3
	};

	// This class represents each node in the behaviour tree.
	class Node {
	public:
		explicit Node(const bool dontSkip = false) : _name(__func__), _dontSkip(dontSkip) {}
		Node(const std::string& name,
			 const bool dontSkip = false) : _name(name), _dontSkip(dontSkip) {}

		virtual Status run() = 0;
		
		const std::string getName() const { return _name; }
		const bool isCompleted() const { return _completed; }
		const bool dontSkip() const { return _dontSkip; }
		const Status  getLastStatus() const { return _lastStatus; }

	protected:
		const std::string _name;
		bool _dontSkip;						// never skip this node
		bool _completed = false;
		Status _lastStatus = Status::NOTRUN;
	};

	//  This type of Node follows the Composite Pattern, containing a list of other Nodes.
	class CompositeNode : public Node {
	private:
		std::vector<Node*> children;
	public:
		CompositeNode() = default;
		~CompositeNode() { for (Node* child : children) { delete child; } }

		const std::vector<Node*>& getChildren() const { 
			return children; 
		}
		void addChild(Node* child) { 
			children.emplace_back(child); 
		}
		void addChildren(std::initializer_list<Node*>&& newChildren) { 
			for (Node* child : newChildren) addChild(child); 
		}
		template <typename CONTAINER>
		void addChildren(const CONTAINER& newChildren) { 
			for (Node* child : newChildren) addChild(child); 
		}
	};

	// The generic Selector implementation
	// If one child succeeds, the Select succeeds and quits immediately.
	// Status::FAILURE only if all children fail. Equivalent of a logical OR
	class Select : public CompositeNode {
	public:
		// If one child succeeds, the entire run() succeeds,
		// FAILURE only if all children fail,
		// RUNNING if at least one of the children is RUNNING and no other is in SUCCESS or ERROR.
		virtual Status run() override {
			Status s = Status::FAILURE;
			bool hasRunningChild = false;
			for (Node* child : getChildren()) {
				if (child->dontSkip()) {
					// run at each and every iteration
					s = child->run();
					_lastStatus = s;
				}
				else {
					// if the job has already done, return the status
					// else execute it
					if (child->isCompleted()) {
						s = child->getLastStatus();
					}
					else {
						s = child->run();
						_lastStatus = s;
					}
				}
				if (s == Status::SUCCESS || s == Status::ERROR) {
					return s;
				}
				if (s == Status::RUNNING) {
					hasRunningChild = true;
				}
			}
			if (!hasRunningChild) {
				_completed = true;
			}
			else{
				s = Status::RUNNING;
			}
			return s;  // All children failed so the entire run() operation fails.
		}
	};

	// The generic Sequence implementation.
	// If one child fails, then the entire sequence fails and quits immediately.  
	// Status::SUCCESS only if all children succeed. Equivalent of a logical AND.
	class Sequence : public CompositeNode {
	public:
		// If one child fails, then the entire operation fails.  
		// SUCCESS only if all children succeed.
		// RUNNING if at least one of the children is RUNNING and no other is in SUCCESS or ERROR.
		virtual Status run() override {
			for (Node* child : getChildren()) {
				Status s = Status::ERROR;
				if (child->dontSkip()) {
					// run at each and every iteration
					s = child->run();
					_lastStatus = s;
				}
				else {
					// if the job has already done, return the status
					// else execute it
					if (child->isCompleted()) {
						s = child->getLastStatus();
					}
					else {
						s = child->run();
						_lastStatus = s;
						if (s != Status::RUNNING) 
							_completed = true;
					}
					if (s != Status::SUCCESS) {
						return s;
					}
				}
				switch (s) {
				case Status::RUNNING: break;
				case Status::SUCCESS: continue;
				case Status::FAILURE:
				case Status::ERROR: _completed = true; return s;
				}
			}
			return Status::SUCCESS;  // All children suceeded, so the entire run() operation succeeds.
		}
	};

	// A Decorator adds a functionality to its child node.
	// Function is either to transform the Status it receives from the child, 
	// to terminate the child, or repeat the processing of the child, depending 
	// on the type of decorator node.
	class DecoratorNode : public Node {  
	private:
		Node* child;  // Only one child allowed
	protected:
		Node* getChild() const { return child; }
	public:
		DecoratorNode() = default;
		~DecoratorNode() { delete child; }
		void setChild(Node* newChild) { child = newChild; }
	};

	// Root of a BehaviourTree
	class Root : public DecoratorNode {
	private:
		friend class BehaviourTree;
		virtual Status run() override { 
			Status s = getChild()->run();
			while (s == Status::RUNNING)
				s = getChild()->run();
			return s;
		}
	};

	// Negates the Status of the child. 
	// A child fails and it will return Status::SUCCESS to its parent, 
	// or a child succeeds and it will return Status::FAILURE to the parent.
	class Invert : public DecoratorNode {  
	private:
		virtual Status run() override {
			Node* child = getChild();
			if (child->dontSkip()) {
				// run at each and every iteration
				const Status s = child->run();
				_lastStatus = s;
				switch (s)
				{
				case Status::SUCCESS: return Status::FAILURE;
				case Status::FAILURE: return Status::SUCCESS;
				default: return s;
				}
			}
			else {
				// if the job has already been done, return the status
				// else execute it
				Status s = Status::ERROR;
				if (child->isCompleted()) {
					s = child->getLastStatus();
				}
				else {
					s = child->run();
					_lastStatus = s;
					if (s != Status::RUNNING)
						_completed = true;
				}
				switch (s)
				{
				case Status::SUCCESS: return Status::FAILURE;
				case Status::FAILURE: return Status::SUCCESS;
				default: return s;
				}
			}
		}
	};

	// A succeeder will always return Status::SUCCESS, irrespective of 
	// what the child node actually returned. 
	// These are useful in cases where you want to process a branch of a tree 
	// where a Status::FAILURE is expected or anticipated, 
	// but you don’t want to abandon processing of a sequence that branch sits on.
	class Succeed : public DecoratorNode {  
	private:
		virtual Status run() override {
			Node* child = getChild();
			if (child->dontSkip()) {
				// run at each and every iteration
				const Status s = child->run();
				_lastStatus = s;
				if (s == Status::ERROR || s == Status::RUNNING)
					return s;
			}
			else {
				// if the job has already done, return the status
				// else execute it
				Status s = Status::ERROR;
				if (child->isCompleted()) {
					s = child->getLastStatus();
				}
				else {
					s = child->run();
					_lastStatus = s;
					if (s != Status::RUNNING)
						_completed = true;
				}
				if (s == Status::ERROR || s == Status::RUNNING)
					return s;
			}
			return Status::SUCCESS;
		}
	};

	// The opposite of a Succeeder, always returning fail.
	// Note that this can be achieved also by using an Inverter and setting its child to a Succeeder.
	class Fail : public DecoratorNode {  
	private:
		virtual Status run() override { 
			Node* child = getChild();
			if (child->dontSkip()) {
				// run at each and every iteration
				const Status s = child->run();
				_lastStatus = s;
				if (s == Status::ERROR || s == Status::RUNNING)
					return s;
			}
			else {
				// if the job has already done, return the status
				// else execute it
				Status s = Status::ERROR;
				if (child->isCompleted()) {
					s = child->getLastStatus();
				}
				else {
					s = child->run();
					_lastStatus = s;
					if (s != Status::RUNNING)
						_completed = true;
				}
				if (s == Status::ERROR || s == Status::RUNNING)
					return s;
			}
			return Status::FAILURE;
		}
	};

	// A repeater will reprocess its child node each time its child returns a Status. 
	// These are often used at the very base of the tree, to make the tree to run continuously. 
	// Repeaters may optionally run their children a set number of times before returning to their parent.
	class Repeat : public DecoratorNode { 
	public:
		explicit Repeat(int num = NOT_FOUND) : _numRepeats(num) {}  // By default, never terminate.
	private:
		int _numRepeats;
		static const int NOT_FOUND = -1;

		void iterate(Status& s) {
			Node* child = getChild();
			if (child->dontSkip()) {
				// run at each and every iteration
				s = child->run();
			}
			else {
				// if the job has already done, return the status
				// else execute it
				if (child->isCompleted()) {
					s = child->getLastStatus();
				}
				else {
					s = child->run();
					_lastStatus = s;
					if (s != Status::RUNNING)
						_completed = true;
				}
			}
		}

		virtual Status run() override {
			Status s = Status::ERROR;
			if (_numRepeats == NOT_FOUND)
			{
				// Loop indefinitely unless...
				do {
					iterate(s);
				} while (s != Status::ERROR && s != Status::RUNNING);
			}
			else {
				for (int i = 0; i < _numRepeats; i++)
				{
					iterate(s);
					if (s == Status::ERROR || s == Status::RUNNING)
						break;
				}
			}
			return s;
		}
	};

	// Execute its child asynchronously in a separate thread,
	// regularly yielding RUNNNING until it gets a final Status
	class Async : public DecoratorNode {
	public:
		Async(std::chrono::milliseconds poolTime = std::chrono::milliseconds(10)) : _statusPoolTime(poolTime) {}
	private:
		std::chrono::milliseconds _statusPoolTime;

		virtual Status run() override {
			Node* child = getChild();

			if (child->dontSkip()) {
				// run at each and every iteration
				std::future<Status> fut = std::async(std::launch::async, [&] {
					return getChild()->run();
				});
				// if no answer within time delay
				if (fut.wait_for(_statusPoolTime) == std::future_status::timeout) {
					_lastStatus = Status::RUNNING;
				}
				else {
					_lastStatus = fut.get();
				}
			}
			else {
				// if the job has already done, return the status
				// else execute it
				if (child->isCompleted()) {
					return child->getLastStatus();
				}
				else {
					std::future<Status> fut = std::async(std::launch::async, [&] {
						return getChild()->run();
					});
					// if no answer within time delay
					if (fut.wait_for(_statusPoolTime) == std::future_status::timeout) {
						_lastStatus = Status::RUNNING;
					}
					else {
						_lastStatus = fut.get();
						_completed = true;
					}
				}
			}
			return _lastStatus;
		}
	};

	// Insert a delay in msec (1 msec by default) and return Status::SUCCESS
	class Sleep : public DecoratorNode {
		Sleep(const std::chrono::milliseconds msec = std::chrono::milliseconds(1)) : _msec(msec) {}
	private:
		std::chrono::milliseconds _msec;
		virtual Status run() override {
			std::this_thread::sleep_for(_msec);
			return Status::SUCCESS;
		}
	};

	// Like a repeater, these decorators will continue to reprocess their child. 
	// That is until the child finally returns the expected status, at which point 
	// the repeater will return the status to its parent.
	// The expected status must be either Status::SUCCESS or Status::FAILURE.
	class RepeatUntil : public DecoratorNode {  
	public:
		RepeatUntil(const std::string& name,
					const Status exitStatus,
					const bool neverSkip = false) : _exitStatus(exitStatus) {}
	private:
		Status _exitStatus;
		virtual Status run() override {
			Status s = Status::ERROR;
			Node* child = getChild();
			if (child->dontSkip()) {
				// run at each and every iteration
				s = getChild()->run();
				while (s != _exitStatus && s != Status::ERROR && s != Status::RUNNING) {
					s = getChild()->run();
					_lastStatus = s;
				}
			}
			else {
				// if the job has already done, return the status
				// else execute it
				if (child->isCompleted()) {
					s = child->getLastStatus();
				}
				else {
					s = getChild()->run();
					while (s != _exitStatus && s != Status::ERROR && s != Status::RUNNING) {
						s = getChild()->run();
						_lastStatus = s;
						if (s != Status::RUNNING)
							_completed = true;
					}
				}
			}
			return s;
		}
	};

	/// The following are useful nodes
	
	// Stack nodes
	template <typename T>
	class StackNode : public Node {
	protected:
		std::stack<T*>& stack;  // Must be reference to a stack to work.
		StackNode(std::stack<T*>& s) : stack(s) {}
	};

	// Specific type of leaf (hence has no child).
	template <typename T>
	class PushToStack : public StackNode<T> { 
	private:
		T*& item;
	public:
		PushToStack(T*& t, std::stack<T*>& s) : StackNode<T>(s), item(t) {}
	private:
		virtual Status run() override {
			this->stack.push(item);
			return Status::SUCCESS;
		}
	};

	// Specific type of leaf (hence has no child).
	template <typename T>
	class GetStack : public StackNode<T> {
	private:
		const std::stack<T*>& obtainedStack;
		T* object;
	public:
		GetStack(std::stack<T*>& s, const std::stack<T*>& o, T* t = nullptr) : StackNode<T>(s), obtainedStack(o), object(t) {}
	private:
		virtual Status run() override {
			this->stack = obtainedStack;
			if (object)
				this->stack.push(object);
			return Status::SUCCESS;
		}
	};

	// Specific type of leaf (hence has no child).
	template <typename T>
	class PopFromStack : public StackNode<T> {
	private:
		T*& item;
	public:
		PopFromStack(T*& t, std::stack<T*>& s) : StackNode<T>(s), item(t) {}
	private:
		virtual Status run() override {
			if (this->stack.empty())
				return Status::FAILURE;
			item = this->stack.top();
			// template specialization with T = Door needed for this line actually
			std::cout << "Trying to get through door #" << item->doorNumber << "." << std::endl;
			this->stack.pop();
			return Status::SUCCESS;
		}
	};

	// Specific type of leaf (hence has no child).
	template <typename T>
	class StackIsEmpty : public StackNode<T> { 
	public:
		StackIsEmpty(std::stack<T*>& s) : StackNode<T>(s) {}
	private:
		virtual Status run() override {
			if (this->stack.empty())
				return Status::SUCCESS;
			else
				return Status::FAILURE;
		}
	};

	// Specific type of leaf (hence has no child).
	template <typename T>
	class SetVariable : public BehaviourTree::Node { 
	private:
		T*& variable, *& object;  // Must use reference to pointer to work correctly.
	public:
		SetVariable(T*& t, T*& obj) : variable(t), object(obj) {}
		virtual Status run() override {
			variable = object;
			// template specialization with T = Door needed for this line actually
			std::cout << "The door that was used to get in is door #" << variable->doorNumber << "." << std::endl; 
			return Status::SUCCESS;
		};
	};

	// Specific type of leaf (hence has no child).
	template <typename T>
	class IsNull : public BehaviourTree::Node { 
	private:
		T*& object;  // Must use reference to pointer to work correctly.
	public:
		IsNull(T*& t) : object(t) {}
		virtual Status run() override { 
			if (object == nullptr)
				return Status::SUCCESS;
			else
				return Status::FAILURE; 
		}
	};


public:
	BehaviourTree() : root(new Root) {}
	void setRootChild(Node* rootChild) const { root->setChild(rootChild); }
	Status run() const { return root->run(); }

private:
	Root* root;
};
