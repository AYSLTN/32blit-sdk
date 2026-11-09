/**
 * \file registry.cpp
 * \brief Storage and lookup for registered component types.
 *
 * The table is a plain array with no initialiser, so it is ready before any
 * registrar runs and their order does not matter.
 */
#include "registry.hpp"

#include <cstring>

namespace sdk {

  namespace {
    const ComponentInfo *registry[max_components];
    uint16_t registry_count;
  }

  namespace detail {

    ComponentRegistrar::ComponentRegistrar(const char *name, uint16_t size, uint16_t align,
                                           const FieldInfo *fields, uint8_t field_count)
      : info{name, registry_count, size, align, 0, fields, field_count} {

      if(registry_count == max_components)
        return;  // over the limit: reported by find_component failing, not by crashing here

      registry[registry_count++] = &info;
    }
  }

  const ComponentInfo *find_component(const char *name) {
    if(!name)
      return nullptr;

    for(uint16_t i = 0; i < registry_count; i++) {
      if(strcmp(registry[i]->name, name) == 0)
        return registry[i];
    }

    return nullptr;
  }

  const ComponentInfo *component_by_id(uint16_t id) {
    return id < registry_count ? registry[id] : nullptr;
  }

  uint16_t component_count() {
    return registry_count;
  }
}
