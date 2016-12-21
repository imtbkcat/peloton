//===----------------------------------------------------------------------===//
//
//                         Peloton
//
// value.h
//
// Identification: src/backend/type/value.h
//
// Copyright (c) 2015-16, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cfloat>
#include <climits>
#include <cstdint>
#include <vector>

#include "common/exception.h"
#include "common/macros.h"
#include "common/printable.h"

#include "type/type.h"
#include "type/varlen_pool.h"
#include "type/serializeio.h"

namespace peloton {
namespace type {

class Type;

static const double DBL_LOWEST = std::numeric_limits<double>::lowest();
static const double FLT_LOWEST = std::numeric_limits<float>::lowest();

static const int8_t PELOTON_INT8_MIN = (SCHAR_MIN + 1);
static const int16_t PELOTON_INT16_MIN = (SHRT_MIN + 1);
static const int32_t PELOTON_INT32_MIN = (INT_MIN + 1);
static const int64_t PELOTON_INT64_MIN = (LLONG_MIN + 1);
static const double PELOTON_DECIMAL_MIN = FLT_LOWEST;
static const uint64_t PELOTON_TIMESTAMP_MIN = 0;
static const int8_t PELOTON_BOOLEAN_MIN = 0;

static const int8_t PELOTON_INT8_MAX = SCHAR_MAX;
static const int16_t PELOTON_INT16_MAX = SHRT_MAX;
static const int32_t PELOTON_INT32_MAX = INT_MAX;
static const int64_t PELOTON_INT64_MAX = LLONG_MAX;
static const uint64_t PELOTON_UINT64_MAX = ULLONG_MAX - 1;
static const double PELOTON_DECIMAL_MAX = DBL_MAX;
static const uint64_t PELOTON_TIMESTAMP_MAX = 11231999986399999999U;
static const int8_t PELOTON_BOOLEAN_MAX = 1;

static const uint32_t PELOTON_VALUE_NULL = UINT_MAX;
static const int8_t PELOTON_INT8_NULL = SCHAR_MIN;
static const int16_t PELOTON_INT16_NULL = SHRT_MIN;
static const int32_t PELOTON_INT32_NULL = INT_MIN;
static const int64_t PELOTON_INT64_NULL = LLONG_MIN;
static const uint64_t PELOTON_TIMESTAMP_NULL = ULLONG_MAX;
static const double PELOTON_DECIMAL_NULL = DBL_LOWEST;
static const int8_t PELOTON_BOOLEAN_NULL = SCHAR_MIN;

static const uint32_t PELOTON_VARCHAR_MAX_LEN = UINT_MAX;

// Objects (i.e., VARCHAR) with length prefix of -1 are NULL
#define OBJECTLENGTH_NULL -1

// A value is an abstract class that represents a view over SQL data stored in
// some materialized state. All values have a type and comparison functions, but
// subclasses implement other type-specific functionality.
class Value : public Printable {
#ifdef VALUE_TESTS
 public:
#else
 private:
#endif

  Value(const Type::TypeId type) : type_(Type::GetInstance(type)), manage_data_(false) {}

  // ARRAY values
  template <class T>
  Value(Type::TypeId type, const std::vector<T> &vals,
        Type::TypeId element_type);

  // BOOLEAN and TINYINT
  Value(Type::TypeId type, int8_t val);

  // DECIMAL
  Value(Type::TypeId type, double d);
  Value(Type::TypeId type, float f);

  // SMALLINT
  Value(Type::TypeId type, int16_t i);
  // INTEGER and PARAMETER_OFFSET
  Value(Type::TypeId type, int32_t i);
  // BIGINT
  Value(Type::TypeId type, int64_t i);

  // TIMESTAMP
  Value(Type::TypeId type, uint64_t i);

  // VARCHAR and VARBINARY
  Value(Type::TypeId type, const char *data, uint32_t len, bool manage_data);
  Value(Type::TypeId type, const std::string &data);

 public:
  Value();
  Value(Value &&other);
  Value(const Value &other);
  ~Value();

  friend void swap(Value &first, Value &second)  // nothrow
  {
    std::swap(first.type_, second.type_);
    std::swap(first.value_, second.value_);
    std::swap(first.size_, second.size_);
    std::swap(first.manage_data_, second.manage_data_);
  }

