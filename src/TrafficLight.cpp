#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 
    // Perform queue modification under the lock
    std::unique_lock<std::mutex> uLock(_mutex);
    _cond.wait(uLock, [this] { return !_queue.empty(); }); // pass unique lock to condition variable

    // remove last vector element from queue
    T msg = std::move(_queue.back());
    _queue.pop_back();

    return msg; // will not be copied due to return value optimization (RVO) in C++
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    // simulate some work
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // perform vector modification under the lock
    std::lock_guard<std::mutex> uLock(_mutex);

    // add vector to queue
    std::string st = msg==TrafficLightPhase::green ? std::string("green") : std::string("red");
    std::cout << "MessageQueue: light = " << st << " has been sent to the queue" << std::endl;
    _queue.push_back(std::move(msg));
    _cond.notify_one(); // notify client after pushing new Vehicle into vector
}

/* Implementation of class "TrafficLight" */
TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

TrafficLight::~TrafficLight() {}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.

    TrafficLightPhase newPhase = TrafficLightPhase::red;
    // _msgQueue.receive() waits for any received message (transition on traffic light)
    while (true)
    {
        newPhase = _msgQueue.receive();
        if (newPhase == TrafficLightPhase::green)
            return;
    }
    return;
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class. 
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 
    std::unique_lock<std::mutex> lck(_mtx);
    std::cout << "TrafficLight::cycleThroughPhases: thread id = " << std::this_thread::get_id() << std::endl;
    lck.unlock();

    double cycleDuration = 1; // duration of a single simulation cycle in ms
    std::chrono::time_point<std::chrono::system_clock> lastUpdate;
    std::chrono::time_point<std::chrono::system_clock> lastTrafficLightCycleTime;

    // init stop watch
    lastUpdate = std::chrono::system_clock::now();
    lastTrafficLightCycleTime = std::chrono::system_clock::now();
    bool foundTrafficLightCycleTime = false;
    long trafficLightCycleDelta = 0;  // between 4 to 6 seconds
    long timeSinceLastUpdate = 0;
    long timeSinceLastTrafficLightCycle = 0;

    while (true)
    {
        // sleep at every iteration to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        // putting locks down here (or further below) will cause a lockup to the simulation! Why?
        // lck.lock();
        // std::cout << "TrafficLight cycleThroughPhases is still running!" << std::endl;
        // lck.unlock();
        // compute time difference to stop watch
        timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastUpdate).count();
        timeSinceLastTrafficLightCycle = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastTrafficLightCycleTime).count();
        if (timeSinceLastUpdate >= cycleDuration)
        {
            if (!foundTrafficLightCycleTime)
            {
                foundTrafficLightCycleTime = true;
                std::random_device rd;
                std::mt19937 eng(rd());
                std::uniform_int_distribution<> distr(4000, 6000);  // give me a number between these two
                trafficLightCycleDelta = distr(eng);
                // lck.lock();
                // std::cout << "TrafficLight trafficLightCycleDelta: " << trafficLightCycleDelta <<  " for thread id = " << std::this_thread::get_id() << "" << std::endl;
                // lck.unlock();
            }

            if (foundTrafficLightCycleTime && timeSinceLastTrafficLightCycle >= trafficLightCycleDelta)
            {
                TrafficLightPhase newPhase = this->_currentPhase==TrafficLightPhase::red ? TrafficLightPhase::green : TrafficLightPhase::red;
                this->_currentPhase = newPhase;
                foundTrafficLightCycleTime = false;
                lastTrafficLightCycleTime = std::chrono::system_clock::now();
                _msgQueue.send(std::move(newPhase));
            }
        }
        // reset stop watch for next cycle
        lastUpdate = std::chrono::system_clock::now();
    }

}
