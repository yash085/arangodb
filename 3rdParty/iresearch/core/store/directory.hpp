////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2016 by EMC Corporation, All Rights Reserved
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is EMC Corporation
///
/// @author Andrey Abramov
/// @author Vasiliy Nabatchikov
////////////////////////////////////////////////////////////////////////////////

#ifndef IRESEARCH_DIRECTORY_H
#define IRESEARCH_DIRECTORY_H

#include "data_input.hpp"
#include "data_output.hpp"
#include "utils/attributes_provider.hpp"
#include "utils/memory.hpp"
#include "utils/noncopyable.hpp"
#include "utils/string.hpp"

#include <ctime>
#include <vector>

NS_ROOT

//////////////////////////////////////////////////////////////////////////////
/// @struct index_lock
/// @brief an interface for abstract resource locking
//////////////////////////////////////////////////////////////////////////////
struct IRESEARCH_API index_lock : private util::noncopyable {
  DECLARE_IO_PTR(index_lock, unlock);
  DECLARE_FACTORY(index_lock);

  static const size_t LOCK_WAIT_FOREVER = integer_traits<size_t>::const_max;

  ////////////////////////////////////////////////////////////////////////////
  /// @brief destructor 
  ////////////////////////////////////////////////////////////////////////////
  virtual ~index_lock();

  ////////////////////////////////////////////////////////////////////////////
  /// @brief checks whether the guarded resource is locked
  /// @param[out] true if resource is already locked
  /// @returns call success
  ////////////////////////////////////////////////////////////////////////////
  virtual bool is_locked(bool& result) const NOEXCEPT = 0;

  ////////////////////////////////////////////////////////////////////////////
  /// @brief locks the guarded resource
  /// @returns true on success
  /// @exceptions any exception thrown by the underlying lock
  ////////////////////////////////////////////////////////////////////////////
  virtual bool lock() = 0;

  ////////////////////////////////////////////////////////////////////////////
  /// @brief tries to lock the guarded resource within the specified amount of
  ///        time
  /// @param[in] wait_timeout timeout between different locking attempts
  /// @returns true on success
  ////////////////////////////////////////////////////////////////////////////
  bool try_lock(size_t wait_timeout = 1000) NOEXCEPT;

  ////////////////////////////////////////////////////////////////////////////
  /// @brief unlocks the guarded resource
  /// @returns call success
  ////////////////////////////////////////////////////////////////////////////
  virtual bool unlock() NOEXCEPT = 0;
}; // unique_lock

//////////////////////////////////////////////////////////////////////////////
/// @enum IOAdvice
/// @brief defines access patterns for data in a directory
//////////////////////////////////////////////////////////////////////////////
enum class IOAdvice : uint32_t {
  ////////////////////////////////////////////////////////////////////////////
  /// @brief Indicates that caller has no advice to give about its access
  ///        pattern for the data
  ////////////////////////////////////////////////////////////////////////////
  NORMAL = 0,

  ////////////////////////////////////////////////////////////////////////////
  /// @brief Indicates that caller expects to access data sequentially
  ////////////////////////////////////////////////////////////////////////////
  SEQUENTIAL = 1,

  ////////////////////////////////////////////////////////////////////////////
  /// @brief Indicates that caller expects to access data in random order
  ////////////////////////////////////////////////////////////////////////////
  RANDOM = 2,

  ////////////////////////////////////////////////////////////////////////////
  /// @brief Indicates that caller expects that data will not be accessed
  ///        in the near future
  ////////////////////////////////////////////////////////////////////////////
  READONCE = 4,

  ////////////////////////////////////////////////////////////////////////////
  /// @brief convenience value for READONCE | SEQUENTIAL
  ///        explicitly required for MSVC2013
  ////////////////////////////////////////////////////////////////////////////
  READONCE_SEQUENTIAL = 5,

  ////////////////////////////////////////////////////////////////////////////
  /// @brief convenience value for READONCE | RANDOM
  ///        explicitly required for MSVC2013
  ////////////////////////////////////////////////////////////////////////////
  READONCE_RANDOM = 6,
}; // IOAdvice

ENABLE_BITMASK_ENUM(IOAdvice); // enable bitmap operations on the enum