  Value &operator=(Value other);

  // Get the type of this value
  Type::TypeId GetTypeId() const { return type_->GetTypeId(); }
  const std::string GetInfo() const override;

  // Comparison functions
  //
  // NOTE:
  // We could get away with only CompareLessThan() being purely virtual, since
  // the remaining comparison functions can derive their logic from
  // CompareLessThan(). For example:
  //
  //    CompareEquals(o) = !CompareLessThan(o) && !o.CompareLessThan(this)
  //    CompareNotEquals(o) = !CompareEquals(o)
  //    CompareLessThanEquals(o) = CompareLessThan(o) || CompareEquals(o)
  //    CompareGreaterThan(o) = !CompareLessThanEquals(o)
  //    ... etc. ...
  //
  // We don't do this for two reasons:
  // (1) The redundant calls to CompareLessThan() may be a performance problem,
  //     and since Value is a core component of the execution engine, we want to
  //     make it as performant as possible.
  // (2) Keep the interface consistent by making all functions purely virtual.
  inline Value CompareEquals(const Value &o) const {
    return type_->CompareEquals(*this, o);
  }
  inline Value CompareNotEquals(const Value &o) const {
    return type_->CompareNotEquals(*this, o);
  }
  inline Value CompareLessThan(const Value &o) const {
    return type_->CompareLessThan(*this, o);
  }
  inline Value CompareLessThanEquals(const Value &o) const {
    return type_->CompareLessThanEquals(*this, o);
  }
  inline Value CompareGreaterThan(const Value &o) const {
    return type_->CompareGreaterThan(*this, o);
  }
  inline Value CompareGreaterThanEquals(const Value &o) const {
    return type_->CompareGreaterThanEquals(*this, o);
  }

  // Other mathematical functions
  inline Value Add(const Value &o) const {
    return type_->Add(*this, o);
  }
  inline Value Subtract(const Value &o) const {
    return type_->Subtract(*this, o);
  }
  inline Value Multiply(const Value &o) const {
    return type_->Multiply(*this, o);
  }
  inline Value Divide(const Value &o) const {
    return type_->Divide(*this, o);
  }
  inline Value Modulo(const Value &o) const {
    return type_->Modulo(*this, o);
  }
  inline Value Min(const Value &o) const {
    return type_->Min(*this, o);
  }
  inline Value Max(const Value &o) const {
    return type_->Max(*this, o);
  }
  inline Value Sqrt() const {
    return type_->Sqrt(*this);
  }
  inline Value OperateNull(const Value &o) const {
    return type_->OperateNull(*this, o);
  }
  inline bool IsZero() const {
    return type_->IsZero(*this);
  }

  // Is the data inlined into this classes storage space, or must it be accessed
  // through an indirection/pointer?
  inline bool IsInlined() const;

  // Is a value null?
  inline bool IsNull() const {
    return size_.len == PELOTON_VALUE_NULL;
  }

  // Examine the type of this object.
  bool CheckInteger() const;

  // Can two types of value be compared?
  bool CheckComparable(const Value &o) const;

  inline bool IsTrue() const {
    PL_ASSERT(GetTypeId() == Type::BOOLEAN);
    return (value_.boolean == 1);
  }

  inline bool IsFalse() const {
    PL_ASSERT(GetTypeId() == Type::BOOLEAN);
    return (value_.boolean == 0);
  }

  // Return a stringified version of this value
  inline std::string ToString() const {
    return type_->ToString(*this);
  }

  // Compute a hash value
  inline size_t Hash() const {
    return type_->Hash(*this);
  }

  inline void HashCombine(size_t &seed) const {
    return type_->HashCombine(*this, seed);
  }

  // Serialize this value into the given storage space. The inlined parameter
  // indicates whether we are allowed to inline this value into the storage
  // space, or whether we must store only a reference to this value. If inlined
  // is false, we may use the provided data pool to allocate space for this
  // value, storing a reference into the allocated pool space in the storage.
  inline void SerializeTo(char *storage, bool inlined, VarlenPool *pool) const {
    type_->SerializeTo(*this, storage, inlined, pool);
  }

  inline void SerializeTo(SerializeOutput &out) const {
    type_->SerializeTo(*this, out);
  }

