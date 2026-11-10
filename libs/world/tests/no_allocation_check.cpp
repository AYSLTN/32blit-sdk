// The library must not allocate while a game is running: the handheld has no
// room for fragmentation and no tolerance for a malloc inside a frame. This
// counts allocations around the calls under test, which catches the usual way
// that rule gets broken, someone reaching for std::string or std::vector.
//
// A program of its own, with no GoogleTest. Replacing global operator new and
// delete in a binary means every allocation in it must go through the pair
// below; GoogleTest allocates through variants it is not worth replacing
// (std::get_temporary_buffer uses nothrow new), and the mismatch shows up
// under AddressSanitizer as alloc-dealloc-mismatch.
#include <cstdio>
#include <cstdlib>
#include <new>

#include "registry.hpp"

namespace {
  int allocations = 0;

  void *counted(size_t size) {
    allocations++;
    void *memory = malloc(size ? size : 1);

    if(!memory)
      abort();

    return memory;
  }
}

void *operator new(size_t size) { return counted(size); }
void *operator new[](size_t size) { return counted(size); }
void *operator new(size_t size, const std::nothrow_t &) noexcept { return counted(size); }
void *operator new[](size_t size, const std::nothrow_t &) noexcept { return counted(size); }

void operator delete(void *memory) noexcept { free(memory); }
void operator delete[](void *memory) noexcept { free(memory); }
void operator delete(void *memory, size_t) noexcept { free(memory); }
void operator delete[](void *memory, size_t) noexcept { free(memory); }
void operator delete(void *memory, const std::nothrow_t &) noexcept { free(memory); }
void operator delete[](void *memory, const std::nothrow_t &) noexcept { free(memory); }

namespace {
  struct Transform { float x, y, rotation; };
  struct Name { char value[sdk::string_field_size]; };
}
SDK_COMPONENT(Transform, x, y, rotation);
SDK_COMPONENT(Name, value);

int main() {
  char text[sdk::max_value_text];
  char name[sdk::string_field_size] = {};
  float number = 0.0f;

  const int before = allocations;

  // lookup
  auto component = sdk::find_component("Transform");
  sdk::component_by_id(0);
  sdk::component_count();
  sdk::field_type_name(sdk::FieldType::Float);

  // values, including a rejected one
  sdk::parse_value(sdk::FieldType::Float, "not-a-float", &number);
  sdk::parse_value(sdk::FieldType::Float, "1.5", &number);
  sdk::format_value(sdk::FieldType::Float, &number, text, sizeof(text));
  sdk::parse_value(sdk::FieldType::String, "player", name);
  sdk::format_value(sdk::FieldType::String, name, text, sizeof(text));

  const int allocated = allocations - before;

  if(!component) {
    printf("FAILED: Transform is not registered\n");
    return 1;
  }

  if(allocated) {
    printf("FAILED: %d allocation(s) during lookup and value conversion\n", allocated);
    return 1;
  }

  printf("PASSED: no allocations during lookup and value conversion\n");
  return 0;
}
