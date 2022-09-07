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
    ring = IoUringInterface::CreateLinuxIoUring(8, 0);
    ASSERT_NE(ring, nullptr);
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
