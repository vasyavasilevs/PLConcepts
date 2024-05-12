#pragma once

#include <cassert>
#include <type_traits>
#include <unordered_map>

namespace macro_detail {

template <unsigned N>
struct FixedString {
    char buf[N + 1]{};

    constexpr FixedString(const char *s) {
        for (unsigned i = 0; i != N; ++i) {
            buf[i] = s[i];
        }
    }

    constexpr operator const char *() const {
        return buf;
    }
};

template<unsigned N> FixedString(const char (&)[N]) -> FixedString<N - 1>;

using MethodType = void (*)(void *);

struct MacroBasedVTable {
    MacroBasedVTable() = delete;
    explicit MacroBasedVTable(std::size_t id, MacroBasedVTable *most_derived, MacroBasedVTable *base)
            : base_vtable_ptr(base), type_id(id) {
        most_derived_vtable_ptr = (most_derived == nullptr) ? this : most_derived;
    }

    MacroBasedVTable *most_derived_vtable_ptr {nullptr};
    MacroBasedVTable *base_vtable_ptr {nullptr};
    std::size_t type_id {0};
    std::size_t class_body_size {0};
    std::unordered_map<const char *, MethodType> methods;
};

struct MethodVTableEmplacer {
    explicit MethodVTableEmplacer(MacroBasedVTable &vtable, const char *name, MethodType method) {
        auto inserted = vtable.methods.emplace(name, method);
        assert(inserted.second);
    }
};

template <typename T, typename V>
T *macro_dynamic_cast(V *object, MacroBasedVTable *from_object, MacroBasedVTable *from_target_class) {
    assert(object != nullptr);
    assert(from_object != nullptr);
    assert(from_target_class != nullptr);

    if constexpr (std::is_same_v<T, V>) {
        return object;
    }

    // Only downcasts are possible.
    bool can_cast = from_object->most_derived_vtable_ptr == from_target_class->most_derived_vtable_ptr &&
        from_object->type_id >= from_target_class->type_id;
    assert(can_cast);
    // Valid due to padding.
    return reinterpret_cast<T *>(object);
}

template <typename Base, typename Derived>
concept StaticMethodCallable = Base::_HIERARCHY_TAG == Derived::_HIERARCHY_TAG &&
    Base::_CLASS_TAG <= Derived::_CLASS_TAG;

}   // namespace macro_detail

template <typename T, macro_detail::FixedString MethodName>
struct MacroMethodCaller {
    static auto macro_call_method(T &object, const char *name) {
        for (auto *current_vtable = object._VTABLE_PTR;
                current_vtable != nullptr;
                current_vtable = current_vtable->base_vtable_ptr) {
            auto iter = current_vtable->methods.find(name);
            if (iter != current_vtable->methods.end()) {
                if constexpr (std::is_same_v<std::invoke_result_t<decltype(iter->second), void *>, void>) {
                    iter->second(&object);
                    return;
                } else {
                    return iter->second(&object);
                }
            }
        }
        assert(false);
    }
};

template <typename T, macro_detail::FixedString MethodName>
struct MacroMethodCaller<T *, MethodName> {
    static auto macro_call_method(T *object, const char *name) {
        for (auto *current_vtable = object->_VTABLE_PTR;
                current_vtable != nullptr;
                current_vtable = current_vtable->base_vtable_ptr) {
            auto iter = current_vtable->methods.find(name);
            if (iter != current_vtable->methods.end()) {
                if constexpr (std::is_same_v<std::invoke_result_t<decltype(iter->second), void *>, void>) {
                    iter->second(object);
                    return;
                } else {
                    return iter->second(object);
                }
            }
        }
        assert(false && "method not found");
    }
};

#define GLOBAL_VTABLE_NAME(CLASS_NAME)              CLASS_NAME ## _GLOBAL_MACRO_VTABLE
#define STATIC_METHOD_NAME(CLASS_NAME, METHOD_NAME) CLASS_NAME ## _ ## METHOD_NAME ## _METHOD_IMPL

#define BASE_CLASS_START(CLASS_NAME)                            \
macro_detail::MacroBasedVTable GLOBAL_VTABLE_NAME(CLASS_NAME)(  \
    __COUNTER__, nullptr, nullptr);                             \
