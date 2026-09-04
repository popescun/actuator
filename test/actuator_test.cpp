// Copyright (c) 2025 Nicolae Popescu. MIT License.

/**
 * @brief Test the actuator concept.
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
  ~triangle() override {
    std::cout << "triangle::~triangle" << std::endl;
  }
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
//! [test_polymorphism1]

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
  actuator_rotate.invoke_action("circle", 20);
  testing::Mock::VerifyAndClearExpectations(c.get());

  // invalidate triangle: invoke_action must detect the dead binding and erase it
  EXPECT_CALL(*t, rotate(testing::_)).Times(0);
  auto* raw_t = t.get();
  t.reset();
  actuator_rotate.invoke_action("triangle", 20);
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
  //! [test_polymorphism2]
  auto action1 = untangle::bind(t, &triangle_mock::rotate);
  auto action2 = untangle::bind(c, &circle_mock::rotate);
  auto action3 = untangle::bind(s, &square_mock::rotate);

  auto actuator_rotate = untangle::connect(action1, action2, action3);
  actuator_rotate(20);
  //! [test_polymorphism2]

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
  //! [test_assignment]
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
  //! [test_assignment]
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
  //! [test_add]
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
  //! [test_add]
}

TEST(test_actuator, test_remove) {
  //! [test_remove]
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
  //! [test_remove]
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

TEST(test_actuator, test_empty_action_added_directly) {
  const auto t = std::make_shared<triangle_mock>();

  EXPECT_CALL(*t, rotate(70)).WillOnce(testing::Return());
  auto action1 = untangle::bind(t, &triangle_mock::rotate);

  // connect() filters empty actions out at construction; add() does not,
  // so this is the only way an empty std::function reaches operator().
  std::function<void(int)> empty_action;

  untangle::actuator<std::function<void(int)>> actuator_rotate;
  actuator_rotate.add(&action1);
  actuator_rotate.add(&empty_action);

  // invoking an empty std::function throws std::bad_function_call, which is not
  // an invalid_action and so escapes operator() and terminates the process
  EXPECT_NO_THROW(actuator_rotate(70));

  // the unusable action must be dropped, the live one kept
  EXPECT_EQ(actuator_rotate.actions.size(), 1);

  testing::Mock::VerifyAndClearExpectations(t.get());
}

TEST(test_actuator, test_null_action_added_directly) {
  const auto t = std::make_shared<triangle_mock>();

  EXPECT_CALL(*t, rotate(80)).WillOnce(testing::Return());
  auto action1 = untangle::bind(t, &triangle_mock::rotate);

  untangle::actuator<std::function<void(int)>> actuator_rotate;
  actuator_rotate.add(&action1);
  actuator_rotate.add(nullptr);

  EXPECT_NO_THROW(actuator_rotate(80));

  // a null entry can never be invoked, so it must not linger in the list
  EXPECT_EQ(actuator_rotate.actions.size(), 1);

  testing::Mock::VerifyAndClearExpectations(t.get());
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

TEST(test_actuator, test_invalid_action_is_catchable_as_std_exception) {
  bool caught_as_std_exception = false;
  std::string message;

  try
  {
    throw untangle::invalid_action("boom");
  }
  catch (const std::exception& e)
  {
    caught_as_std_exception = true;
    message = e.what();
  }
  catch (...)
  {
    // private inheritance makes the base inaccessible, so the handler above is skipped
  }

  EXPECT_TRUE(caught_as_std_exception)
      << "invalid_action is not catchable as std::exception";
  EXPECT_EQ(message, "boom")
      << "a generic handler must see the real message, not a placeholder";
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

TEST(test_actuator, test_dead_action_leaves_caller_function_intact) {
  const auto t = std::make_shared<triangle_mock>();
  auto c = std::make_shared<circle_mock>();

  EXPECT_CALL(*t, rotate(testing::_)).WillOnce(testing::Return());
  EXPECT_CALL(*c, rotate(testing::_)).Times(0);
  auto action1 = untangle::bind(t, &triangle_mock::rotate);
  auto action2 = untangle::bind(c, &circle_mock::rotate);

  auto actuator_rotate = untangle::connect(action1, action2);

  // kill the circle binding, then trigger the actuator
  c.reset();
  actuator_rotate(60);

  // the actuator may drop the dead action from its OWN list ...
  EXPECT_EQ(actuator_rotate.actions.size(), 1);

  // ... but action2 is owned by this test, not by the actuator.
  // The actuator must not reach through its action_t* and empty it.
  EXPECT_TRUE(static_cast<bool>(action2))
      << "the actuator emptied a std::function it does not own";

  // Consequence of the same defect: a caller re-invoking its own action should still
  // get the dead-binding report, not std::bad_function_call from an emptied function.
  EXPECT_THROW(action2(20), untangle::invalid_action);

  testing::Mock::VerifyAndClearExpectations(t.get());
}

//! Returns an action bound to a shared_ptr that dies when this function returns.
//! The action must outlive the object safely.
std::function<int()> make_height_action()
{
  const auto t = std::make_shared<triangle>();
  t->height_in(7);
  return untangle::bind(t, &triangle::height_out);
}

TEST(test_actuator, test_bind_temporary_shared_ptr) {
  // The temporary dies at the end of the full expression that creates the binding.
  // Invoking it must report a dead binding, not read freed memory.
  auto action = untangle::bind(std::make_shared<triangle>(), &triangle::height_out);
  EXPECT_THROW(action(), untangle::invalid_action);
}

TEST(test_actuator, test_bind_shared_ptr_dead_after_scope) {
  // The shared_ptr was local to make_height_action() and is gone by now.
  auto action = make_height_action();

  EXPECT_THROW(action(), untangle::invalid_action);
}

TEST(test_actuator, test_bind_shared_ptr_kept_alive_during_call) {
  // A live owner must still work, and must keep working after the binding is copied
  // around -- the action must not depend on the caller's variable staying in scope.
  std::function<int()> action;
  {
    const auto t = std::make_shared<triangle>();
    t->height_in(42);
    action = untangle::bind(t, &triangle::height_out);

    // owner still alive here
    EXPECT_EQ(action(), 42);
  }
  // owner gone: dead binding, reported cleanly
  EXPECT_THROW(action(), untangle::invalid_action);
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

/**
 * @brief A result type with no default constructor.
 *
 * results_t (a std::vector) stores it happily, and operator() handles it via
 * `if constexpr`. Only invoke_action's SFINAE path needs to default-construct one.
 */
