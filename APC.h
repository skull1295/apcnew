#pragma once
#include <Windows.h>
#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <Psapi.h>
#include "ntdlldefs.h"
#include "APCThreading.h"
#include "WorkerFactory.h"

bool CreateAPCDirectThread(HANDLE process, void* addr);