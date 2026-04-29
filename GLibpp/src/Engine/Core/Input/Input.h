#pragma once
#include <vector>
#include <array>
#include <algorithm>

namespace GLibpp {


    class Keyboard {
    public:
        Keyboard() {
            m_rawInput.fill(false);
            m_current.fill(false);
            m_previous.fill(false);
        }

        // --- VLÁKNO OKNA (WndProc / pollEvents) ---

        /**
            * Voláno skrze Windows message loop.
            * Zapisuje do "surové" schránky, která je oddìlená od logiky.
            */
        void setKeyState(unsigned char vkey, bool isDown) {
            // Na x86 je zápis do bool (1 byte) atomický, 
            // nepotøebujeme mutex pro toto jednoduché nastavení.
            m_rawInput[vkey] = isDown;
        }


        // --- LOGICKÉ VLÁKNO (timer.tickAndDispatchAction) ---

        /**
            * Synchronizuje stavy. Volá se JEDNOU na zaèátku každého updateLogic kroku.
            * Pøeklopí surový vstup do stabilního stavu pro aktuální frame.
            */
        void update() {
            // 1. To, co bylo v minulém kroku aktuální, je teï pøedchozí
            m_previous = m_current;

            // 2. Naèteme surová data ze schránky do aktuálního stavu
            // Tímto se "zmrazí" vstup pro celou dobu trvání updateLogic()
            m_current = m_rawInput;
        }

        /** Je klávesa aktuálnì držena? */
        bool isDown(unsigned char vkey) const {
            return m_current[vkey];
        }

        /** Byla klávesa stisknuta pøesnì v tomto logickém kroku? (Debounce) */
        bool wasPressed(unsigned char vkey) const {
            return m_current[vkey] && !m_previous[vkey];
        }

        /** Byla klávesa uvolnìna pøesnì v tomto logickém kroku? */
        bool wasReleased(unsigned char vkey) const {
            return !m_current[vkey] && m_previous[vkey];
        }

        /** Pro pøípad ztráty focusu okna - vynuluje vše */
        void reset() {
            m_rawInput.fill(false);
            m_current.fill(false);
            m_previous.fill(false);
        }

    private:
        // Surová schránka - sem sype data vlákno okna (asynchronnì)
        std::array<bool, 256> m_rawInput;

        // Stabilní stavy - s tìmito pracuje logika (synchronnì v rámci update)
        std::array<bool, 256> m_current;
        std::array<bool, 256> m_previous;
    };



    class Input {
    public:


        // Singleton pøístup nebo instance v Enginu
        //Keyboard& getKeyboard() { return m_keyboard; }

        Keyboard keyboard;
    };

};

