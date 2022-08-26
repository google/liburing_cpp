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

#include <liburing_cpp/IoUring.h>
#include <string.h>

#include <algorithm>
#include <iostream>
#include <memory>

#include "liburing.h"
#include "liburing/io_uring.h"

namespace io_uring_cpp {
IoUring::IoUring(struct io_uring r) : ring(r) {}

IoUring::IoUring(IoUring&& rhs) {
  ring = rhs.ring;
  memset(&rhs.ring, 0, sizeof(rhs.ring));
}

IoUring& IoUring::operator=(IoUring&& rhs) {
  std::swap(ring, rhs.ring);
  return *this;
}

const char* Errno::ErrMsg() {
  if (error_code == 0) {
    return nullptr;
  }
  return strerror(error_code);
}

std::optional<IoUring> IoUring::Create(int queue_depth, int flags) {
  struct io_uring ring {};
  const auto err = io_uring_queue_init(queue_depth, &ring, flags);
  if (err) {
    errno = -err;
    return {};
  }
  return IoUring(ring);
}

std::ostream& operator<<(std::ostream& out, Errno err) {
  out << err.ErrCode() << ", " << err.ErrMsg();
  return out;
}

std::unique_ptr<IoUring> IoUring::CreatePtr(int queue_depth, int flags) {
  auto ring = Create(queue_depth, flags);
  if (!ring.has_value()) {
    return nullptr;
  }
  return std::make_unique<IoUring>(std::move(ring.value()));
}

IoUringSQE IoUring::GetSQE() { return IoUringSQE{io_uring_get_sqe(&ring)}; }

IoUringSubmitResult IoUring::Submit() {
  return IoUringSubmitResult{io_uring_submit(&ring)};
}

Result<Errno, struct io_uring_cqe*> IoUring::PopCQE() {
  struct io_uring_cqe* ptr{};
  const auto ret = io_uring_wait_cqe(&ring, &ptr);
  if (ret != 0) {
    return {Errno(ret)};
  }
  return {ptr};
}

Result<Errno, struct io_uring_cqe*> IoUring::PeekCQE() {
  struct io_uring_cqe* ptr{};
  const auto ret = io_uring_peek_cqe(&ring, &ptr);
  if (ret != 0) {
    return {Errno(ret)};
  }
  return {ptr};
}

template <typename T>
bool IsZeroInitialized(const T& val) {
  auto begin = reinterpret_cast<const char*>(&val);
  auto end = begin + sizeof(val);
  return std::all_of(begin, end, [](const auto& a) { return a == 0; });
}

IoUring::~IoUring() {
  if (!IsZeroInitialized(ring)) {
    io_uring_queue_exit(&ring);
  }
}

}  // namespace io_uring_cpp
