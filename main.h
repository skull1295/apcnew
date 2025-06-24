#pragma once
#include <Windows.h>
#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <Psapi.h>
#include "Injection.h"
#include "ICHooker.h"
#include "ntdlldefs.h"
#include "APCThreading.h"
#include "WorkerFactory.h"
#include "Tools.h"
#include "SyscallCaller.h"
int main(int argc, char** argv);