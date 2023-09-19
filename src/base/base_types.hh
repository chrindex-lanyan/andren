#pragma once



#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string.h>
#include <string>
#include <memory>
#include <assert.h>


namespace chrindex::andren::base
{

#define DEFAULT_CONSTRUCTION(TYPE) TYPE()=default

#define DEFAULT_COPY_CONSTRUCTION(TYPE) TYPE(TYPE const&)=default

#define DEFAULT_COPY_OPERATOR(TYPE) TYPE&operator=(TYPE const &)=default

#define DEFAULT_DECONSTRUCTION(TYPE) ~TYPE()=default

#define DEFAULT_MOVE_CONSTRUCTION(TYPE) TYPE(TYPE &&)=default

#define DEFAULT_MOVE_OPERATOR(TYPE) TYPE&operator=(TYPE &&)=default

#define DELETE_COPY_CONSTRUCTION(TYPE) TYPE(TYPE const&)=delete

#define DELETE_COPY_OPERATOR(TYPE) TYPE&operator=(TYPE const&)=delete

#define DELETE_MOVE_CONSTRUCTION(TYPE) TYPE(TYPE &&)=delete

#define DELETE_MOVE_OPERATOR(TYPE) TYPE&operator=(TYPE &&)=delete

#define _IN_ 
#define _OUT_  
#define _IN_OUT_

}

