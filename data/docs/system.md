# System object overview

## Types of object

* Process
* Thread
* Mutex
* Event
* VNode

## Object monitors

All objects have a monitor associated that can be waited on by threads.
* Waiting on a process waits for the process to exit
* Waiting on a thread waits for the thread to exit
* Waiting on an event waits for the event to be signaled

## Lifetimes

All objects are reference counted
* Processes hold strong references to all objects created by the process
* Threads hold a weak reference to their parent process
* Mutex's hold a weak reference to every thread that is currently locking or waiting on the mutex
* Events hold a weak reference to every thread that is currently waiting on them
* VNodes do not hold references

When an objects strong reference count reaches zero the object is marked as pending destruction.
Using an object that is pending destruction is not valid, before using an object it must have be `retainStrong`d
