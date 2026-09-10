#include <gtest/gtest.h>

TEST(GoogleTest, Init) {
  EXPECT_EQ(1, 1);
}
// TODO: добавить юнит тестрование конструктора
// TODO: как то придумать как посимулировать какие нибудь базовые вещи 


int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}