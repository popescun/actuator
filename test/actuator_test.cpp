/**
 * @brief Test the actuator concept.
 *
 * @file actuator_test.cpp
 * @author Nicu Popescu
 * @date 2021
 */
#include <actuator.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace untangle::test {

class shape
{
public:
  virtual ~shape()= default;
  virtual void rotate(int angle) const = 0;
  virtual void test_vr_no_args() const = 0;
  virtual void test_vr_args(int x, int y) const = 0;
};

class triangle : public shape
{
public:
  void rotate(int angle) const override {
    std::cout << "triangle::rotate " << angle << std::endl;
  }

  void height_in(int h) {
    std::cout << "triangle::height_in" << std::endl; height = h;
  }
  int height_out() const {
    std::cout << "triangle::height_out" << std::endl; return height;
  }

  void test_vr_no_args() const override {
    std::cout << "triangle::test_vr" << std::endl;
  }

  void test_vr_args(int x, int y) const override {
    std::cout << "triangle::test_vr_args " << x << ", "<< y <<std::endl;
  }

private:
  int height{0};
};

class triangle_mock : public shape {
public:
  MOCK_METHOD(void, rotate, (int), (const, override));
  MOCK_METHOD(void, test_vr_no_args, (), (const, override));
  MOCK_METHOD(void, test_vr_args, (int, int), (const, override));
};

class circle : public shape
{
public:
  void rotate(int angle) const override {
    std::cout << "circle::rotate " << angle << std::endl;
  }

  void height_in(int h) {
    std::cout << "circle::height_in" << std::endl; height = h;
  }

  int height_out() const {
    std::cout << "circle::height_out" << std::endl; return height;
  }

  void test_vr_no_args() const override {
    std::cout << "circle::test_vr" << std::endl;
  }
  void test_vr_args(int x, int y) const override {
    std::cout << "circle::test_vr_args " << x << ", "<< y <<std::endl;
  }

private:
  int height{0};
};

class circle_mock : public shape {
public:
  MOCK_METHOD(void, rotate, (int), (const, override));
  MOCK_METHOD(void, test_vr_no_args, (), (const, override));
  MOCK_METHOD(void, test_vr_args, (int, int), (const, override));
};

class square : public shape
{
public:
  void rotate(int angle) const override {
    std::cout << "square::rotate " << angle << std::endl;
  }

  void height_in(int h) {
    std::cout << "square::height_in" << std::endl; height = h;
  }

  int height_out() const {
    std::cout << "square::height_out" << std::endl; return height;
  }

  void test_vr_no_args() const override {
    std::cout << "square::test_vr" << std::endl;
  }

  void test_vr_args(int x, int y) const override {
    std::cout << "square::test_vr_args " << x << ", "<< y <<std::endl;
  }

private:
  int height{0};
};

class square_mock : public shape {
public:
  MOCK_METHOD(void, rotate, (int), (const, override));
  MOCK_METHOD(void, test_vr_no_args, (), (const, override));
  MOCK_METHOD(void, test_vr_args, (int, int), (const, override));
};

void rotate(int angle) { std::cout << "function::rotate " << angle << std::endl; }

//! [test_polymorphism1]
void rotate_shapes(const std::vector<shape*>& shapes, int angle)
{
  for (const auto& s : shapes)
  {
    s->rotate(angle);
  }
}

