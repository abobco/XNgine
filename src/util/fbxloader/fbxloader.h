#pragma once

#include <stdio.h>
#include <stdlib.h>

#include "raylib.h"

/* Load 3D model from a binary FBX file 
    1. loads entire file into memory
    2. creates nested linked list from relevant FBX nodes
    3. uploads vertex data to GPU */
Model LoadModel_FBX(const char *filename);
