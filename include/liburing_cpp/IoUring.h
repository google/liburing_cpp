//
// Copyright (C) 2022 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef __IO_URING_CPP_H
#define __IO_URING_CPP_H

#include <errno.h>
#include <string.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <variant>

#include "IoUringCQE.h"
#include "IoUringSQE.h"

namespace io_uring_cpp {

template <typename Err, typename Res>
struct [[nodiscard]] Result : public std::variant<Err, Res> {
  constexpr bool IsOk() const { return std::holds_alternative<Res>(*this); }
  constexpr bool IsErr() const { return std::holds_alternative<Err>(*this); }
  constexpr Err GetError() const { return std::get<Err>(*this); }
  constexpr Res GetResult() const { return std::get<Res>(*this); }
};

class IoUringInterface {
 public:
  virtual ~IoUringInterface() {}
  // Append a submission entry into this io_uring. This does not submit the
  // operation to the kernel. For that, call |IoUringInterface::Submit()|
  virtual IoUringSQE PrepRead(int fd, void *buf, unsigned nbytes,
                              uint64_t offset) = 0;
  // Caller is responsible for making sure the input memory is available until
  // this write operation completes.
  virtual IoUringSQE PrepWrite(int fd, const void *buf, unsigned nbytes,
                               uint64_t offset) = 0;

  // Ring operations
  virtual IoUringSubmitResult Submit() = 0;
  virtual Result<Errno, IoUringCQE> PopCQE() = 0;
  virtual Result<Errno, IoUringCQE> PeekCQE() = 0;

  static std::unique_ptr<IoUringInterface> CreateLinuxIoUring(int queue_depth,
                                                              int flags);
};

}  // namespace io_uring_cpp

#endif