//////////////////////////////////////////////////////////////////////////////
/// @struct directory
/// @brief represents a flat directory of write once/read many files
//////////////////////////////////////////////////////////////////////////////
struct IRESEARCH_API directory 
  : public util::attribute_store_provider,
    private util::noncopyable {
  typedef std::function<bool(std::string& name)> visitor_f;

  DECLARE_PTR(directory);
  DECLARE_FACTORY(directory);

  ////////////////////////////////////////////////////////////////////////////
  /// @brief destructor 
  ////////////////////////////////////////////////////////////////////////////
  virtual ~directory();

  ////////////////////////////////////////////////////////////////////////////
  /// @brief closes directory
  ////////////////////////////////////////////////////////////////////////////
  virtual void close() NOEXCEPT = 0;

  ////////////////////////////////////////////////////////////////////////////
  /// @brief opens output stream associated with the file
  /// @param[in] name name of the file to open
  /// @returns output stream associated with the file with the specified name
  ////////////////////////////////////////////////////////////////////////////
  virtual index_output::ptr create(const std::string& name) NOEXCEPT = 0;

  ////////////////////////////////////////////////////////////////////////////
  /// @brief check whether the file specified by the given name exists
  /// @param[out] true if file already exists
  /// @param[in] name name of the file
  /// @returns call success
  ////////////////////////////////////////////////////////////////////////////
  virtual bool exists(
    bool& result, const std::string& name
  ) const NOEXCEPT = 0;

  ////////////////////////////////////////////////////////////////////////////
  /// @brief returns the length of the file specified by the given name
  /// @param[out] length of the file specified by the name
  /// @param[in] name name of the file
  /// @returns call success
  ////////////////////////////////////////////////////////////////////////////
  virtual bool length(
    uint64_t& result, const std::string& name
  ) const NOEXCEPT = 0;

  ////////////////////////////////////////////////////////////////////////////
  /// @brief creates an index level lock with the specified name 
  /// @param[in] name name of the lock
  /// @returns lock hande
  ////////////////////////////////////////////////////////////////////////////
  virtual index_lock::ptr make_lock(const std::string& name) NOEXCEPT = 0;

  ////////////////////////////////////////////////////////////////////////////
  /// @brief returns modification time of the file specified by the given name
  /// @param[out] file last modified time
  /// @param[in] name name of the file
  /// @returns call success
  ////////////////////////////////////////////////////////////////////////////
  virtual bool mtime(
    std::time_t& result, const std::string& name
  ) const NOEXCEPT = 0;

  ////////////////////////////////////////////////////////////////////////////
  /// @brief opens input stream associated with the existing file
  /// @param[in] name   name of the file to open
  /// @returns input stream associated with the file with the specified name
  ////////////////////////////////////////////////////////////////////////////
  virtual index_input::ptr open(
    const std::string& name,
    IOAdvice advice
  ) const NOEXCEPT = 0;

  ////////////////////////////////////////////////////////////////////////////
  /// @brief removes the file specified by the given name from directory
  /// @param[in] name name of the file
  /// @returns true if file has been removed
  ////////////////////////////////////////////////////////////////////////////
  virtual bool remove(const std::string& name) NOEXCEPT = 0;

  ////////////////////////////////////////////////////////////////////////////
  /// @brief renames the 'src' file to 'dst'
  /// @param[in] src initial name of the file
  /// @param[in] dst final name of the file
  /// @returns true if file has been renamed
  ////////////////////////////////////////////////////////////////////////////
  virtual bool rename(
    const std::string& src, const std::string& dst
  ) NOEXCEPT = 0;

  ////////////////////////////////////////////////////////////////////////////
  /// @brief ensures that all modification have been sucessfully persisted
  /// @param[in] name name of the file
  /// @returns call success
  ////////////////////////////////////////////////////////////////////////////
  virtual bool sync(const std::string& name) NOEXCEPT = 0;

  ////////////////////////////////////////////////////////////////////////////
  /// @brief applies the specified 'visitor' to every filename in a directory
  /// @param[in] visitor to be applied
  /// @returns 'false' if visitor has returned 'false', 'true' otherwise
  /// @exceptions any exception thrown by the visitor
  ////////////////////////////////////////////////////////////////////////////
  virtual bool visit(const visitor_f& visitor) const = 0;
}; // directory

NS_END

#endif