struct CLASS_NAME {                                             \
    static constexpr long _HIERARCHY_TAG = __COUNTER__;         \
    static constexpr long _CLASS_TAG = __COUNTER__;             \
    macro_detail::MacroBasedVTable *_VTABLE_PTR                 \
        = &GLOBAL_VTABLE_NAME(CLASS_NAME);

#define DERIVED_CLASS_START(CLASS_NAME, BASE_CLASS_NAME)                        \
macro_detail::MacroBasedVTable GLOBAL_VTABLE_NAME(CLASS_NAME)(                  \
    __COUNTER__,                                                                \
    (GLOBAL_VTABLE_NAME(BASE_CLASS_NAME).most_derived_vtable_ptr == nullptr)    \
        ? &GLOBAL_VTABLE_NAME(BASE_CLASS_NAME)                                  \
        : GLOBAL_VTABLE_NAME(BASE_CLASS_NAME).most_derived_vtable_ptr,          \
    &GLOBAL_VTABLE_NAME(BASE_CLASS_NAME));                                      \
struct CLASS_NAME {                                                             \
    static constexpr long _HIERARCHY_TAG = BASE_CLASS_NAME::_HIERARCHY_TAG;     \
    static constexpr long _CLASS_TAG = __COUNTER__;                             \
    macro_detail::MacroBasedVTable *_VTABLE_PTR                                 \
        = &GLOBAL_VTABLE_NAME(CLASS_NAME);                                      \
    char _BASE_PADDING[sizeof(BASE_CLASS_NAME) -                                \
                       sizeof(macro_detail::MacroBasedVTable *)] = {0};

#define DYNAMIC_CAST(OBJECT, CLASS_NAME)                                    \
    macro_detail::macro_dynamic_cast<CLASS_NAME>(                           \
        (OBJECT), (OBJECT)->_VTABLE_PTR, &GLOBAL_VTABLE_NAME(CLASS_NAME))


#define METHOD_START(CLASS_NAME, METHOD_NAME)                                   \
void STATIC_METHOD_NAME(CLASS_NAME, METHOD_NAME)(void *object) {                \
    [[maybe_unused]] CLASS_NAME *self = reinterpret_cast<CLASS_NAME *>(object);

#define METHOD_END(CLASS_NAME, METHOD_NAME)                                                     \
}                                                                                               \
template <typename T>                                                                           \
requires macro_detail::StaticMethodCallable<CLASS_NAME, T>                                      \
struct MacroMethodCaller<T *, #METHOD_NAME> {                                                   \
    static auto macro_call_method(T *object, [[maybe_unused]] const char *name) {               \
        if constexpr (std::is_same_v<std::invoke_result_t<                                      \
                        decltype(STATIC_METHOD_NAME(CLASS_NAME, METHOD_NAME)), void *>,         \
                        void>) {                                                                \
            STATIC_METHOD_NAME(CLASS_NAME, METHOD_NAME)(object);                                \
            return;                                                                             \
        } else {                                                                                \
            return STATIC_METHOD_NAME(CLASS_NAME, METHOD_NAME)(object);                         \
        }                                                                                       \
    }                                                                                           \
};                                                                                              \
template <typename T>                                                                           \
requires macro_detail::StaticMethodCallable<CLASS_NAME, T>                                      \
struct MacroMethodCaller<T, #METHOD_NAME> {                                                     \
    static auto macro_call_method(T &object, [[maybe_unused]] const char *name) {               \
        if constexpr (std::is_same_v<std::invoke_result_t<                                      \
                        decltype(STATIC_METHOD_NAME(CLASS_NAME, METHOD_NAME)), void *>,         \
                        void>) {                                                                \
            STATIC_METHOD_NAME(CLASS_NAME, METHOD_NAME)(&object);                               \
            return;                                                                             \
        } else {                                                                                \
            return STATIC_METHOD_NAME(CLASS_NAME, METHOD_NAME)(&object);                        \
        }                                                                                       \
    }                                                                                           \
};

#define VIRTUAL_METHOD_START(CLASS_NAME, METHOD_NAME)                           \
void CLASS_NAME ## _ ## METHOD_NAME ## _VIRTUAL_METHOD_IMPL(void *object) {     \
    [[maybe_unused]] CLASS_NAME *self = reinterpret_cast<CLASS_NAME *>(object);

#define VIRTUAL_METHOD_END(CLASS_NAME, METHOD_NAME)                 \
}                                                                   \
volatile macro_detail::MethodVTableEmplacer                         \
    CLASS_NAME ## _ ## METHOD_NAME ## _METHOD_VTABLE_EMPLACER(      \
        GLOBAL_VTABLE_NAME(CLASS_NAME),                             \
        #METHOD_NAME,                                               \
        CLASS_NAME ## _ ## METHOD_NAME ## _VIRTUAL_METHOD_IMPL);


#define CALL_METHOD(OBJECT, METHOD_NAME)                                    \
    MacroMethodCaller<decltype(OBJECT), #METHOD_NAME>::macro_call_method(   \
        (OBJECT), #METHOD_NAME)
