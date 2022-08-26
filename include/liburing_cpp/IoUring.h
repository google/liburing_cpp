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
#include <liburing.h>
#include <string.h>

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <optional>
#include <variant>

#include "liburing/io_uring.h"

namespace io_uring_cpp {

struct [[nodiscard]] IoUringSubmitResult {
  constexpr bool IsOk() const noexcept { return ret > 0; }
  const char *ErrMsg() const noexcept {
    if (IsOk()) {
      return nullptr;
    }
    return strerror(-ret);
  }
  constexpr auto ErrCode() const { return std::min(ret, 0); }
  constexpr auto EntriesSubmitted() const { return std::max(ret, 0); }

  int ret;
};

struct [[nodiscard]] Errno {
  constexpr Errno(int ret) : error_code(ret) {
    error_code = std::abs(error_code);
  }
  int error_code;
  constexpr int ErrCode() { return error_code; }
  const char *ErrMsg();
};

std::ostream &operator<<(std::ostream &, Errno err);

struct IoUringSQE {
  IoUringSQE &SetData(void *data) {
    io_uring_sqe_set_data(sqe, data);
    return *this;
  }
  IoUringSQE &SetFlags(unsigned int flags) {
    io_uring_sqe_set_flags(sqe, flags);
    return *this;
  }
  IoUringSQE &PrepReadv(int fd, const struct iovec *iovecs, unsigned nr_vecs,
                        __u64 offset) {
    io_uring_prep_readv(sqe, fd, iovecs, nr_vecs, offset);
    return *this;
  }
  IoUringSQE &PrepRead(int fd, void *buf, unsigned nbytes, __u64 offset) {
    io_uring_prep_read(sqe, fd, buf, nbytes, offset);
    return *this;
  }
  IoUringSQE &PrepWrite(int fd, const void *buf, unsigned nbytes,
                        __u64 offset) {
    io_uring_prep_write(sqe, fd, buf, nbytes, offset);
    return *this;
  }
  IoUringSQE &PrepWritev(int fd, const struct iovec *iovecs, unsigned nr_vecs,
                         __u64 offset) {
    io_uring_prep_writev(sqe, fd, iovecs, nr_vecs, offset);
    return *this;
  }
  io_uring_sqe *sqe;
};

template <typename Err, typename Res>
struct [[nodiscard]] Result : public std::variant<Err, Res> {
  constexpr bool IsOk() const { return std::holds_alternative<Res>(*this); }
  constexpr bool IsErr() const { return std::holds_alternative<Err>(*this); }
  constexpr Err GetError() const { return std::get<Err>(*this); }
  constexpr Res GetResult() const { return std::get<Res>(*this); }
};
class IoUringInterface {
 public:
  virtual IoUringSQE GetSQE() = 0;
  virtual IoUringSubmitResult Submit() = 0;
  virtual Result<Errno, struct io_uring_cqe *> PopCQE() = 0;
  virtual Result<Errno, struct io_uring_cqe *> PeekCQE() = 0;
};

class IoUring final : public IoUringInterface {
 public:
  static std::optional<IoUring> Create(int queue_depth, int flags);
  static std::unique_ptr<IoUring> CreatePtr(int queue_depth, int flags);
  ~IoUring();
  IoUring(const IoUring &) = delete;
  IoUring(IoUring &&);
  IoUring &operator=(IoUring &&);

  IoUringSQE GetSQE() override;
  IoUringSubmitResult Submit() override;
  Result<Errno, struct io_uring_cqe *> PopCQE() override;
  Result<Errno, struct io_uring_cqe *> PeekCQE() override;

 private:
  IoUring(struct io_uring);
  struct io_uring ring {};
};

}  // namespace io_uring_cpp

#endif
