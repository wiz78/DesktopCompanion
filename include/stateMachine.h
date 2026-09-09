//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include <Arduino.h>

#include <functional>
#include <unordered_map>

template <typename T, typename State>
class StateMachine
{
public:
    StateMachine( State initialState, const std::unordered_map<State, void(T::*)()>& handlers ) : handlers( handlers )
    {
        setState( initialState );
    }

protected:
    void runStateMachine()
    {
        if( const auto handler = handlers.find( state ); handler != handlers.end() )
            std::invoke( handler->second, reinterpret_cast<T&>( *this ));
    }

    void setState( const State x ) { if( x != state ) { state = x; stateStart = millis(); } }
    void resetStateTimer() { stateStart = millis(); }
    [[nodiscard]] State getState() const { return state; }
    [[nodiscard]] unsigned long getStateTime() const { return millis() - stateStart; }
    [[nodiscard]] bool stateElapsed( const unsigned long ms ) const { return getStateTime() >= ms; }

private:
    const std::unordered_map<State, void( T::* )()> handlers;
    State state{};
    unsigned long stateStart = 0;
};

#endif //STATEMACHINE_H
