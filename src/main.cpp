#include <iostream>
#include <thread>
#include <chrono>
#include "aegis.hpp"

int main() {
    std::cout << "--- Aegis Standalone Daemon Started ---" << std::endl;
    std::cout << "[System] Initializing out-of-process observer..." << std::endl;
    
    // 1. Initialize the observer engine
    Aegis engine;
    
    std::cout << "[System] Aegis Daemon is now listening for real host telemetry..." << std::endl;
    std::cout << "[System] Press Ctrl+C to terminate." << std::endl;

    // 2. Keep the daemon alive indefinitely to observe the shared memory
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}