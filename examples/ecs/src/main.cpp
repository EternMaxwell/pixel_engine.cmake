﻿#pragma once

#include <pixel_engine/entity.h>

#include <iostream>
#include <random>

#include "event_test_with_condition_system.h"
#include "general_spawn_despawn.h"
#include "resource_test.h"
#include "spawn_with_child.h"
#include "basic_loop.h"
#include "state_nextstate.h"

int main() {
    std::cout << "==TEST GENERAL SPAWN DESPAWN===" << std::endl;
    test_spawn_despawn::test();
    std::cout << "=====TEST SPAWN WITH CHILD=====" << std::endl;
    test_with_child::test();
    std::cout << "==========TEST RESOURCE========" << std::endl;
    test_resource::test();
    std::cout << "===========TEST EVENT==========" << std::endl;
    test_event::test();
    std::cout << "===========TEST BASIC LOOP==========" << std::endl;
    test_loop::test();
    std::cout << "===========TEST STATE NEXTSTATE==========" << std::endl;
    test_state::test();

    return 0;
}