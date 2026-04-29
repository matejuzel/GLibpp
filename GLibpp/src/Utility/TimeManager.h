#pragma once

//#pragma comment(lib, "winmm.lib")

#include <windows.h>
#include <functional>
#include <cmath>
#include <deque>
#include <vector>
#include <algorithm>
#include <timeapi.h>
#include <thread>

#include <timeapi.h> // Pot¯eba pro timeBeginPeriod
#include <chrono>

#pragma comment(lib, "winmm.lib")

class TimeManager {
public:
    // hz - frekvence fixnÌho updatu
    // fpsWindowSeconds - jak dlouhÈ obdobÌ se bere pro v˝poËet FPS (nap¯. 1.0s)
    TimeManager(double hz = 60.0, double fpsWindowSeconds = 1.0)
        : m_fixedDelta(1.0 / hz), m_accumulator(0.0), m_fpsWindow(fpsWindowSeconds)
    {
        QueryPerformanceFrequency(&m_freq);
        QueryPerformanceCounter(&m_start);
        m_prev = m_start;
        m_absoluteStart = m_start;

        timeBeginPeriod(1);
    }

    ~TimeManager() {
        timeEndPeriod(1);
	}

    // ZmÏ¯Ì Ëas a aktualizuje historii pro FPS
    double tick(double limit = 0.25) {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);

        double frameTime = double(now.QuadPart - m_prev.QuadPart) / double(m_freq.QuadPart);
        m_prev = now;

        // Ochrana proti spir·le smrti
        if (frameTime > limit) frameTime = limit;

        m_accumulator += frameTime;
        m_lastFrameTime = frameTime;

        // --- Aktualizace FPS historie ---
        double currentTime = sinceStart();
        m_frameTimestamps.push_back(currentTime);

        double cutoff = currentTime - m_fpsWindow;
        while (!m_frameTimestamps.empty() && m_frameTimestamps.front() < cutoff) {
            m_frameTimestamps.pop_front();
        }

