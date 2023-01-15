#ifndef UTILITY_H
#define UTILITY_H

#include <memory>
#include <variant>

using namespace std;

// Pointers

template <class T>
using ptr = shared_ptr<T>;

template <class T>
using wptr = weak_ptr<T>;

// Macros

#define IS(variant_value, T) (holds_alternative<T>(variant_value))
#define AS(variant_value, T) (get<T>(variant_value))

#define IS_PTR(variant_value, T) (holds_alternative<ptr<T>>(variant_value))
#define AS_PTR(variant_value, T) (get<ptr<T>>(variant_value))

#define CREATE(T) (ptr<T>(new T))

#endif