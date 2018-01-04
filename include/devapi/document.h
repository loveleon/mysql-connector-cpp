/*
 * Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
 *
 * The MySQL Connector/C++ is licensed under the terms of the GPLv2
 * <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>, like most
 * MySQL Connectors. There are special exceptions to the terms and
 * conditions of the GPLv2 as it is applied to this software, see the
 * FLOSS License Exception
 * <http://www.mysql.com/about/legal/licensing/foss-exception.html>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
 */

#ifndef MYSQLX_DOCUMENT_H
#define MYSQLX_DOCUMENT_H

/**
  @file
  Declaration of DbDoc and related classes.
*/

#include "common.h"
#include <memory>
#include <stdint.h>
#include <limits>
#include <vector>
#include <assert.h>

#undef min
#undef max

namespace mysqlx {

class Value;
class Field;
class DbDoc;
class DocResult;
class SessionSettings;


// Field class
// ===========

/*
  %Field object/values represent fields in a document.

  TODO: _fld suffix

  @ingroup devapi_res
*/

class Field
{
  string m_fld;

public:

  Field(const string &s) : m_fld(s)
  {}

  // TODO: is it auto generated?
  Field(string &&s) : m_fld(std::move(s))
  {}

  Field(const char *s) : m_fld(s)
  {}

  Field()
  {}

  operator const string&() const { return m_fld; }

  bool operator <(const Field &other) const
  {
    return m_fld < other.m_fld;
  }

  bool operator >(const Field &other) const
  {
    return m_fld > other.m_fld;
  }

  bool operator ==(const Field &other) const
  {
    return m_fld == other.m_fld;
  }

  bool operator !=(const Field &other) const
  {
    return m_fld != other.m_fld;
  }

};


// Document class
// ==============


/**
  Represents a collection of key-value pairs where value can be a scalar
  or another document.

  @note Internal document implementation is shared among DbDoc instances
  and thus using DbDoc objects should be cheap.

  @ingroup devapi_res
*/

class PUBLIC_API DbDoc
  : public common::Printable
{
  // TODO: move PUBLIC_API stuff to a detail class

  class INTERNAL Impl;

DLL_WARNINGS_PUSH

  std::shared_ptr<Impl> m_impl;

DLL_WARNINGS_POP

  INTERNAL DbDoc(const std::shared_ptr<Impl>&);

  const char* get_json() const;

public:

  /**
    Create null document instance.

    @note Null document is different from empty document that has
    no fields.
  */

  DbDoc() {}

  /**
    Creates DbDoc instance out of given JSON string description.
  */

  explicit DbDoc(const std::string&);


  /**
    Check if document is null
  */

  bool isNull() const { return NULL == m_impl.get(); }
  operator bool() const { return !isNull(); }


  /**
    Check if named field is a top-level field in the document.
  */

  virtual bool hasField(const Field&) const;


  /**
    Return Value::XXX constant that identifies type of value
    stored at given field.
  */

  virtual int  fieldType(const Field&) const;

  /**
    Return value of given field.
  */

  virtual const Value& operator[](const Field&) const;
  const Value& operator[](const char *name) const;
  const Value& operator[](const wchar_t *name) const;


  /**
    Print JSON description of the document.
  */

  virtual void print(std::ostream&) const;

  /**
    Iterator instance can iterate over (top-level) fields of a document.
    A new iterator is obtained from begin() method.

    @note Only one instance of an iterator can be used at a time (not
    thread safe!).
  */

  class Iterator;

  virtual Iterator begin();
  virtual Iterator end();

  friend Impl;
  friend DocResult;
  friend Value;
};


class PUBLIC_API DbDoc::Iterator
{
  DLL_WARNINGS_PUSH
  std::shared_ptr<DbDoc::Impl> m_impl;
  DLL_WARNINGS_POP
  bool         m_end;

public:

  Iterator& operator++();
  bool operator==(const Iterator&) const;
  bool operator!=(const Iterator &other) const { return !(*this == other); }
  const Field& operator*();

