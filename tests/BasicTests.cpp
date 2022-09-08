#include <fcntl.h>
#include <gtest/gtest.h>
#include <liburing_cpp/IoUring.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include <algorithm>
#include <array>

using namespace io_uring_cpp;

class IoUringTest : public ::testing::Test {
 public:
  void SetUp() override {
    ring = IoUringInterface::CreateLinuxIoUring(4096, 0);
    ASSERT_NE(ring, nullptr);
  }
  void Write(int fd, const void* data, const size_t len) {
    const auto buf = static_cast<const char*>(data);
    constexpr size_t IO_BATCH_SIZE = 4096;
    size_t i = 0;
    for (i = 0; i < len; i += IO_BATCH_SIZE) {
      const auto sqe = ring->PrepWrite(fd, buf + i, IO_BATCH_SIZE, i);
      ASSERT_TRUE(sqe.IsOk());
    }
    const auto bytes_remaining = len - i;
    if (bytes_remaining) {
      ASSERT_TRUE(ring->PrepWrite(fd, buf + i, bytes_remaining, i).IsOk());
    }
    const auto ret = ring->Submit();
    ASSERT_TRUE(ret.IsOk()) << ret.ErrMsg();
    for (size_t i = (len + IO_BATCH_SIZE - 1) / IO_BATCH_SIZE; i > 0; i--) {
      const auto cqe = ring->PopCQE();
      ASSERT_TRUE(cqe.IsOk());
      ASSERT_GT(cqe.GetResult().res, 0);
    }
  }
  std::unique_ptr<IoUringInterface> ring;
};

TEST_F(IoUringTest, SmallRead) {
  int fd = open("/proc/self/maps", O_RDONLY);
  std::array<char, 1024> buf{};
  const auto sqe = ring->PrepRead(fd, buf.data(), buf.size(), 0);
  ASSERT_TRUE(sqe.IsOk()) << "Submission Queue is full!";
  const auto ret = ring->Submit();
  ASSERT_TRUE(ret.IsOk()) << ret.ErrMsg();
  const auto cqe = ring->PopCQE();
  ASSERT_TRUE(cqe.IsOk()) << cqe.GetError();
  ASSERT_GT(cqe.GetResult().res, 0);
}

TEST_F(IoUringTest, SmallWrite) {
  auto fp = tmpfile();
  int fd = fileno(fp);
  std::string buffer(256, 'A');
  const auto sqe = ring->PrepWrite(fd, buffer.data(), buffer.size(), 0);
  ASSERT_TRUE(sqe.IsOk()) << "Submission Queue is full!";
  const auto ret = ring->Submit();
  ASSERT_TRUE(ret.IsOk()) << ret.ErrMsg();
  const auto cqe = ring->PopCQE();
  ASSERT_TRUE(cqe.IsOk()) << cqe.GetError();

  pread(fd, buffer.data(), buffer.size(), 0);
  ASSERT_TRUE(std::all_of(buffer.begin(), buffer.end(), [](const auto& a) {
    return a == 'A';
  })) << buffer;
}

TEST_F(IoUringTest, ChunkedWrite) {
  auto fp = tmpfile();
  int fd = fileno(fp);
  std::string buffer(16 * 1024 * 1024, 'A');
  ASSERT_NO_FATAL_FAILURE(Write(fd, buffer.data(), buffer.size()));

  pread(fd, buffer.data(), buffer.size(), 0);
  ASSERT_TRUE(std::all_of(buffer.begin(), buffer.end(), [](const auto& a) {
    return a == 'A';
  })) << buffer;
}