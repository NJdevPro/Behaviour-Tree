A C++11 implementation of the Behaviour Tree data structure/design pattern
.
The behaviour tree design pattern fills the same niche as state machines,
but it has several advantages over them:
- it is easier to maintain and understand
- it can be multithreaded

The Behaviour tree (BT) is a decision tree where branching decisions are  
based on user defined environment variables.
A BT is comparable in some ways to an abstract syntax tree. 
Because it features nodes that allow branching, loops, delays and actions, it is 
Turing complete.
This particular implementation allows to run subtrees in a separate thread in a
fashion similar to futures.

The main disadvantage of BT compared to state machines is they can be 
less responsive than state machines because the tree needs to be 
constantly traversed.

This implementation is a single .h file making it very easy to integrate.

Each node has a NOTRUN status, and a run() method that returns to their parent node a final status. The final status can be either SUCCESS, FAILURE, or ERROR. Additionally, asynchronous nodes can have an intermediate RUNNING status indicating their parent nodes that they cannot return a final state as yet.

## Detailed descrition

### Branching type nodes

*Composite*: This type of Node follows the Composite Pattern, containing a list of 1...n children Nodes.

*Sequence*: Composite node. If one child fails, then the entire sequence fails and quits immediately.  
The Status is SUCCESS only if all children succeed. Equivalent of a logical AND.

*Select*: Composite Node. If one child succeeds, the Select succeeds and quits immediately.
The status is FAILURE only if all children fail. Equivalent of a logical OR.

*DecoratorNode*: A DecoratorNode adds a functionality to its child node. Function is either to transform the Status it receives from the child, to terminate the child, or repeat the processing of the child.

*Root*: A Decorator at the root of the Behaviour Tree.

*Invert*: A Decorator that negates the Status of the child. 

*Succeed*: A Decorator that will always return Status SUCCESS, irrespective of what the child node actually returned. These are useful in cases where you want to process a branch of a tree where a Status::FAILURE is expected or anticipated, but you don’t want to abandon the processing of a sequence that branch sits on.

*Fail*: A Decorator that is the opposite of a Succeed node, always returning fail.

*Repeat*: This Decorator will reprocess its child node each time its child returns a Status. These are often used at the root of the tree, to make the tree run continuously. Repeaters may optionally run their children a defined number of times before returning to their parent.

*RepeatUntil*: This Decorator will continue to reprocess its child until the child finally returns the expected status, at which point the repeater will return the status to its parent. The expected status must be either SUCCESS or FAILURE.

*Async*: This Decorator executes its child asynchronously in a separate thread, regularly yielding RUNNNING until it gets a final Status.

*Sleep*: A Decorator that inserts a delay in msec (1 msec by default) and return Status SUCCESS.

### Memory type nodes

These nodes persist data between node runs.
There are two sorts of memory: a thread safe stack and variables.

*SetVar*: associate a memory object to a variable

*IsNull*: return SUCCESS if the object passed in argument is nullptr.

*StackNode*: this node implements a stack.

*Push*: push an object on the stack node

*Pop*: pop an object from the stack node



Published under MIT License.