        return frameTime;
    }

    void dispatchAction(const std::function<void(double)>& stepCallback) {
        while (m_accumulator >= m_fixedDelta) {
            stepCallback(m_fixedDelta);
            m_accumulator -= m_fixedDelta;
        }
    }

    void tickAndDispatchAction(const std::function<void(double)>& stepCallback, double limit = 0.25) {
        tick(limit);
        dispatchAction(stepCallback);
    }

    // --- FPS Metriky ---

    double getFps() const {
        if (m_frameTimestamps.size() < 2) return 0.0;
        return static_cast<double>(m_frameTimestamps.size()) / m_fpsWindow;
    }

    // p = 0.99 pro 1% Low, p = 0.999 pro 0.1% Low
    double getLowPercentile(double p) const {
        if (m_frameTimestamps.size() < 3) return 0.0;

        // V˝poËet delta Ëas˘ z historie timestamp˘
        m_tmpFrameTimes.clear();
        m_tmpFrameTimes.reserve(m_frameTimestamps.size() - 1);

        for (size_t i = 1; i < m_frameTimestamps.size(); ++i) {
            m_tmpFrameTimes.push_back(m_frameTimestamps[i] - m_frameTimestamps[i - 1]);
        }

        // Se¯azenÌ od nejrychlejöÌch po nejpomalejöÌ framy
        std::sort(m_tmpFrameTimes.begin(), m_tmpFrameTimes.end());

        // Vybereme index odpovÌdajÌcÌ percentilu (nap¯. 99% nejhoröÌho Ëasu)
        size_t idx = static_cast<size_t>(p * (m_tmpFrameTimes.size() - 1));
        double percentileFrameTime = m_tmpFrameTimes[idx];

        return (percentileFrameTime > 0) ? 1.0 / percentileFrameTime : 0.0;
    }

    double getLow1Percent() const { return getLowPercentile(0.99); }
    double getLowPoint1Percent() const { return getLowPercentile(0.999); }

    // --- OstatnÌ Gettery ---

    double getFixedDelta() const { return m_fixedDelta; }
    double getFrameDelta() const { return m_lastFrameTime; }
    double getInterpolationFactor() const { return m_accumulator / m_fixedDelta; }

	double getLastFrameTime() const { return m_lastFrameTime; }

    double sinceStart() const {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return double(now.QuadPart - m_absoluteStart.QuadPart) / double(m_freq.QuadPart);
    }

    void reset() {
        QueryPerformanceCounter(&m_prev);
        m_accumulator = 0.0;
        m_frameTimestamps.clear();
    }

    /**
     * UspÌ vl·kno, pokud do dalöÌho logickÈho kroku zb˝v· vÌce Ëasu neû threshold.
     * @param threshold Minim·lnÌ Ëas v sekund·ch, po kter˝ m· smysl sp·t (default 1ms).
     */
    void sleepIfIdle(double threshold = 0.001) {
        double timeToWait = m_fixedDelta - m_accumulator;
        if (timeToWait > threshold) {
            // SpÌme o nÏco mÈnÏ (nap¯. o 0.5ms), abychom nep¯eövihli zaË·tek kroku 
            // kv˘li nep¯esnosti Windows scheduleru.
            std::this_thread::sleep_for(std::chrono::microseconds(static_cast<long long>((timeToWait - 0.0005) * 1000000.0)));
        }
    }

	double getAccumulator() const 
    { 
        return m_accumulator; 
    }

    void waitUntilNextStep(double slack = 0.0005) {
        // 1. NastavÌme Windows scheduler na vysokou p¯esnost (pokud uû nenÌ)
        // Ide·lnÏ volat jednou glob·lnÏ, ale pro jistotu:
        static bool precisionSet = false;
        if (!precisionSet) {
            timeBeginPeriod(1);
            precisionSet = true;
        }

        while (true) {
            double timeToWait = m_fixedDelta - m_accumulator;

            // Pokud zb˝v· vÌc Ëasu neû rezerva + minim·lnÌ rozumn˝ sp·nek (1ms)
            if (timeToWait > (slack + 0.001)) {
                // SpÌme o 'slack' mÈnÏ
                double sleepTime = timeToWait - slack;
                std::this_thread::sleep_for(
                    std::chrono::microseconds(static_cast<long long>(sleepTime * 1000000.0))
                );

                // Po probuzenÌ musÌme znovu aktualizovat akumul·tor (vnit¯nÌ tick)
                // Aby smyËka vÏdÏla, kolik Ëasu re·lnÏ uplynulo
                updateAccumulatorOnly();
            }
            else {
                // Jsme v oblasti rezervy ñ zbytek Ëasu "doùuk·me" kr·tk˝mi yieldy 
                // nebo pr·zdnou smyËkou pro absolutnÌ p¯esnost.
                break;
            }
        }
    }

    double getMinFrameTime() const {
        if (m_frameTimestamps.size() < 2) return 0.0;
        double minDelta = std::numeric_limits<double>::max();
        for (size_t i = 1; i < m_frameTimestamps.size(); ++i) {
            double delta = m_frameTimestamps[i] - m_frameTimestamps[i - 1];
            if (delta < minDelta) {
                minDelta = delta;
            }
        }
        // P¯evod na FPS (pokud je minDelta 0, vr·tÌme 0, abychom se vyhnuli dÏlenÌ nulou)
        return minDelta;
	}

    double getMinFrameTimeFps() const {
        double minFrameTime = getMinFrameTime();
        return (minFrameTime > 0) ? 1.0 / minFrameTime : 0.0;
	}

    double getMaxFrameTime() const {
        if (m_frameTimestamps.size() < 2) return 0.0;

        double maxDelta = 0.0;
        for (size_t i = 1; i < m_frameTimestamps.size(); ++i) {
            double delta = m_frameTimestamps[i] - m_frameTimestamps[i - 1];
            if (delta > maxDelta) {
                maxDelta = delta;
            }
        }

        // P¯evod na FPS (pokud je maxDelta 0, vr·tÌme 0, abychom se vyhnuli dÏlenÌ nulou)
        return maxDelta;
    }

    double getMaxFrameTimeFps() const {
        double maxFrameTime = getMaxFrameTime();
        return (maxFrameTime > 0) ? 1.0 / maxFrameTime : 0.0;
    }

private:

    // Pomocn· metoda pro aktualizaci vnit¯nÌho Ëasu bez ovlivnÏnÌ FPS historie
    void updateAccumulatorOnly() {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        double dt = double(now.QuadPart - m_prev.QuadPart) / double(m_freq.QuadPart);
        m_prev = now;
        m_accumulator += dt;
    }

    LARGE_INTEGER m_freq;
    LARGE_INTEGER m_prev;
    LARGE_INTEGER m_start;
    LARGE_INTEGER m_absoluteStart;

    double m_fixedDelta;
    double m_accumulator;
    double m_lastFrameTime = 0.0;

    // FPS data
    double m_fpsWindow;
    std::deque<double> m_frameTimestamps;
    // mutable umoûÚuje modifikaci tmp pole i v const metod·ch (pro v˝poËet percentilu)
    mutable std::vector<double> m_tmpFrameTimes;
};