PROJECT(nstllib LANGUAGES CXX)

SET(nstllibHdrs
  range_print.hpp
  ro_buffer.hpp
  unlock_guard.hpp
)

ADD_LIBRARY(nstllib INTERFACE ${nstllibHdrs})
