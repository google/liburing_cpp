#include <fcntl.h>
#include <gtest/gtest.h>
#include <liburing_cpp/IoUring.h>
#include <unistd.h>

#include <array>

using namespace io_uring_cpp;

class IoUringTest : public ::testing::Test {
 public:
  void SetUp() override {
    ring = IoUring::Create(8, 0);
    ASSERT_TRUE(ring.has_value());
  }
  std::optional<IoUring> ring;
};

TEST_F(IoUringTest, SmallRead) {
  int fd = open("/proc/self/maps", O_RDONLY);
  std::array<char, 1024> buf{};
  ring->GetSQE().PrepRead(fd, buf.data(), buf.size(), 0);
  const auto ret = ring->Submit();
  ASSERT_TRUE(ret.IsOk()) << ret.ErrMsg();
  const auto cqe = ring->PopCQE();
  ASSERT_TRUE(cqe.IsOk()) << cqe.GetError();
  std::cout << buf.data();
}