  // Deserialize a value of the given type from the given storage space.
  inline static Value DeserializeFrom(const char *storage, const Type::TypeId type_id,
                               const bool inlined, VarlenPool *pool = nullptr) {
    return Type::GetInstance(type_id)->DeserializeFrom(storage, inlined, pool);
  }

  inline static Value DeserializeFrom(SerializeInput &in, const Type::TypeId type_id,
                               VarlenPool *pool = nullptr) {
    return Type::GetInstance(type_id)->DeserializeFrom(in, pool);
  }

  // Perform a shallow copy from a serialized varlen value to another serialized varlen value
  // Only support VARCHAR/VARBINARY
  inline static void ShallowCopyTo(char *dest, char *src,
                                   const Type::TypeId type_id, bool inlined, VarlenPool *src_pool) {
    Type::GetInstance(type_id)->DoShallowCopy(dest, src, inlined, src_pool);
  }

  // Access the raw variable length data
  inline const char *GetData() const {
    return type_->GetData(*this);
  }

  // Access the raw variable length data from a pointer pointed to a tuple storage
  inline static char *GetDataFromStorage(Type::TypeId type_id, char *storage) {
    switch (type_id) {
      case Type::VARCHAR:
      case Type::VARBINARY: {
        return Type::GetInstance(type_id)->GetData(storage);
      }
      default:
        throw Exception(EXCEPTION_TYPE_INCOMPATIBLE_TYPE,
                        "Invalid Type for getting raw data pointer");
    }
  }

  // Get the length of the variable length data
  inline uint32_t GetLength() const {
    return type_->GetLength(*this);
  }

  template <class T>
  inline T GetAs() const {
    return *reinterpret_cast<const T *>(&value_);
  }

  // Create a copy of this value
  inline Value Copy() const {
    return type_->Copy(*this);
  }

  inline Value CastAs(const Type::TypeId type_id) const {
    return type_->CastAs(*this, type_id);
  }

  // Get the element at a given index in this array
  inline Value GetElementAt(uint64_t idx) const {
    return type_->GetElementAt(*this, idx);
  }

  inline Type::TypeId GetElementType() const {
    return type_->GetElementType(*this);
  }

  // Does this value exist in this array?
  inline Value InList(const Value &object) const {
    return type_->InList(*this, object);
  }

  // For unordered_map
  struct equal_to {
    bool operator()(const Value &x, const Value &y) const {
      Value cmp(x.type_->CompareEquals(x, y));
      return cmp.IsTrue();
    }
  };

  template <class T>
  inline void hash_combine(std::size_t &seed, const T &v) const {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  }

  struct hash {
    size_t operator()(const Value &x) const { return x.type_->Hash(x); }
  };

  friend struct equal_to;
  friend struct hash_combine;
  friend struct hash;

  // Type classes
  friend class Type;
  friend class ArrayType;
  friend class BooleanType;
  friend class NumericType;
  friend class IntegerParentType;
  friend class TinyintType;
  friend class SmallintType;
  friend class IntegerType;
  friend class BigintType;
  friend class DecimalType;
  friend class VarlenType;
  friend class TimestampType;

  friend class ValueFactory;

 protected:
  // TODO: Pack allocated flag with the type id
  // The data type
  Type *type_;

  // The actual value item
  union Val {
    int8_t boolean;
    int8_t tinyint;
    int16_t smallint;
    int32_t integer;
    int64_t bigint;
    double decimal;
    uint64_t timestamp;
    char * varlen;
    const char * const_varlen;
    char * array;
  } value_;

  union{
   uint32_t len;
   Type::TypeId elem_type_id;
  } size_;

  bool manage_data_;
};

// ARRAY here to ease creation of templates
// TODO: Fix the representation for a null array
template <class T>
Value::Value(Type::TypeId type, const std::vector<T> &vals,
             Type::TypeId element_type)
    : Value(Type::ARRAY) {
  switch (type) {
    case Type::ARRAY:
      value_.array = (char *)&vals;
      size_.elem_type_id = element_type;
      break;
    default:
      throw Exception(EXCEPTION_TYPE_INCOMPATIBLE_TYPE,
                      "Invalid Type for constructor");
  }
}

}  // namespace type
}  // namespace peloton