TEST(test_actuator, test_polymorphism_named_actions) {
  //! [test_polymorphism_named_actions2]
  auto t = std::make_shared<triangle_mock>();
  const auto c = std::make_shared<circle_mock>();
  const auto s = std::make_shared<square_mock>();

  // using polymorphism
  std::vector<shape*> shapes;
  shapes.push_back(t.get());
  shapes.push_back(c.get());
  shapes.push_back(s.get());

  EXPECT_CALL(*t, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(*c, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(*s, rotate(testing::_)).WillOnce(testing::Return());
  rotate_shapes(shapes, 10);
  testing::Mock::VerifyAndClearExpectations(t.get());
  testing::Mock::VerifyAndClearExpectations(c.get());
  testing::Mock::VerifyAndClearExpectations(s.get());

  // using named actuator
  auto action1 = untangle::bind(t, &triangle_mock::rotate);
  auto action2 = untangle::bind(c, &circle_mock::rotate);
  auto action3 = untangle::bind(s, &square_mock::rotate);

  auto actuator_rotate = untangle::connect(std::make_pair(std::string("triangle"), &action1),
                                           std::make_pair(std::string("circle"), &action2),
                                           std::make_pair(std::string("square"), &action3));

  actuator_rotate.remove("circle");
  EXPECT_FALSE(actuator_rotate.has_action("circle"));

  actuator_rotate.add("circle", &action2);
  EXPECT_TRUE(actuator_rotate.has_action("circle"));

  EXPECT_CALL(*c, rotate(20)).WillOnce(testing::Return());
  actuator_rotate.invokeAction("circle", 20);
  testing::Mock::VerifyAndClearExpectations(c.get());

  // invalidate triangle: invokeAction must detect the dead binding and erase it
  EXPECT_CALL(*t, rotate(testing::_)).Times(0);
  auto* raw_t = t.get();
  t.reset();
  actuator_rotate.invokeAction("triangle", 20);
  EXPECT_FALSE(actuator_rotate.has_action("triangle"));
  testing::Mock::VerifyAndClearExpectations(raw_t);
  //! [test_polymorphism_named_actions2]
}

TEST(test_actuator, test_polymorphism_using_shared_pointers) {
  const auto t = std::make_shared<triangle_mock>();
  const auto c = std::make_shared<circle_mock>();
  const auto s = std::make_shared<square_mock>();

  // using polymorphism
  std::vector<shape*> shapes;
  shapes.push_back(t.get());
  shapes.push_back(c.get());
  shapes.push_back(s.get());

  EXPECT_CALL(*t, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(*c, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(*s, rotate(testing::_)).WillOnce(testing::Return());
  rotate_shapes(shapes, 10);
  testing::Mock::VerifyAndClearExpectations(t.get());
  testing::Mock::VerifyAndClearExpectations(c.get());
  testing::Mock::VerifyAndClearExpectations(s.get());

  // using actuator
  EXPECT_CALL(*t, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(*c, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(*s, rotate(testing::_)).WillOnce(testing::Return());
  auto action1 = untangle::bind(t, &triangle_mock::rotate);
  auto action2 = untangle::bind(c, &circle_mock::rotate);
  auto action3 = untangle::bind(s, &square_mock::rotate);

  auto actuator_rotate = untangle::connect(action1, action2, action3);
  actuator_rotate(20);

  testing::Mock::VerifyAndClearExpectations(t.get());
  testing::Mock::VerifyAndClearExpectations(c.get());
  testing::Mock::VerifyAndClearExpectations(s.get());
}

TEST(test_actuator, test_polymorphism_using_pointers) {
  triangle_mock t;
  circle_mock c;
  square_mock s;

  // using polymorphism
  std::vector<shape*> shapes;
  shapes.push_back(&t);
  shapes.push_back(&c);
  shapes.push_back(&s);

  EXPECT_CALL(t, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(c, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(s, rotate(testing::_)).WillOnce(testing::Return());
  rotate_shapes(shapes, 10);

  testing::Mock::VerifyAndClearExpectations(&t);
  testing::Mock::VerifyAndClearExpectations(&c);
  testing::Mock::VerifyAndClearExpectations(&s);

  // using actuator
  EXPECT_CALL(t, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(c, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(s, rotate(testing::_)).WillOnce(testing::Return());
  auto action1 = untangle::bind(&t, &triangle_mock::rotate);
  auto action2 = untangle::bind(&c, &circle_mock::rotate);
  auto action3 = untangle::bind(&s, &square_mock::rotate);

  auto actuator_rotate = untangle::connect(action1, action2, action3);
  actuator_rotate(20);

  testing::Mock::VerifyAndClearExpectations(&t);
  testing::Mock::VerifyAndClearExpectations(&c);
  testing::Mock::VerifyAndClearExpectations(&s);
}

TEST(test_actuator, test_assignment) {
  const auto t = std::make_shared<triangle_mock>();
  const auto c = std::make_shared<circle_mock>();
  const auto s = std::make_shared<square_mock>();

  EXPECT_CALL(*t, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(*c, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(*s, rotate(testing::_)).WillOnce(testing::Return());
  auto action1 = untangle::bind(t, &triangle_mock::rotate);
  auto action2 = untangle::bind(c, &circle_mock::rotate);
  auto action3 = untangle::bind(s, &square_mock::rotate);

  auto actuator_rotate = untangle::connect(action1, action2, action3);
  untangle::actuator<decltype(actuator_rotate.type())> actuator_rotate_1 = actuator_rotate;
  actuator_rotate_1(20);

  testing::Mock::VerifyAndClearExpectations(t.get());
  testing::Mock::VerifyAndClearExpectations(c.get());
  testing::Mock::VerifyAndClearExpectations(s.get());
}

TEST(test_actuator, test_self_assignment) {
  const auto t = std::make_shared<triangle_mock>();
  const auto c = std::make_shared<circle_mock>();

  EXPECT_CALL(*t, rotate(30)).WillOnce(testing::Return());
  EXPECT_CALL(*c, rotate(30)).WillOnce(testing::Return());
  auto action1 = untangle::bind(t, &triangle_mock::rotate);
  auto action2 = untangle::bind(c, &circle_mock::rotate);

  auto actuator_rotate = untangle::connect(action1, action2);
  actuator_rotate.add("circle", &action2);
  ASSERT_EQ(actuator_rotate.actions.size(), 2);
  ASSERT_TRUE(actuator_rotate.has_action("circle"));

  // self-assignment must be a no-op, not a wipe.
  // assigned through an alias so the compiler does not flag the self-assignment (-Wself-assign-overloaded).
  const auto& alias = actuator_rotate;
  actuator_rotate = alias;

  EXPECT_EQ(actuator_rotate.actions.size(), 2);
  EXPECT_TRUE(actuator_rotate.has_action("circle"));

  // the surviving actions must still be callable
  actuator_rotate(30);

  testing::Mock::VerifyAndClearExpectations(t.get());
  testing::Mock::VerifyAndClearExpectations(c.get());
}

TEST(test_actuator, test_assignment_copies_results) {
  const auto t = std::make_shared<triangle>();
  const auto c = std::make_shared<circle>();

  t->height_in(11);
  c->height_in(22);

  auto action1 = untangle::bind(t, &triangle::height_out);
  auto action2 = untangle::bind(c, &circle::height_out);

  auto source = untangle::connect(action1, action2);
  source();
  ASSERT_THAT(source.results, testing::ElementsAre(11, 22));

  // copy-assignment must carry the results over, the way copy-construction does
  untangle::actuator<decltype(source.type())> assigned;
  assigned = source;
  EXPECT_THAT(assigned.results, testing::ElementsAre(11, 22));

  // the two copy paths must produce indistinguishable objects
  untangle::actuator<decltype(source.type())> constructed = source;
  EXPECT_EQ(assigned.results, constructed.results);
}

TEST(test_actuator, test_add) {
  const auto t = std::make_shared<triangle_mock>();
  const auto c = std::make_shared<circle_mock>();
  const auto s = std::make_shared<square_mock>();

  EXPECT_CALL(*t, rotate(testing::_)).Times(2).WillRepeatedly(testing::Return());
  EXPECT_CALL(*c, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(*s, rotate(testing::_)).WillOnce(testing::Return());
  auto action1 = untangle::bind(t, &triangle_mock::rotate);
  auto action2 = untangle::bind(c, &circle_mock::rotate);
  auto action3 = untangle::bind(s, &square_mock::rotate);

  auto actuator_rotate = untangle::connect(action1, action2, action3);
  actuator_rotate.add(&action1);
  actuator_rotate(20);

  testing::Mock::VerifyAndClearExpectations(t.get());
  testing::Mock::VerifyAndClearExpectations(c.get());
  testing::Mock::VerifyAndClearExpectations(s.get());
}

TEST(test_actuator, test_remove) {
  const auto t = std::make_shared<triangle_mock>();
  const auto c = std::make_shared<circle_mock>();
  const auto s = std::make_shared<square_mock>();

  EXPECT_CALL(*t, rotate(testing::_)).Times(0);
  EXPECT_CALL(*c, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(*s, rotate(testing::_)).WillOnce(testing::Return());
  auto action1 = untangle::bind(t, &triangle_mock::rotate);
  auto action2 = untangle::bind(c, &circle_mock::rotate);
  auto action3 = untangle::bind(s, &square_mock::rotate);

  auto actuator_rotate = untangle::connect(action1, action2, action3);
  actuator_rotate.remove(&action1);
  actuator_rotate(50);

  testing::Mock::VerifyAndClearExpectations(t.get());
  testing::Mock::VerifyAndClearExpectations(c.get());
  testing::Mock::VerifyAndClearExpectations(s.get());
}

TEST(test_actuator, test_remove_by_empty_action) {
  const auto t = std::make_shared<triangle_mock>();
  const auto c = std::make_shared<circle_mock>();
  const auto s = std::make_shared<square_mock>();

  EXPECT_CALL(*t, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(*c, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(*s, rotate(testing::_)).WillOnce(testing::Return());
  auto action1 = untangle::bind(t, &triangle_mock::rotate);
  auto action2 = untangle::bind(c, &circle_mock::rotate);
  auto action3 = untangle::bind(s, &square_mock::rotate);

  auto actuator_rotate = untangle::connect(action1, action2, action3);
  actuator_rotate(70);

  testing::Mock::VerifyAndClearExpectations(t.get());
  testing::Mock::VerifyAndClearExpectations(c.get());
  testing::Mock::VerifyAndClearExpectations(s.get());

  EXPECT_CALL(*t, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(*c, rotate(testing::_)).Times(0);
  EXPECT_CALL(*s, rotate(testing::_)).WillOnce(testing::Return());
  std::function<void(int)> action_empty;
  actuator_rotate = untangle::connect(action1, action_empty, action3);
  actuator_rotate(80);

  testing::Mock::VerifyAndClearExpectations(t.get());
  testing::Mock::VerifyAndClearExpectations(c.get());
  testing::Mock::VerifyAndClearExpectations(s.get());
}

TEST(test_actuator, test_reset) {
  const auto t = std::make_shared<triangle_mock>();
  const auto c = std::make_shared<circle_mock>();

  // nothing may fire after a reset
  EXPECT_CALL(*t, rotate(testing::_)).Times(0);
  EXPECT_CALL(*c, rotate(testing::_)).Times(0);
  auto action1 = untangle::bind(t, &triangle_mock::rotate);
  auto action2 = untangle::bind(c, &circle_mock::rotate);

  auto actuator_rotate = untangle::connect(action1);
  actuator_rotate.add("circle", &action2);
  EXPECT_TRUE(actuator_rotate.is_connected());
  EXPECT_TRUE(actuator_rotate.has_action("circle"));

  actuator_rotate.reset();

  EXPECT_FALSE(actuator_rotate.is_connected());
  EXPECT_FALSE(actuator_rotate.has_action("circle"));
  EXPECT_EQ(actuator_rotate.actions.size(), 0);
  actuator_rotate(10);

  testing::Mock::VerifyAndClearExpectations(t.get());
  testing::Mock::VerifyAndClearExpectations(c.get());

  // reset must clear the stored results too
  const auto tr = std::make_shared<triangle>();
  auto action3 = untangle::bind(tr, &triangle::height_out);
  auto actuator_height = untangle::connect(action3);
  actuator_height();
  EXPECT_EQ(actuator_height.results.size(), 1);

  actuator_height.reset();
  EXPECT_EQ(actuator_height.results.size(), 0);
}

TEST(test_actuator, test_invalid_action) {
  const auto t = std::make_shared<triangle_mock>();
  auto c = std::make_shared<circle_mock>();
  const auto s = std::make_shared<square_mock>();

  EXPECT_CALL(*t, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(*c, rotate(testing::_)).Times(0);
  EXPECT_CALL(*s, rotate(testing::_)).WillOnce(testing::Return());
  auto action1 = untangle::bind(t, &triangle_mock::rotate);
  auto action2 = untangle::bind(c, &circle_mock::rotate);
  auto action3 = untangle::bind(s, &square_mock::rotate);

  auto actuator_rotate = untangle::connect(action1, action2, action3);
  c.reset();
  actuator_rotate(60);

  testing::Mock::VerifyAndClearExpectations(t.get());
  testing::Mock::VerifyAndClearExpectations(c.get());
  testing::Mock::VerifyAndClearExpectations(s.get());
}

TEST(test_actuator, test_extract_results) {
  //! [test_extract_results]
  const auto t = std::make_shared<triangle>();
  const auto c = std::make_shared<circle>();
  const auto s = std::make_shared<square>();

  auto action1 = untangle::bind(t, &triangle::height_in);
  auto action2 = untangle::bind(c, &circle::height_in);
  auto action3 = untangle::bind(s, &square::height_in);

  auto actuator_height_in = untangle::connect(action1, action2, action3);
  actuator_height_in(80);

  EXPECT_EQ(actuator_height_in.results.size(), 0);

  auto action4 = untangle::bind(t, &triangle::height_out);
  auto action5 = untangle::bind(c, &circle::height_out);
  auto action6 = untangle::bind(s, &square::height_out);

  auto actuator_height_out = untangle::connect(action4, action5, action6);
  actuator_height_out();

  EXPECT_EQ(actuator_height_out.results.size(), 3);
  EXPECT_THAT(actuator_height_out.results, testing::ElementsAre(80, 80, 80));

  //! [test_extract_results]
}

TEST(test_actuator, test_void_return_no_args)
{
  //! [test_void_return_no_args]
  const auto t = std::make_shared<triangle_mock>();
  const auto c = std::make_shared<circle_mock>();
  const auto s = std::make_shared<square_mock>();

  EXPECT_CALL(*t, test_vr_no_args).Times(1);
  EXPECT_CALL(*c, test_vr_no_args).Times(1);
  EXPECT_CALL(*s, test_vr_no_args).Times(1);
  auto action1 = untangle::bind(t, &triangle_mock::test_vr_no_args);
  auto action2 = untangle::bind(c, &circle_mock::test_vr_no_args);
  auto action3 = untangle::bind(s, &square_mock::test_vr_no_args);

  auto actuator = untangle::connect(action1, action2, action3);

  actuator();

  EXPECT_EQ(actuator.results.size(), 0);

  testing::Mock::VerifyAndClearExpectations(t.get());
  testing::Mock::VerifyAndClearExpectations(c.get());
  testing::Mock::VerifyAndClearExpectations(s.get());
  //! [test_void_return_no_args]
}

TEST(test_actuator, test_void_return_and_args)
{
  //! [test_void_return_and_args]
  const auto t = std::make_shared<triangle_mock>();
  const auto c = std::make_shared<circle_mock>();
  const auto s = std::make_shared<square_mock>();

  EXPECT_CALL(*t, test_vr_args).Times(1);
  EXPECT_CALL(*c, test_vr_args).Times(1);
  EXPECT_CALL(*s, test_vr_args).Times(1);
  auto action1 = untangle::bind(t, &triangle_mock::test_vr_args);
  auto action2 = untangle::bind(c, &circle_mock::test_vr_args);
  auto action3 = untangle::bind(s, &square_mock::test_vr_args);

  auto actuator = untangle::connect(action1, action2, action3);

  actuator(90, 100);

  EXPECT_EQ(actuator.results.size(), 0);

  testing::Mock::VerifyAndClearExpectations(t.get());
  testing::Mock::VerifyAndClearExpectations(c.get());
  testing::Mock::VerifyAndClearExpectations(s.get());
  //! [test_void_return_and_args]
}

} // namespace untangle::test
