A C++11 implementation of the Behaviour Tree data structure/design pattern

This is an exercise based on articles I've read online.
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

The main disadvantage of BT compared to state achines is they can be 
less responsive than state machines because the tree needs to be 
constantly traversed.

Published under MIT License.
