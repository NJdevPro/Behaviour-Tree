//
// Created by njanin on 17/11/22.
//

#include <iostream>
#include <cassert>
#include "ConcurrentStack.h"

ConcurrentStack<int> stack(5, std::chrono::milliseconds(500));

int main()
{
    //assert(stack.top() == 0);
    //assert(stack.pop() == 0);
    stack.push(1);
    assert(stack.top() == 1);
    stack.push(2);
    stack.push(3);
    stack.push(4);
    stack.push(5);
    assert(stack.top() == 5);
    assert(stack.pop() == 5);
    assert(stack.pop() == 4);
    assert(stack.pop() == 3);
    assert(stack.pop() == 2);
    assert(stack.pop() == 1);
}
