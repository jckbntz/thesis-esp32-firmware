#pragma once

#ifndef ACTIVE_NODE
  #error "ACTIVE_NODE not defined (set via PlatformIO build_flags)"
#endif

#if ACTIVE_NODE == 1
  #include "node_config_esp1.h"
  #include "credentials_esp1.h"
#elif ACTIVE_NODE == 2
  #include "node_config_esp2.h"
  #include "credentials_esp2.h"
#elif ACTIVE_NODE == 3
  #include "node_config_esp3.h"
  #include "credentials_esp3.h"
#elif ACTIVE_NODE == 4
  #include "node_config_esp4.h"
  #include "credentials_esp4.h"
#else
  #error "Invalid ACTIVE_NODE"
#endif