  friend DbDoc;
};


// Value class
// ===========

/**
  %Value object can store value of scalar type, string, array or document.

  Implicit conversions to and from corresponding C++ types are defined.
  If conversion to wrong type is attempted, an error is thrown. If Value
  object holds an array or document, then array elements or fields of
  the document can be accessed using operator[]. Array values can be used
  as STL containers.

  Only direct conversions of stored value to the corresponding C++ type
  are supported. There are no implicit number->string conversions etc.

  Values of type RAW can refer to a region of memory containing raw bytes.
  Such values are created from `bytes` and can by casted to `bytes` type.

  @note Value object copies the values it stores. Thus, after storing value
  in Value object, the original value can be destroyed without invalidating
  the copy. This includes RAW Values which hold a copy of bytes.

  @ingroup devapi_res
*/

class Value
  : public virtual common::Printable
  , protected common::Value
{
public:

  /**
    Possible types of values.

    @sa getType()
  */

  enum Type
  {
    VNULL,      ///< Null value
    UINT64,     ///< Unsigned integer
    INT64,      ///< Signed integer
    FLOAT,      ///< Float number
    DOUBLE,     ///< Double number
    BOOL,       ///< Boolean
    STRING,     ///< String
    DOCUMENT,   ///< Document
    RAW,        ///< Raw bytes
    ARRAY,      ///< Array of values
  };

  typedef std::vector<Value>::iterator iterator;
  typedef std::vector<Value>::const_iterator const_iterator;

  ///@name Value Constructors
  ///@{

  Value();  ///< Constructs Null value.
  Value(std::nullptr_t); ///< Constructs Null value.

  Value(const std::string &str);
  Value(std::string &&str);
  Value(const char *str) : Value(std::string(str)) {}

  Value(const std::wstring &str);
  Value(std::wstring &&str);
  Value(const wchar_t *str) : Value(std::wstring(str)) {}

  Value(const bytes&);
  Value(int64_t);
  Value(uint64_t);
  Value(float);
  Value(double);
  Value(bool);
  Value(const DbDoc& doc);

  Value(const std::initializer_list<Value> &list);
  template <typename Iterator>
  Value(Iterator begin_, Iterator end_);

  ///@}

  Value(common::Value &&other);
  Value(const common::Value &other);

  Value(const Value&) = default;

#ifdef HAVE_MOVE_CTORS

  Value(Value&&) = default;

#else

  // Note move ctor implemented using move assignment defined below.

  Value(Value &&other)
  {
    *this = std::move(other);
  }

#endif

  /*
    Note: These templates are needed to disambiguate constructor resolution
    for integer types.
  */

  template <
    typename T,
    typename std::enable_if<std::is_signed<T>::value>::type* = nullptr
  >
  Value(T x)
    : Value(static_cast<int64_t>(x))
  {}

  template <
    typename T,
    typename std::enable_if<std::is_unsigned<T>::value>::type* = nullptr
  >
  Value(T x)
    : Value(static_cast<uint64_t>(x))
  {}


  Value& operator=(const Value&) = default;

  /*
    Note: Move assignment is defined explicitly to avoid problems with
    virtual Printable base.
  */

  Value& operator=(Value&&);

  /*
    Assignment is implemented in terms of constructors: first an instance
    is created from the input data and then move assignment is used to place
    the result into this instance.
  */

  template<typename T>
  Value& operator=(T&& x)
  {
    try {
      *this = Value(std::forward<T>(x));
      return *this;
    }
    CATCH_AND_WRAP
  }

private:

  /*
    Note: Avoid implicit conversion from pointer types to bool.
    Without this declaration, Value(bool) constructor is invoked
    for pointer types. Here we declare and hide an explicit constructor
    for pointer types which prevents compiler to pick Value(bool).
  */

  template <typename T>
  Value(const T*);

public:

  /**
    @name Conversion to C++ Types

    Attempt to convert value of non-compatible type throws an error.
  */
  //@{

  operator int() const;
  operator unsigned() const;
  operator int64_t() const;
  operator uint64_t() const;

  operator float() const;
  operator double() const;

  operator bool() const;

  operator std::wstring() const;
  operator std::string() const;

  explicit operator bytes() const;
  operator DbDoc() const;


  template<typename T>
  T get() const;

  //@}


  bytes getRawBytes() const
  {
    try {
      size_t len;
      const byte *ptr = get_bytes(&len);
      return{ ptr, len };
    }
    CATCH_AND_WRAP
  }


  /**
    Return type of the value stored in this instance (or VNULL if no
    value is stored).
  */

  Type  getType() const;

  /// Convenience method for checking if value is null.

  bool isNull() const
  {
    return VNULL == getType();
  }

  /**
    Check if document value contains given (top-level) field.
    Throws error if this is not a document value.
  */

  bool  hasField(const Field&) const;

  /**
    If this value is not a document, throws error. Otherwise
    returns value of given field of the document.
  */

  const Value& operator[](const Field&) const;

  const Value& operator[](const char *name) const
  { return (*this)[Field(name)]; }

  const Value& operator[](const wchar_t *name) const
  { return (*this)[Field(name)]; }


  /**
    Access to elements of an array value.

    If non-array value is accessed like an array, an error is thrown.
  */
  //@{

  iterator begin();
  const_iterator begin() const;
  iterator end();
  const_iterator end() const;
  size_t   elementCount() const;

  const Value&  operator[](unsigned) const;
  const Value&  operator[](int pos) const
  {
    assert(pos >= 0);
    return operator[]((unsigned)pos);
  }

  //@}


  /// Print the value to a stream.

  void print(std::ostream &out) const
  {
    switch (m_type)
    {
    case DOC: out << m_doc; return;
    case ARR: out << "<array with " << elementCount() << " element(s)>"; return;
    default:  common::Value::print(out); return;
    }
  }

protected:

  enum { VAL, ARR, DOC } m_type = VAL;

  void check_type(Type t) const
  {
    if (getType() != t)
      throw Error("Invalid value type");
  }

  bool is_expr() const
  {
    return VAL == m_type && common::Value::EXPR == common::Value::get_type();
  }

  void set_as_expr()
  {
    common::Value::m_type = common::Value::EXPR;
  }

  /*
    TODO: Instead extend common::Value with array and document types. Requires
    moving DbDoc code to the common layer.
  */

  typedef std::vector<Value> Array;

  DLL_WARNINGS_PUSH

  DbDoc  m_doc;

  // Note: shared with other Value instances for the same array.
  std::shared_ptr<Array>  m_arr;

  DLL_WARNINGS_POP

public:

  friend mysqlx::string;
  friend SessionSettings;
  friend DbDoc;
  template <class Base> friend class Bind_parameters;

  struct INTERNAL Access;
  friend Access;
};

static const Value nullvalue;


inline
Value& Value::operator=(Value &&other)
{
  m_type = other.m_type;

  switch (m_type)
  {
  case VAL:
    common::Value::operator=(std::move(other));
    break;

  case DOC: m_doc = std::move(other.m_doc); break;
  case ARR: m_arr = std::move(other.m_arr); break;

  default: break;
  }

  return *this;
}


namespace internal {

/*
  Helper class to identify usage of expressions
*/

class Expression
: public mysqlx::Value
{
 Expression()
 {}

 template <typename V>
 Expression(V&& val)
 : Value(std::forward<V>(val))
 {
   set_as_expr();
 }

 template <typename V>
 Expression(const V& val)
 : Value(val)
 {
   set_as_expr();
 }

 friend Expression expr(string&& s);
 friend Expression expr(const string& s);
};


/**
  Function which indicates that a given string should be treated
  as expression.

  If `s` is a string value, then in contexts where values are
  expected, `expr(s)` treats `s` as a DevAPI expression. For
  example statement

  table.select("foo > 1").execute();

  returns the string `"foo  1"` for each row in the table while

  table.select(expr("foo > 1")).execute();

  returns true/false, depending on the value of the expression.

  @ingroup devapi
*/

inline
mysqlx::internal::Expression expr(string&& e)
{
  return std::forward<string>(e);
}

inline
mysqlx::internal::Expression expr(const string& e)
{
  return e;
}

}  // internal


using internal::expr;


inline
Value::Type Value::getType() const
{
  switch (m_type)
  {
  case ARR: return ARRAY;
  case DOC: return DOCUMENT;
  case VAL:
    switch (common::Value::get_type())
    {
    case common::Value::VNULL:    return VNULL;
    case common::Value::UINT64:   return UINT64;
    case common::Value::INT64:    return INT64;
    case common::Value::FLOAT:    return FLOAT;
    case common::Value::DOUBLE:   return DOUBLE;
    case common::Value::BOOL:     return BOOL;
    case common::Value::STRING:   return STRING;
    case common::Value::WSTRING:  return STRING;
    case common::Value::RAW:      return RAW;
    case common::Value::EXPR:     return STRING;
    case common::Value::JSON:     return DOCUMENT;
    default: break;
    }
  default: assert(false); return VNULL; // quiet compiler warning
  }
}


inline
const Value& DbDoc::operator[](const char *name) const
{
  return (*this)[Field(name)];
}

inline
const Value& DbDoc::operator[](const wchar_t *name) const
{
  return (*this)[Field(name)];
}


/*
  Value type conversions
  ----------------------
  TODO: more informative errors
*/


inline
Value::Value(const std::initializer_list<Value> &list)
  : m_type(ARR)
{
  try {
    m_arr = std::make_shared<Array>(list);
  }
  CATCH_AND_WRAP
}

template <typename Iterator>
inline
Value::Value(Iterator begin_, Iterator end_)
  : m_type(ARR)
{
  try {
    m_arr = std::make_shared<Array>(begin_, end_);
  }
  CATCH_AND_WRAP
}


inline
Value::Value(common::Value &&other)
try
  : common::Value(std::move(other))
{}
CATCH_AND_WRAP

inline
Value::Value(const common::Value &other)
try
  : common::Value(other)
{}
CATCH_AND_WRAP


inline Value::Value()
{}

inline Value::Value(std::nullptr_t)
{}

inline Value::Value(int64_t val)
try
  : common::Value(val)
{}
CATCH_AND_WRAP

inline Value::Value(uint64_t val)
try
  : common::Value(val)
{}
CATCH_AND_WRAP

template<>
inline
int Value::get<int>() const
{
  try {
    int64_t val = get_sint();
    if (val > std::numeric_limits<int>::max())
      throw Error("Numeric conversion overflow");
    if (val < std::numeric_limits<int>::min())
      throw Error("Numeric conversion overflow");

    return (int)val;
  }
  CATCH_AND_WRAP
}

inline
Value::operator int() const
{
  return get<int>();
}


template<>
inline unsigned Value::get<unsigned>() const
{
  try {
    uint64_t val = get_uint();
    if (val > std::numeric_limits<unsigned>::max())
      throw Error("Numeric conversion overflow");

    return (unsigned)val;
  }
  CATCH_AND_WRAP
}

inline
Value::operator unsigned() const
{
  return get<unsigned>();
}


template<>
inline int64_t Value::get<int64_t>() const
{
  try {
    return get_sint();
  }
  CATCH_AND_WRAP
}

inline
Value::operator int64_t() const
{
  return get<int64_t>();
}


template<>
inline uint64_t Value::get<uint64_t>() const
{
  try {
    return get_uint();
  }
  CATCH_AND_WRAP
}

inline
Value::operator uint64_t() const
{
  return get<uint64_t>();
}


inline Value::Value(float val)
try
  : common::Value(val)
{}
CATCH_AND_WRAP

template<>
inline
float Value::get<float>() const
{
  try {
    return get_float();
  }
  CATCH_AND_WRAP
}

inline
Value::operator float() const
{
  return get<float>();
}


inline Value::Value(double val)
try
  : common::Value(val)
{}
CATCH_AND_WRAP

template<>
inline
double Value::get<double>() const
{
  try {
    return get_double();
  }
  CATCH_AND_WRAP
}

inline
Value::operator double() const
{
  return get<double>();
}


inline Value::Value(bool val)
try
  : common::Value(val)
{}
CATCH_AND_WRAP


template<>
inline
bool Value::get<bool>() const
{
  try {
    return get_bool();
  }
  CATCH_AND_WRAP
}

inline
Value::operator bool() const
{
  return get<bool>();
}


inline Value::Value(const DbDoc &doc)
try
  : m_type(DOC)
  , m_doc(doc)
{}
CATCH_AND_WRAP


inline Value::Value(const std::string &val)
try
  : common::Value(val)
{}
CATCH_AND_WRAP

inline Value::Value(std::string &&val)
try
  : common::Value(std::move(val))
{}
CATCH_AND_WRAP


inline Value::Value(const std::wstring &val)
try
  : common::Value(val)
{}
CATCH_AND_WRAP

inline Value::Value(std::wstring &&val)
try
  : common::Value(std::move(val))
{}
CATCH_AND_WRAP

template<>
inline
std::wstring Value::get<std::wstring>() const
{
  try {
    return get_wstring();
  }
  CATCH_AND_WRAP
}

inline
Value::operator std::wstring() const
{
  return get<std::wstring>();
}


template<>
inline
std::string Value::get<std::string>() const
{
  try {
    return get_string();
  }
  CATCH_AND_WRAP
}

inline
Value::operator std::string() const
{
  return get<std::string>();
}


template<>
inline
string Value::get<string>() const
{
  return get<std::wstring>();
}

// Conversion to mysqlx::string is done via its ctor from common::Value.

inline
mysqlx::string::string(const Value &val)
  : string((const common::Value&)val)
{}


inline Value::Value(const bytes &data)
try
  : common::Value(data.begin(), data.length())
{}
CATCH_AND_WRAP

template<>
inline
bytes Value::get<bytes>() const
{
  return getRawBytes();
}

inline
Value::operator bytes() const
{
  return get<bytes>();
}


template<>
inline
DbDoc Value::get<DbDoc>() const
{
  check_type(DOCUMENT);
  return m_doc;
}

inline
Value::operator DbDoc() const
{
  return get<DbDoc>();
}


inline
bool Value::hasField(const Field &fld) const
{
  check_type(DOCUMENT);
  return m_doc.hasField(fld);
}

inline
const Value& Value::operator[](const Field &fld) const
{
  check_type(DOCUMENT);
  return m_doc[fld];
}

inline
int DbDoc::fieldType(const Field &fld) const
{
  return (*this)[fld].getType();
}

// Array access


inline
Value::iterator Value::begin()
{
  if (ARR != m_type)
    throw Error("Attempt to iterate over non-array value");
  return m_arr->begin();
}

inline
Value::const_iterator Value::begin() const
{
  if (ARR != m_type)
    throw Error("Attempt to iterate over non-array value");
  return m_arr->begin();
}

inline
Value::iterator Value::end()
{
  if (ARR != m_type)
    throw Error("Attempt to iterate over non-array value");
  return m_arr->end();
}

inline
Value::const_iterator Value::end() const
{
  if (ARR != m_type)
    throw Error("Attempt to iterate over non-array value");
  return m_arr->end();
}

inline
const Value& Value::operator[](unsigned pos) const
{
  check_type(ARRAY);
  return m_arr->at(pos);
}

inline
size_t Value::elementCount() const
{
  check_type(ARRAY);
  return m_arr->size();
}


}  // mysqlx


#endif
