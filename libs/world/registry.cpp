/**
 * \file registry.cpp
 * \brief Storage and lookup for registered component types.
 *
 * The table is a plain array with no initialiser, so it is ready before any
 * registrar runs and their order does not matter.
 */
#include "registry.hpp"

#include <charconv>
#include <cstring>

namespace sdk {

  namespace {
    const ComponentInfo *registry[max_components];
    uint16_t registry_count;

    /// Reads a whole number or float, rejecting anything left over.
    template<class T>
    bool parse_number(const char *text, void *dst) {
      const char *end = text + strlen(text);

      T value{};
      auto [stopped_at, error] = std::from_chars(text, end, value);

      if(error != std::errc{} || stopped_at != end)
        return false;

      memcpy(dst, &value, sizeof(T));
      return true;
    }

    /// Writes a number in its shortest round-trip form, terminated.
    template<class T>
    int format_number(const void *src, char *buf, int cap) {
      T value;
      memcpy(&value, src, sizeof(T));

      auto [stopped_at, error] = std::to_chars(buf, buf + cap - 1, value);  // -1 leaves room for the terminator

      if(error != std::errc{})
        return -1;

      *stopped_at = '\0';
      return static_cast<int>(stopped_at - buf);
    }

    int copy_text(const char *text, int length, char *buf, int cap) {
      if(length + 1 > cap)
        return -1;

      memcpy(buf, text, length);
      buf[length] = '\0';
      return length;
    }
  }

  bool parse_value(FieldType type, const char *text, void *dst) {
    if(!text || !*text || !dst)
      return false;

    switch(type) {
      case FieldType::Bool: {
        bool value;

        if(strcmp(text, "true") == 0 || strcmp(text, "1") == 0)
          value = true;
        else if(strcmp(text, "false") == 0 || strcmp(text, "0") == 0)
          value = false;
        else
          return false;

        memcpy(dst, &value, sizeof(value));
        return true;
      }

      case FieldType::Int32:  return parse_number<int32_t>(text, dst);
      case FieldType::UInt32: return parse_number<uint32_t>(text, dst);
      case FieldType::Float:  return parse_number<float>(text, dst);

      case FieldType::String: {
        const size_t length = strlen(text);

        if(length >= string_field_size)  // no room for the terminator
          return false;

        auto chars = static_cast<char *>(dst);
        memcpy(chars, text, length);
        memset(chars + length, 0, string_field_size - length);
        return true;
      }
    }

    return false;
  }

  int format_value(FieldType type, const void *src, char *buf, int cap) {
    if(!src || !buf || cap <= 0)
      return -1;

    switch(type) {
      case FieldType::Bool: {
        bool value;
        memcpy(&value, src, sizeof(value));

        const char *text = value ? "true" : "false";
        return copy_text(text, static_cast<int>(strlen(text)), buf, cap);
      }

      case FieldType::Int32:  return format_number<int32_t>(src, buf, cap);
      case FieldType::UInt32: return format_number<uint32_t>(src, buf, cap);
      case FieldType::Float:  return format_number<float>(src, buf, cap);

      case FieldType::String: {
        auto chars = static_cast<const char *>(src);
        // a field filled to the brim has no terminator of its own
        const auto length = static_cast<int>(strnlen(chars, string_field_size));
        return copy_text(chars, length, buf, cap);
      }
    }

    return -1;
  }

  const char *field_type_name(FieldType type) {
    switch(type) {
      case FieldType::Bool:   return "bool";
      case FieldType::Int32:  return "int32";
      case FieldType::UInt32: return "uint32";
      case FieldType::Float:  return "float";
      case FieldType::String: return "string";
    }

    return "?";
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