struct measurement
{
  explicit measurement(int v) : value(v) {}
  int value;
};

TEST(test_actuator, test_invoke_action_non_default_constructible_result)
{
  std::function<measurement(int)> action = [](int v) { return measurement{v}; };

  auto actuator = untangle::connect(std::make_pair(std::string("measure"), &action));

  actuator.invoke_action("measure", 7);

  ASSERT_EQ(actuator.results.size(), 1);
  EXPECT_EQ(actuator.results.front().value, 7);
}

/**
 * @brief A result type that records whether it was copied or moved.
 *
 * std::vector's move constructor is O(1) and must not touch the elements at all,
 * so a copy count of 0 after moving an actuator proves the move was a real move.
 */
struct counted_result
{
  static inline int copies = 0;
  static inline int moves = 0;
  static void reset() { copies = 0; moves = 0; }

  int value;
  explicit counted_result(int v) : value(v) {}
  counted_result(const counted_result& other) : value(other.value) {
    ++copies;
  }
  counted_result(counted_result&& other) noexcept : value(other.value) {
    ++moves;
  }
  counted_result& operator=(const counted_result&) = default;
  counted_result& operator=(counted_result&&) = default;
};

TEST(test_actuator, test_move_does_not_copy)
{
  using action_t = std::function<counted_result(int)>;
  using actuator_t = untangle::actuator<action_t>;

  // An actuator must stay copyable: declaring a move constructor without also
  // declaring the copy constructor would define the latter as deleted, which
  // breaks test_assignment's `constructed = source`.
  static_assert(std::is_copy_constructible_v<actuator_t>);
  static_assert(std::is_copy_assignable_v<actuator_t>);

  actuator_t source;
  source.results.push_back(counted_result{11});
  source.results.push_back(counted_result{22});

  counted_result::reset();
  auto moved = std::move(source);
  EXPECT_EQ(counted_result::copies, 0) << "move-construction copied the results";
  ASSERT_EQ(moved.results.size(), 2);

  actuator_t assigned;
  counted_result::reset();
  assigned = std::move(moved);
  EXPECT_EQ(counted_result::copies, 0) << "move-assignment copied the results";
  ASSERT_EQ(assigned.results.size(), 2);
  EXPECT_EQ(assigned.results.front().value, 11);
}

} // namespace untangle::test
