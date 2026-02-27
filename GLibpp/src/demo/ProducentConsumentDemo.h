#pragma once

#include <thread>
#include <iostream>
#include "utils/datastruct/SPSCQueue.h"

class ProducentConsumentDemo
{
public:

    SPSCQueue<int, 256> queue;

	ProducentConsumentDemo() = default;

	void run() {

		std::thread t1(&ProducentConsumentDemo::producer, this);
		std::thread t2(&ProducentConsumentDemo::consumer, this);

		t1.join();
		t2.join();

		return;
	}

    void producer() {

        for (int i = 0; i < 100000; i++) {

            queue.pushLoop(i);
            std::cout << "Producer:" << i << std::endl;

            if (i % 10 == 0) std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (i % 100 == 0) std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }

        queue.pushLoop(-1);
    }

    void consumer() {

        // tohle je jeste lepsi varianta nez SPSCQueue::popLoop, protože umožòuje adaptivní backoff a tím snižuje CPU usage

        // Adaptivní backoff: pøi prázdné frontì postupnì prodlužujeme èekání
        // až do urèitého maxima. To sníží CPU usage, ale zvyšuje latenci.
        int backoffMs = 0;
        const int stepMs = 50; // pøírùstek pøi každém neúspìchu
		const int maxBackoffMs = 500; // maximální èekání mezi kontrolami

        while (true) {

            int value;
            if (queue.pop(value)) {
                
                // reset backoff po úspìšném ètení
                backoffMs = 0;
                if (value == -1) return;
                std::cout << "Consumer: " << value << std::endl;
            } else {
                
                // žádné data -> spíme krátce (kontrola nìkolikrát za vteøinu)
                backoffMs = std::min(backoffMs + stepMs, maxBackoffMs);
				std::this_thread::sleep_for(std::chrono::milliseconds(backoffMs));
            }
        }
    }

};
