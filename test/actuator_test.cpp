// Copyright (c) 2025 Nicolae Popescu. MIT License.

/**
 * @brief Test the actuator concept.
 */
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <actuator.hpp>

namespace untangle::test {

class shape {
 public:
  virtual ~shape() = default;
  virtual void rotate(int angle) const = 0;
  virtual void test_vr_no_args() const = 0;
  virtual void test_vr_args(int x, int y) const = 0;
};

class triangle : public shape {
 public:
  ~triangle() override { std::cout << "triangle::~triangle" << std::endl; }
  void rotate(int angle) const override { std::cout << "triangle::rotate " << angle << std::endl; }

  void height_in(int h) {
    std::cout << "triangle::height_in" << std::endl;
    height = h;
  }
  int height_out() const {
    std::cout << "triangle::height_out" << std::endl;
    return height;
  }

  void test_vr_no_args() const override { std::cout << "triangle::test_vr" << std::endl; }

  void test_vr_args(int x, int y) const override {
    std::cout << "triangle::test_vr_args " << x << ", " << y << std::endl;
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

class circle : public shape {
 public:
  void rotate(int angle) const override { std::cout << "circle::rotate " << angle << std::endl; }

  void height_in(int h) {
    std::cout << "circle::height_in" << std::endl;
    height = h;
  }

  int height_out() const {
    std::cout << "circle::height_out" << std::endl;
    return height;
  }

  void test_vr_no_args() const override { std::cout << "circle::test_vr" << std::endl; }
  void test_vr_args(int x, int y) const override {
    std::cout << "circle::test_vr_args " << x << ", " << y << std::endl;
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

class square : public shape {
 public:
  void rotate(int angle) const override { std::cout << "square::rotate " << angle << std::endl; }

  void height_in(int h) {
    std::cout << "square::height_in" << std::endl;
    height = h;
  }

  int height_out() const {
    std::cout << "square::height_out" << std::endl;
    return height;
  }

  void test_vr_no_args() const override { std::cout << "square::test_vr" << std::endl; }

  void test_vr_args(int x, int y) const override {
    std::cout << "square::test_vr_args " << x << ", " << y << std::endl;
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
void rotate_shapes(const std::vector<shape*>& shapes, int angle) {
  for (const auto& s : shapes) {
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
  // assigned through an alias so the compiler does not flag the self-assignment
  // (-Wself-assign-overloaded).
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

  try {
    throw untangle::invalid_action("boom");
  } catch (const std::exception& e) {
    caught_as_std_exception = true;
    message = e.what();
  } catch (...) {
    // private inheritance makes the base inaccessible, so the handler above is skipped
  }

  EXPECT_TRUE(caught_as_std_exception) << "invalid_action is not catchable as std::exception";
  EXPECT_EQ(message, "boom") << "a generic handler must see the real message, not a placeholder";
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
  EXPECT_TRUE(static_cast<bool>(action2)) << "the actuator emptied a std::function it does not own";

  // Consequence of the same defect: a caller re-invoking its own action should still
  // get the dead-binding report, not std::bad_function_call from an emptied function.
  EXPECT_THROW(action2(20), untangle::invalid_action);

  testing::Mock::VerifyAndClearExpectations(t.get());
}

//! Returns an action bound to a shared_ptr that dies when this function returns.
//! The action must outlive the object safely.
std::function<int()> make_height_action() {
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

TEST(test_actuator, test_void_return_no_args) {
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

TEST(test_actuator, test_void_return_and_args) {
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
struct measurement {
  explicit measurement(int v) : value(v) {}
  int value;
};

TEST(test_actuator, test_invoke_action_non_default_constructible_result) {
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
struct counted_result {
  static inline int copies = 0;
  static inline int moves = 0;
  static void reset() {
    copies = 0;
    moves = 0;
  }

  int value;
  explicit counted_result(int v) : value(v) {}
  counted_result(const counted_result& other) : value(other.value) { ++copies; }
  counted_result(counted_result&& other) noexcept : value(other.value) { ++moves; }
  counted_result& operator=(const counted_result&) = default;
  counted_result& operator=(counted_result&&) = default;
};

TEST(test_actuator, test_move_does_not_copy) {
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

TEST(test_actuator, test_const_observers) {
  const auto t = std::make_shared<triangle_mock>();
  auto action = untangle::bind(t, &triangle_mock::rotate);

  // A const actuator must still answer questions about itself. Before the
  // observers were marked const these three lines did not compile.
  const auto actuator_rotate = untangle::connect(std::make_pair(std::string("triangle"), &action));

  EXPECT_TRUE(actuator_rotate.is_connected());
  EXPECT_TRUE(actuator_rotate.has_action("triangle"));
  EXPECT_FALSE(actuator_rotate.has_action("circle"));

  untangle::actuator<decltype(actuator_rotate.type())> copy = actuator_rotate;
  EXPECT_TRUE(copy.is_connected());
}

TEST(test_actuator, test_bind_null_pointer_is_a_dead_action) {
  // The raw-pointer bind guards only with assert(), which compiles out under
  // NDEBUG and leaves a raw dereference. A null binding should instead behave
  // like a dead weak_ptr binding: throw invalid_action so the actuator drops it.
  triangle_mock* dead = nullptr;
  auto action = untangle::bind(dead, &triangle_mock::rotate);

  EXPECT_THROW(action(10), untangle::invalid_action);

  auto actuator_rotate = untangle::connect(action);
  actuator_rotate(10);
  EXPECT_FALSE(actuator_rotate.is_connected());
}

TEST(test_actuator, test_action_has_callback) {
  int result = 0;
  std::function action = [](int v, std::function<void(int)>& cbk) { return v; };

  auto actuator = untangle::connect(action);
  std::function cbk = [&result](int v) { result = v; };
  actuator(1, cbk);

  ASSERT_EQ(actuator.results.size(), 1);
  ASSERT_EQ(result, 1);
}

TEST(test_actuator, test_action_callback_passed_as_rvalue) {
  // The action takes the callback by value, so invoking the action moves from the
  // caller's std::function. The actuator must copy the callback out of the argument
  // pack before it invokes the action, otherwise it calls a moved-from function and
  // std::bad_function_call escapes the call operator.
  int result = 0;
  // A capture too large for the std::function small-object buffer forces a
  // heap-allocated target, which a move really does leave empty.
  char pad[256] = {};
  // That is a property of the standard library, not of the code under test, so verify it
  // holds here -- otherwise the assertions below would pass without proving anything.
  std::function moved_from = [&result, pad](int v) { result = v + pad[0]; };
  std::function sink = std::move(moved_from);
  ASSERT_FALSE(moved_from) << "pad is too small for this stdlib; the test below is vacuous";
  std::function action = [](int v, std::function<void(int)>) { return v; };

  auto actuator = untangle::connect(action);
  std::function cbk = [&result, pad](int v) { result = v + pad[0]; };
  actuator(10, std::move(cbk));

  ASSERT_EQ(actuator.results.size(), 1);
  ASSERT_EQ(result, 10);
}

TEST(test_actuator, test_named_action_has_callback) {
  // The callback convention applies to invoke_action() exactly as it does to operator()().
  int result = 0;
  std::function<int(int, std::function<void(int)>)> action = [](int v, std::function<void(int)>) {
    return v;
  };

  auto actuator = untangle::connect(std::make_pair(std::string("echo"), &action));
  std::function<void(int)> cbk = [&result](int v) { result = v; };
  actuator.invoke_action("echo", 21, cbk);

  ASSERT_EQ(actuator.results.size(), 1);
  ASSERT_EQ(result, 21);
}

TEST(test_actuator, test_callback_invoked_for_each_action) {
  // The callback is copied once, ahead of the action loop, so every action must still reach
  // it -- each with its own return value.
  std::vector<int> seen;
  std::function<int(int, std::function<void(int)>)> first = [](int v, std::function<void(int)>) {
    return v;
  };
  std::function<int(int, std::function<void(int)>)> second = [](int v, std::function<void(int)>) {
    return v * 2;
  };

  auto actuator = untangle::connect(first, second);
  std::function<void(int)> cbk = [&seen](int v) { seen.push_back(v); };
  actuator(10, cbk);

  ASSERT_EQ(actuator.results.size(), 2);
  ASSERT_THAT(seen, ::testing::ElementsAre(10, 20));
}

TEST(test_actuator, test_anonymous_lambda_as_callback) {
  // The callback convention tests what the trailing argument can do, not what it is, so a
  // lambda works without being wrapped in a std::function first.
  int result = 0;
  std::function<int(int, std::function<void(int)>)> action = [](int v, std::function<void(int)>) {
    return v;
  };

  auto actuator = untangle::connect(action);
  actuator(7, [&result](int v) { result = v; });

  ASSERT_EQ(actuator.results.size(), 1);
  ASSERT_EQ(result, 7);
}

TEST(test_actuator, test_trailing_non_callable_is_not_a_callback) {
  // A trailing argument that cannot be called with the return type is an ordinary argument.
  std::function<int(int, int)> action = [](int v, int w) { return v + w; };

  auto actuator = untangle::connect(action);
  actuator(2, 3);

  ASSERT_EQ(actuator.results.size(), 1);
  ASSERT_EQ(actuator.results.front(), 5);
}

TEST(test_actuator, test_callback_with_return_type_is_not_accepted) {
  // A callback exists to consume the action's return value, so it returns nothing itself.
  // A trailing callable that does return something is the action's own data -- a transform,
  // a comparator -- and must not be taken for a callback. This action ignores the transform
  // entirely, so any call to it can only have come from the callback convention.
  int calls = 0;
  std::function<int(int, std::function<int(int)>)> action = [](int v, std::function<int(int)> f) {
    return v;
  };

  auto actuator = untangle::connect(action);
  std::function<int(int)> cbk = [&calls](int v) {
    ++calls;
    return v;
  };
  actuator(5, cbk);

  ASSERT_EQ(actuator.results.size(), 1);
  ASSERT_EQ(actuator.results.front(), 5);
  ASSERT_EQ(calls, 0) << "a callable returning non-void is not a callback and must not run";
}

TEST(test_actuator, test_every_action_receives_a_usable_callback) {
  // The argument convention: an action must not take ownership of what it is invoked with,
  // because operator() forwards one argument pack to every action in the list. Honour it --
  // here by invoking with an lvalue -- and each action in turn is handed a callback it can
  // actually call, alongside the actuator's own invocation of it.
  //
  // Violating it is undetectable from inside the actuator: pass the callback below as
  // std::move(cbk) and the second action receives an empty std::function instead, because
  // its by value parameter move constructs from the same pack the first action already
  // emptied. Nothing in the library can diagnose that; it is the caller and the action
  // signatures together that decide it.
  int usable_callbacks = 0;
  std::function<int(int, std::function<void(int)>)> first =
      [&usable_callbacks](int v, std::function<void(int)> cbk) {
        if (cbk) {
          ++usable_callbacks;
        }
        return v;
      };
  std::function<int(int, std::function<void(int)>)> second =
      [&usable_callbacks](int v, std::function<void(int)> cbk) {
        if (cbk) {
          ++usable_callbacks;
        }
        return v * 2;
      };

  auto actuator = untangle::connect(first, second);
  std::vector<int> completed;
  std::function<void(int)> cbk = [&completed](int v) { completed.push_back(v); };
  actuator(10, cbk);

  ASSERT_EQ(actuator.results.size(), 2);
  ASSERT_THAT(completed, ::testing::ElementsAre(10, 20));
  ASSERT_EQ(usable_callbacks, 2) << "every action must be handed a callback it can call";
}

TEST(test_actuator, test_add_anonymous_lambda) {
  //! [test_add_anonymous_lambda]
  // An anonymous lambda has no named variable to outlive the actuator, so the actuator takes
  // ownership of it: it is moved into actuator::owned and the list points at the stored copy.
  untangle::actuator<std::function<int(int)>> actuator_scale;
  actuator_scale.add([](int v) { return v * 2; });
  actuator_scale.add([](int v) { return v * 3; });

  actuator_scale(10);
  ASSERT_THAT(actuator_scale.results, ::testing::ElementsAre(20, 30));
  //! [test_add_anonymous_lambda]
  ASSERT_EQ(actuator_scale.owned.size(), 2);
}

TEST(test_actuator, test_add_anonymous_lambda_mixed_with_owned_action) {
  // Ownership is per action, not per actuator: an action the caller owns is still only
  // pointed at, and lives alongside the ones the actuator owns.
  std::function<int(int)> external = [](int v) { return v - 1; };

  untangle::actuator<std::function<int(int)>> actuator_scale;
  actuator_scale.add(&external);
  actuator_scale.add([](int v) { return v * 2; });

  actuator_scale(10);
  ASSERT_THAT(actuator_scale.results, ::testing::ElementsAre(9, 20));
  ASSERT_EQ(actuator_scale.actions.size(), 2);
  ASSERT_EQ(actuator_scale.owned.size(), 1) << "only the anonymous lambda is owned";
}

TEST(test_actuator, test_add_named_anonymous_lambda) {
  untangle::actuator<std::function<int(int)>> actuator_scale;
  ASSERT_NE(actuator_scale.add("double", [](int v) { return v * 2; }), nullptr);
  ASSERT_TRUE(actuator_scale.has_action("double"));

  actuator_scale.invoke_action("double", 10);
  ASSERT_THAT(actuator_scale.results, ::testing::ElementsAre(20));
}

TEST(test_actuator, test_add_named_anonymous_lambda_rejects_a_taken_name) {
  // A taken name keeps the action it already has, as it does for the pointer overload. The
  // action offered here must not be left in actuator::owned with nothing pointing at it.
  untangle::actuator<std::function<int(int)>> actuator_scale;
  actuator_scale.add("scale", [](int v) { return v * 2; });
  ASSERT_EQ(actuator_scale.add("scale", [](int v) { return v * 3; }), nullptr);

  actuator_scale.invoke_action("scale", 10);
  ASSERT_THAT(actuator_scale.results, ::testing::ElementsAre(20)) << "the first action is kept";
  ASSERT_EQ(actuator_scale.owned.size(), 1) << "the rejected action must not be stored";
}

TEST(test_actuator, test_owned_action_survives_the_source_of_a_copy) {
  // A defaulted copy would duplicate actuator::owned but leave the copied pointers aimed at
  // the source's storage, so the copy would dangle the moment the source lets go of it.
  untangle::actuator<std::function<int(int)>> actuator_scale;
  actuator_scale.add([](int v) { return v * 2; });
  actuator_scale.add("triple", [](int v) { return v * 3; });

  auto actuator_copy = actuator_scale;
  actuator_scale.reset();  // destroys the actions the copy was made from

  actuator_copy(10);
  ASSERT_THAT(actuator_copy.results, ::testing::ElementsAre(20));
  actuator_copy.invoke_action("triple", 10);
  ASSERT_THAT(actuator_copy.results, ::testing::ElementsAre(30));
}

TEST(test_actuator, test_copy_does_not_re_point_an_action_it_does_not_own) {
  // The translation table holds the owned actions only, so a pointer to the caller's action
  // is copied unchanged and both actuators keep pointing at the one object.
  std::function<int(int)> external = [](int v) { return v - 1; };

  untangle::actuator<std::function<int(int)>> actuator_scale;
  actuator_scale.add(&external);

  const auto actuator_copy = actuator_scale;
  ASSERT_EQ(actuator_copy.actions.front(), &external);
  ASSERT_TRUE(actuator_copy.owned.empty());
}

TEST(test_actuator, test_remove_releases_the_storage_of_an_owned_action) {
  // Removing drops a pointer; for an owned action the std::function behind it has to go too,
  // or a loop of add() and remove() grows actuator::owned without bound.
  untangle::actuator<std::function<int(int)>> actuator_scale;
  for (int i = 0; i < 4; ++i) {
    auto* handle = actuator_scale.add([](int v) { return v; });
    actuator_scale.remove(handle);
  }
  ASSERT_TRUE(actuator_scale.actions.empty());
  ASSERT_TRUE(actuator_scale.owned.empty());

  actuator_scale.add("named", [](int v) { return v; });
  actuator_scale.remove("named");
  ASSERT_TRUE(actuator_scale.actions_map.empty());
  ASSERT_TRUE(actuator_scale.owned.empty());
}

TEST(test_actuator, test_remove_keeps_an_owned_action_something_still_points_at) {
  // One handle can be added to both the list and the map. Removing it from one must not
  // destroy it while the other is still pointing at it.
  untangle::actuator<std::function<int(int)>> actuator_scale;
  auto* handle = actuator_scale.add([](int v) { return v * 2; });
  actuator_scale.add("double", handle);
  ASSERT_EQ(actuator_scale.owned.size(), 1);

  actuator_scale.remove(handle);
  ASSERT_TRUE(actuator_scale.actions.empty());
  ASSERT_EQ(actuator_scale.owned.size(), 1) << "the map still points at it";

  actuator_scale.invoke_action("double", 10);
  ASSERT_THAT(actuator_scale.results, ::testing::ElementsAre(20));

  actuator_scale.remove("double");
  ASSERT_TRUE(actuator_scale.owned.empty());
}

TEST(test_actuator, test_dead_owned_binding_releases_its_storage) {
  // An owned action can be a binding too, and a binding to a destroyed object is dropped by
  // operator() as any other. The storage behind it must be released with it.
  auto t = std::make_shared<triangle_mock>();

  untangle::actuator<std::function<void(int)>> actuator_rotate;
  actuator_rotate.add(untangle::bind(t, &triangle_mock::rotate));
  ASSERT_EQ(actuator_rotate.owned.size(), 1);

  t.reset();
  actuator_rotate(20);
  ASSERT_TRUE(actuator_rotate.actions.empty());
  ASSERT_TRUE(actuator_rotate.owned.empty());
}

TEST(test_actuator, test_connect_anonymous_lambda) {
  //! [test_connect_anonymous_lambda]
  // connect() deduces the action type from its arguments. A lambda has its own closure type
  // and is not a std::function until something converts it, so a call made only of anonymous
  // lambdas has nothing to deduce from and the signature has to be named on connect()
  // itself. Naming the type of the variable assigned to would not supply it: template
  // arguments are never deduced from what the returned value is assigned to.
  auto actuator_scale = untangle::connect<std::function<int(int)>>([](int v) { return v * 2; },
                                                                   [](int v) { return v * 3; });

  actuator_scale(10);
  ASSERT_THAT(actuator_scale.results, ::testing::ElementsAre(20, 30));
  //! [test_connect_anonymous_lambda]
  ASSERT_EQ(actuator_scale.owned.size(), 2) << "connect() owns what it was given as rvalues";
}

TEST(test_actuator, test_connect_deduces_from_one_named_action) {
  // One named action anywhere in the call deduces the action type for the whole of it, and
  // the anonymous lambdas beside it then need no explicit signature.
  std::function<int(int)> external = [](int v) { return v - 1; };

  auto actuator_scale = untangle::connect(external, [](int v) { return v * 2; });

  actuator_scale(10);
  ASSERT_THAT(actuator_scale.results, ::testing::ElementsAre(9, 20));
  ASSERT_EQ(actuator_scale.actions.front(), &external) << "the named action is only pointed at";
  ASSERT_EQ(actuator_scale.owned.size(), 1) << "the lambda beside it is owned";
}

TEST(test_actuator, test_connect_owned_actions_survive_the_call) {
  // connect() builds the actuator locally and returns it. The actions it owns live in a
  // std::list, whose elements keep their addresses when the list is moved out, so the
  // pointers built inside connect() are still the right ones here.
  const auto actuator_scale =
      untangle::connect<std::function<int(int)>>([](int v) { return v * 2; });

  auto actuator_copy = actuator_scale;
  actuator_copy(10);
  ASSERT_THAT(actuator_copy.results, ::testing::ElementsAre(20));
}

TEST(test_actuator, test_connect_drops_an_empty_owned_action) {
  // An empty action is dropped rather than stored, as connect() has always done, and an
  // empty owned one must not be left in actuator::owned with nothing pointing at it.
  auto actuator_scale = untangle::connect<std::function<int(int)>>(std::function<int(int)>(),
                                                                   [](int v) { return v * 2; });

  ASSERT_EQ(actuator_scale.actions.size(), 1);
  ASSERT_EQ(actuator_scale.owned.size(), 1);

  actuator_scale(10);
  ASSERT_THAT(actuator_scale.results, ::testing::ElementsAre(20));
}

TEST(test_actuator, test_connect_named_anonymous_lambda) {
  //! [test_connect_named_anonymous_lambda]
  // A pair holding the action itself hands it to the actuator to own. The action type is
  // deduced from a pointer, and there is no pointer here, so the signature has to be named
  // on connect() -- as it does for the unnamed overload.
  auto actuator_scale = untangle::connect<std::function<int(int)>>(
      std::make_pair("double", [](int v) { return v * 2; }),
      std::make_pair("triple", [](int v) { return v * 3; }));

  actuator_scale.invoke_action("double", 10);
  ASSERT_THAT(actuator_scale.results, ::testing::ElementsAre(20));
  actuator_scale.invoke_action("triple", 10);
  ASSERT_THAT(actuator_scale.results, ::testing::ElementsAre(30));
  //! [test_connect_named_anonymous_lambda]
  ASSERT_EQ(actuator_scale.owned.size(), 2);
}

TEST(test_actuator, test_connect_named_mixes_owned_and_pointed_at_actions) {
  // A leading pointer pair deduces the action type for the whole call, and the pairs beside
  // it can then hold anonymous lambdas without naming it.
  std::function<int(int)> external = [](int v) { return v - 1; };

  auto actuator_scale =
      untangle::connect(std::make_pair(std::string("external"), &external),
                        std::make_pair(std::string("double"), [](int v) { return v * 2; }));

  ASSERT_TRUE(actuator_scale.has_action("external"));
  ASSERT_TRUE(actuator_scale.has_action("double"));
  ASSERT_EQ(actuator_scale.actions_map.at("external"), &external) << "only pointed at";
  ASSERT_EQ(actuator_scale.owned.size(), 1) << "the lambda beside it is owned";

  actuator_scale.invoke_action("double", 10);
  ASSERT_THAT(actuator_scale.results, ::testing::ElementsAre(20));
}

TEST(test_actuator, test_connect_named_owned_action_survives_the_source_of_a_copy) {
  const auto actuator_scale = untangle::connect<std::function<int(int)>>(
      std::make_pair("double", [](int v) { return v * 2; }));

  auto actuator_copy = actuator_scale;
  actuator_copy.invoke_action("double", 10);
  ASSERT_THAT(actuator_copy.results, ::testing::ElementsAre(20));
}

TEST(test_actuator, test_connect_named_drops_an_empty_owned_action) {
  auto actuator_scale = untangle::connect<std::function<int(int)>>(
      std::make_pair("empty", std::function<int(int)>()),
      std::make_pair("double", [](int v) { return v * 2; }));

  ASSERT_FALSE(actuator_scale.has_action("empty"));
  ASSERT_EQ(actuator_scale.owned.size(), 1);
}

TEST(test_actuator, test_connect_named_drops_a_null_pointer) {
  // Unchanged behaviour of the pointer overload: a null pointer is not stored.
  std::function<int(int)> external = [](int v) { return v - 1; };

  const auto actuator_scale = untangle::connect(
      std::make_pair(std::string("null"), static_cast<decltype(external)*>(nullptr)),
      std::make_pair(std::string("external"), &external));

  ASSERT_FALSE(actuator_scale.has_action("null"));
  ASSERT_TRUE(actuator_scale.has_action("external"));
}

TEST(test_actuator, test_invoke_action_drops_an_empty_action) {
  // An empty std::function throws std::bad_function_call when called, which is not an
  // invalid_action and would escape invoke_action(). It is dropped instead, as operator()()
  // drops an empty action from the list.
  std::function<int(int)> empty_action;

  untangle::actuator<std::function<int(int)>> actuator_scale;
  actuator_scale.add("empty", &empty_action);
  ASSERT_TRUE(actuator_scale.has_action("empty"));

  ASSERT_NO_THROW(actuator_scale.invoke_action("empty", 10));
  ASSERT_FALSE(actuator_scale.has_action("empty"));
  ASSERT_TRUE(actuator_scale.results.empty());
}

TEST(test_actuator, test_invoke_action_drops_a_null_action) {
  // Dereferencing the null pointer to call through it is undefined behaviour before a call
  // is even made, so the pointer is tested rather than the action behind it.
  untangle::actuator<std::function<int(int)>> actuator_scale;
  actuator_scale.add("null", nullptr);
  ASSERT_TRUE(actuator_scale.has_action("null"));

  ASSERT_NO_THROW(actuator_scale.invoke_action("null", 10));
  ASSERT_FALSE(actuator_scale.has_action("null"));
}

TEST(test_actuator, test_invoke_action_drops_an_empty_owned_action) {
  // The storage behind a dropped owned action has to go with it, as it does everywhere else.
  untangle::actuator<std::function<int(int)>> actuator_scale;
  std::function<int(int)> empty_action;
  actuator_scale.add("empty", std::move(empty_action));
  ASSERT_EQ(actuator_scale.owned.size(), 1);

  ASSERT_NO_THROW(actuator_scale.invoke_action("empty", 10));
  ASSERT_FALSE(actuator_scale.has_action("empty"));
  ASSERT_TRUE(actuator_scale.owned.empty());
}

TEST(test_actuator, test_invoke_action_ignores_an_unknown_name) {
  untangle::actuator<std::function<int(int)>> actuator_scale;
  actuator_scale.add("double", [](int v) { return v * 2; });

  ASSERT_NO_THROW(actuator_scale.invoke_action("missing", 10));
  ASSERT_TRUE(actuator_scale.results.empty());
  ASSERT_TRUE(actuator_scale.has_action("double")) << "the other actions are untouched";
}

TEST(test_actuator, test_move_keeps_the_handles_of_the_source) {
  // Moving an actuator has to leave every handle already handed out pointing at a live action:
  // actuator::actions holds addresses into actuator::owned, and a std::list move transfers the
  // nodes rather than the elements, so the addresses survive the move.
  untangle::actuator<std::function<int(int)>> actuator_scale;
  auto* handle = actuator_scale.add([](int v) { return v * 2; });
  auto* named_handle = actuator_scale.add("triple", [](int v) { return v * 3; });

  const auto actuator_moved = std::move(actuator_scale);

  ASSERT_EQ(actuator_moved.actions.front(), handle) << "the action moved to another address";
  ASSERT_EQ(actuator_moved.actions_map.at("triple"), named_handle);
  ASSERT_EQ(&actuator_moved.owned.front(), handle) << "the handle is not the stored action";
}

TEST(test_actuator, test_move_carries_the_owned_actions_and_empties_the_source) {
  untangle::actuator<std::function<int(int)>> actuator_scale;
  actuator_scale.add([](int v) { return v * 2; });
  actuator_scale.add("triple", [](int v) { return v * 3; });

  auto actuator_moved = std::move(actuator_scale);

  ASSERT_FALSE(actuator_scale.is_connected()) << "the source kept actions it no longer owns";
  ASSERT_TRUE(actuator_scale.owned.empty());

  actuator_moved(10);
  ASSERT_THAT(actuator_moved.results, ::testing::ElementsAre(20));
  actuator_moved.invoke_action("triple", 10);
  ASSERT_THAT(actuator_moved.results, ::testing::ElementsAre(30));
}

TEST(test_actuator, test_move_assignment_carries_the_owned_actions) {
  untangle::actuator<std::function<int(int)>> actuator_scale;
  auto* handle = actuator_scale.add([](int v) { return v * 2; });

  untangle::actuator<std::function<int(int)>> actuator_moved;
  actuator_moved.add([](int v) { return v; });
  actuator_moved = std::move(actuator_scale);

  ASSERT_EQ(actuator_moved.actions.size(), 1) << "the actions it held are gone";
  ASSERT_EQ(actuator_moved.actions.front(), handle);
  ASSERT_FALSE(actuator_scale.is_connected());

  actuator_moved(10);
  ASSERT_THAT(actuator_moved.results, ::testing::ElementsAre(20));
}

TEST(test_actuator, test_an_action_that_throws_does_not_stop_the_ones_behind_it) {
  // Each action is invoked in isolation: what one throws is recorded and the rest still run.
  untangle::actuator<std::function<int(int)>> actuator_scale;
  actuator_scale.add([](int v) { return v * 2; });
  actuator_scale.add([](int) -> int { throw std::runtime_error("the action threw"); });
  actuator_scale.add([](int v) { return v * 3; });

  actuator_scale(10);

  ASSERT_THAT(actuator_scale.results, ::testing::ElementsAre(20, 30));
  ASSERT_EQ(actuator_scale.errors.size(), 1);
  ASSERT_EQ(actuator_scale.actions.size(), 3) << "an action that threw is not a dead one";
}

TEST(test_actuator, test_what_an_action_threw_is_in_errors) {
  untangle::actuator<std::function<void(void)>> actuator_notify;
  actuator_notify.add([] { throw std::runtime_error("the action threw"); });

  actuator_notify();

  ASSERT_EQ(actuator_notify.errors.size(), 1);
  EXPECT_THROW(std::rethrow_exception(actuator_notify.errors.front()), std::runtime_error);
}

TEST(test_actuator, test_a_dead_binding_is_recorded_and_dropped) {
  // A dead binding used to be printed and dropped. It is still dropped; what it threw is now the
  // caller's to read.
  auto shape_obj = std::make_shared<triangle>();
  untangle::actuator<std::function<void(int)>> actuator_rotate;
  actuator_rotate.add(untangle::bind(shape_obj, &triangle::rotate));

  shape_obj.reset();
  actuator_rotate(90);

  ASSERT_EQ(actuator_rotate.errors.size(), 1);
  EXPECT_THROW(std::rethrow_exception(actuator_rotate.errors.front()), untangle::invalid_action);
  ASSERT_FALSE(actuator_rotate.is_connected()) << "a dead binding is dropped";
}

TEST(test_actuator, test_errors_belong_to_the_last_invocation) {
  untangle::actuator<std::function<void(void)>> actuator_notify;
  auto* handle = actuator_notify.add([] { throw std::runtime_error("the action threw"); });

  actuator_notify();
  ASSERT_EQ(actuator_notify.errors.size(), 1);

  actuator_notify.remove(handle);
  actuator_notify.add([] {});
  actuator_notify();
  ASSERT_TRUE(actuator_notify.errors.empty()) << "the previous invocation's errors were kept";
}

TEST(test_actuator, test_invoke_action_records_what_the_action_threw) {
  untangle::actuator<std::function<void(void)>> actuator_notify;
  actuator_notify.add("throwing", [] { throw std::runtime_error("the action threw"); });

  actuator_notify.invoke_action("throwing");

  ASSERT_EQ(actuator_notify.errors.size(), 1);
  ASSERT_TRUE(actuator_notify.has_action("throwing")) << "an action that threw is not a dead one";
}

// --- tasks, step 1 -------------------------------------------------------------------------------
// A task is an action bound to its arguments and to the callback it must notify. Unlike an action's
// callback, a task's is required, is taken by position rather than recognised by its type, and
// exists for a void result too: a task with nothing to report still has a completion to report.
// See todo/FEATURE_PLAN.md.

//! A callable that consumes an int and returns nothing: a callback for a task returning int.
struct int_sink {
  void operator()(int) const {}
};

//! A callable that consumes an int and returns one: the action's own data, never a callback.
struct int_transform {
  int operator()(int v) const { return v; }
};

//! A callable that takes nothing and returns nothing: a callback for a task returning void.
struct finished_sink {
  void operator()() const {}
};

TEST(test_actuator, test_task_callback_for_accepts_any_void_returning_callable) {
  // The convention tests what the callback can do, not what it is, so a std::function, a
  // functor, a lambda and a plain function pointer all qualify.
  static_assert(untangle::task_callback_for<std::function<void(int)>, int>);
  static_assert(untangle::task_callback_for<int_sink, int>);
  static_assert(untangle::task_callback_for<void (*)(int), int>);

  auto lambda = [](int) {};
  static_assert(untangle::task_callback_for<decltype(lambda), int>);

  // A result type with no default constructor is still a result to report.
  static_assert(untangle::task_callback_for<std::function<void(measurement)>, measurement>);
}

TEST(test_actuator, test_task_callback_for_rejects_what_cannot_report_a_result) {
  // A callback exists to consume the result, so it returns nothing itself. One that returns a
  // value is the action's own data -- a transform, a comparator -- and is not a callback.
  static_assert(!untangle::task_callback_for<std::function<int(int)>, int>);
  static_assert(!untangle::task_callback_for<int_transform, int>);

  // Not callable at all.
  static_assert(!untangle::task_callback_for<int, int>);

  // Callable, returns nothing, but cannot be handed the result.
  static_assert(!untangle::task_callback_for<std::function<void(std::string)>, int>);
  static_assert(!untangle::task_callback_for<std::function<void()>, int>);
}

TEST(test_actuator, test_task_callback_for_a_void_result_takes_no_argument) {
  // Decided 2026-09-25: a task returning nothing still reports that it finished, so its callback
  // is a void() rather than no callback at all. That is what separates a task from an action --
  // an action with a void return has no callback of any kind.
  static_assert(untangle::task_callback_for<std::function<void()>, void>);
  static_assert(untangle::task_callback_for<finished_sink, void>);

  // There is no result to hand it, so one that asks for a result cannot report this task.
  static_assert(!untangle::task_callback_for<std::function<void(int)>, void>);
  static_assert(!untangle::task_callback_for<int_sink, void>);
}

TEST(test_actuator, test_task_names_the_callback_type_its_result_needs) {
  // The caller writes the callback, so the task has to say what shape it must have -- and for a
  // void result that shape is not std::function<void(void)> by accident, it is the one callback a
  // task with no result can have.
  static_assert(std::is_same_v<untangle::task<int>::callback_t, std::function<void(int)>>);
  static_assert(std::is_same_v<untangle::task<void>::callback_t, std::function<void()>>);

  static_assert(std::is_same_v<untangle::task<int>::result_type, int>);
  static_assert(std::is_same_v<untangle::task<void>::result_type, void>);

  // Whatever a task names, it must satisfy the concept the callback is constrained by, or a
  // caller could write the type the task asks for and still be refused.
  static_assert(untangle::task_callback_for<untangle::task<int>::callback_t, int>);
  static_assert(untangle::task_callback_for<untangle::task<void>::callback_t, void>);
}

TEST(test_actuator, test_task_without_a_call_is_empty) {
  // actuator::operator() tests an action before invoking it, and a task has to answer that test
  // the way a std::function does. A default-constructed one holds nothing to run.
  untangle::task<int> empty;
  ASSERT_FALSE(static_cast<bool>(empty));

  untangle::task<int> ready;
  ready.call = [] { return 42; };
  ASSERT_TRUE(static_cast<bool>(ready));
}

TEST(test_actuator, test_task_carries_the_callback_it_must_notify) {
  // The whole point of a task: the callback travels with it rather than arriving beside the
  // invocation, so one held in a container can still be notified when it runs. Built by hand
  // here -- bind_task() is step 2.
  int reported = 0;

  untangle::task<int> one;
  one.call = [] { return 21 * 2; };
  one.callback = [&reported](int result) { reported = result; };

  const int result = one();
  ASSERT_EQ(result, 42);

  one.callback(result);
  ASSERT_EQ(reported, 42) << "a task that cannot notify is not a task";
}

TEST(test_actuator, test_void_task_carries_a_callback_that_reports_only_that_it_finished) {
  // The half an action's callback convention has no answer for: nothing to hand over, and still
  // something to say.
  bool finished = false;
  int ran = 0;

  untangle::task<void> one;
  one.call = [&ran] { ++ran; };
  one.callback = [&finished] { finished = true; };

  one();
  ASSERT_EQ(ran, 1);
  ASSERT_FALSE(finished) << "the call must not notify; call_tasks() does, in step 4";

  one.callback();
  ASSERT_TRUE(finished);
}

// --- tasks, step 2 -------------------------------------------------------------------------------
// bind_task() builds a task: it binds the action's arguments into task::call and takes
// task::callback off the end of the same pack. The callback is the last argument, by position - a
// pack cannot be followed by a deducible parameter, so the split happens inside bind_task() and
// every caller above it just forwards. See todo/FEATURE_PLAN.md step 2.

//! An action type that is not a std::function: a callable naming its own result_type.
struct summing_action {
  using result_type = int;

  int operator()(int a, int b) const { return a + b; }
};

TEST(test_actuator, test_bind_task_binds_the_arguments_and_carries_the_callback) {
  // That this compiles at all is half the claim: the action takes two arguments, and it is given
  // two plus a callback. If bind_task forwarded the callback to the action the call would be
  // action(20, 22, cbk) and there would be no such overload.
  int reported = 0;
  std::function<int(int, int)> action = [](int a, int b) { return a + b; };

  auto one = untangle::bind_task(
      action, 20, 22, std::function<void(int)>([&reported](int result) { reported = result; }));

  ASSERT_TRUE(static_cast<bool>(one)) << "bind_task returned a task with nothing to run";
  ASSERT_EQ(one(), 42);
  ASSERT_EQ(reported, 0) << "the call must not notify; call_tasks() does, in step 4";

  one.callback(42);
  ASSERT_EQ(reported, 42);
}

TEST(test_actuator, test_bind_task_with_no_arguments_takes_only_the_callback) {
  // An action with nothing to bind still needs a callback, so the callback is the whole pack.
  int reported = 0;
  std::function<int()> action = [] { return 7; };

  auto one = untangle::bind_task(
      action, std::function<void(int)>([&reported](int result) { reported = result; }));

  one.callback(one());
  ASSERT_EQ(reported, 7);
}

TEST(test_actuator, test_bind_task_for_a_void_action_takes_a_void_callback) {
  // The half an action's callback convention has no answer for: nothing to hand over, and still
  // something to say.
  bool finished = false;
  int ran = 0;
  std::function<void(int)> action = [&ran](int by) { ran += by; };

  auto one =
      untangle::bind_task(action, 5, std::function<void()>([&finished] { finished = true; }));

  one();
  ASSERT_EQ(ran, 5);
  ASSERT_FALSE(finished);

  one.callback();
  ASSERT_TRUE(finished);
}

TEST(test_actuator, test_bind_task_copies_its_arguments_at_bind_time) {
  // A task runs later than it is built -- that is what a queue is for -- so what it runs with has
  // to be the caller's values as they were when the task was made, not whatever they became.
  int value = 10;
  std::function<int(int)> action = [](int n) { return n; };

  auto one = untangle::bind_task(action, value, std::function<void(int)>([](int) {}));

  value = 99;
  ASSERT_EQ(one(), 10) << "the task read the caller's variable rather than its own copy";
}

TEST(test_actuator, test_bind_task_hands_the_action_lvalues) {
  // The bound arguments are the task's own, and it hands them over as lvalues -- so an action
  // taking a reference is given the copy inside the task, and may write to it. Anything else would
  // mean an action taking int& could not be a task at all.
  std::function<int(int&)> action = [](int& n) {
    n *= 2;
    return n;
  };

  int original = 21;
  auto one = untangle::bind_task(action, original, std::function<void(int)>([](int) {}));

  ASSERT_EQ(one(), 42);
  ASSERT_EQ(original, 21) << "the action wrote through to the caller's object";
}

TEST(test_actuator, test_bind_task_accepts_a_bare_lambda_as_the_callback) {
  // task_callback_for tests what the callback can do, not what it is, so a lambda needs no
  // wrapping at the call site; the task erases it into its own callback_t.
  int reported = 0;
  std::function<int(int)> action = [](int n) { return n * 3; };

  auto one = untangle::bind_task(action, 7, [&reported](int result) { reported = result; });

  static_assert(std::is_same_v<decltype(one), untangle::task<int>>);

  one.callback(one());
  ASSERT_EQ(reported, 21);
}

TEST(test_actuator, test_bind_task_accepts_an_action_type_that_is_not_a_std_function) {
  // bind_task reads result_type off the action type, which is all it asks of it. A caller's own
  // functor therefore builds a task, and never touches std::function::result_type -- which C++20
  // removed.
  int reported = 0;

  auto one = untangle::bind_task(summing_action{}, 40, 2,
                                 std::function<void(int)>([&reported](int r) { reported = r; }));

  static_assert(std::is_same_v<decltype(one), untangle::task<int>>);

  one.callback(one());
  ASSERT_EQ(reported, 42);
}

TEST(test_actuator, test_two_tasks_from_one_action_keep_their_own_arguments_and_callbacks) {
  // What lets a container of tasks exist: the arguments live in the task, not in the action, so one
  // action yields tasks that differ in what they run with and in who they report to.
  int first = 0;
  int second = 0;
  std::function<int(int)> action = [](int n) { return n; };

  auto one =
      untangle::bind_task(action, 1, std::function<void(int)>([&first](int r) { first = r; }));
  auto two =
      untangle::bind_task(action, 2, std::function<void(int)>([&second](int r) { second = r; }));

  two.callback(two());
  one.callback(one());

  ASSERT_EQ(first, 1);
  ASSERT_EQ(second, 2);
}

// --- tasks, step 3 -------------------------------------------------------------------------------
// The actuator holds tasks beside its actions: actuator::tasks, reached through add_task(). A task
// is stored by value and never removed individually - call_tasks() consumes the list - so none of
// the machinery actions need (pointers, owned, remove(), the translate() step in copy_from) applies
// to it. What does apply is that copy_from() copies every member by hand, so tasks have to be
// copied there too or a copied actuator silently loses them. See todo/FEATURE_PLAN.md step 3.

TEST(test_actuator, test_add_task_holds_the_task) {
  untangle::actuator<std::function<int(int)>> actuator;
  ASSERT_TRUE(actuator.tasks.empty());

  const bool taken = actuator.add_task(untangle::bind_task(
      std::function<int(int)>([](int n) { return n; }), 1, std::function<void(int)>([](int) {})));

  ASSERT_TRUE(taken) << "add_task refused a task it should have taken";
  ASSERT_EQ(actuator.tasks.size(), 1);
  ASSERT_TRUE(static_cast<bool>(actuator.tasks.front()));
}

TEST(test_actuator, test_add_task_keeps_the_order_they_were_added) {
  // The order tasks come out in is the order they went in. A queue built on this depends on it.
  untangle::actuator<std::function<int(int)>> actuator;
  std::function<int(int)> action = [](int n) { return n; };

  for (int i = 1; i <= 3; ++i) {
    actuator.add_task(untangle::bind_task(action, i, std::function<void(int)>([](int) {})));
  }

  std::vector<int> seen;
  for (auto& one : actuator.tasks) {
    seen.push_back(one());
  }

  ASSERT_THAT(seen, testing::ElementsAre(1, 2, 3));
}

TEST(test_actuator, test_a_task_survives_being_stored) {
  // It is moved into the list, so what it bound has to still be there afterwards -- both the
  // arguments in its call and the callback it has to notify.
  int reported = 0;
  untangle::actuator<std::function<int(int, int)>> actuator;

  actuator.add_task(untangle::bind_task(
      std::function<int(int, int)>([](int a, int b) { return a + b; }), 20, 22,
      std::function<void(int)>([&reported](int result) { reported = result; })));

  auto& stored = actuator.tasks.front();
  stored.callback(stored());

  ASSERT_EQ(reported, 42);
}

TEST(test_actuator, test_copying_an_actuator_copies_its_tasks) {
  // copy_from() copies each member by hand rather than defaulting, so a member it does not name is
  // silently dropped. A copy that lost its tasks would look like an actuator with nothing to do.
  int reported = 0;
  untangle::actuator<std::function<int(int)>> source;

  source.add_task(untangle::bind_task(
      std::function<int(int)>([](int n) { return n * 2; }), 21,
      std::function<void(int)>([&reported](int result) { reported = result; })));

  untangle::actuator<std::function<int(int)>> copy = source;

  ASSERT_EQ(copy.tasks.size(), 1) << "the copy lost the tasks the source held";
  ASSERT_EQ(source.tasks.size(), 1) << "copying took the tasks from the source";

  auto& one = copy.tasks.front();
  one.callback(one());
  ASSERT_EQ(reported, 42) << "the copied task no longer runs what it was bound to";
}

TEST(test_actuator, test_moving_an_actuator_carries_its_tasks) {
  // The queue this is built for takes its batch by move, under a lock, and runs it outside one.
  int reported = 0;
  untangle::actuator<std::function<int(int)>> source;

  source.add_task(untangle::bind_task(
      std::function<int(int)>([](int n) { return n * 2; }), 21,
      std::function<void(int)>([&reported](int result) { reported = result; })));

  auto moved = std::move(source);
  ASSERT_EQ(moved.tasks.size(), 1);

  untangle::actuator<std::function<int(int)>> assigned;
  assigned = std::move(moved);
  ASSERT_EQ(assigned.tasks.size(), 1);

  auto& one = assigned.tasks.front();
  one.callback(one());
  ASSERT_EQ(reported, 42);
}

TEST(test_actuator, test_tasks_and_actions_live_side_by_side) {
  // The two kinds share an actuator and nothing else. operator() fires the actions and leaves the
  // tasks where they are -- call_tasks() is what fires those, in step 4.
  int action_calls = 0;
  int task_runs = 0;
  int reported = 0;

  std::function<int(int)> action = [&action_calls](int n) {
    ++action_calls;
    return n;
  };

  auto actuator = untangle::connect(action);
  actuator.add_task(untangle::bind_task(
      std::function<int(int)>([&task_runs](int n) {
        ++task_runs;
        return n;
      }),
      7, std::function<void(int)>([&reported](int result) { reported = result; })));

  actuator(5);

  ASSERT_EQ(action_calls, 1) << "adding a task disturbed the actions";
  ASSERT_EQ(actuator.results.size(), 1) << "the task's result was collected by operator()";
  ASSERT_EQ(actuator.results.front(), 5);
  ASSERT_EQ(task_runs, 0) << "operator() ran a task; only call_tasks() may";
  ASSERT_EQ(reported, 0) << "operator() notified a task's callback";
  ASSERT_EQ(actuator.tasks.size(), 1) << "operator() consumed a task";
}

TEST(test_actuator, test_add_task_refuses_an_empty_callback) {
  // The one hole the signature cannot close: bind_task's parameter makes a callback impossible to
  // omit, not impossible to leave empty, and an empty std::function satisfies task_callback_for.
  // Refused here, on the caller's own stack, rather than thrown from call_tasks() later -- where it
  // would be recorded as a failure of a task whose action had in fact succeeded.
  untangle::actuator<std::function<int(int)>> actuator;

  auto one = untangle::bind_task(std::function<int(int)>([](int n) { return n; }), 1,
                                 std::function<void(int)>{});
  ASSERT_FALSE(static_cast<bool>(one.callback)) << "the callback under test is not actually empty";

  const bool taken = actuator.add_task(std::move(one));

  ASSERT_FALSE(taken) << "add_task took a task that can never notify";
  ASSERT_TRUE(actuator.tasks.empty()) << "the refused task was stored anyway";
}

TEST(test_actuator, test_add_task_refuses_a_task_with_nothing_to_run) {
  // The same rule from the other side. A hand-built task can hold a callback and no call, and it
  // would throw std::bad_function_call out of call_tasks() exactly as an empty callback does.
  untangle::actuator<std::function<int(int)>> actuator;

  untangle::task<int> one;
  one.callback = [](int) {};
  ASSERT_FALSE(static_cast<bool>(one));

  const bool taken = actuator.add_task(std::move(one));

  ASSERT_FALSE(taken) << "add_task took a task with no action to run";
  ASSERT_TRUE(actuator.tasks.empty());
}

TEST(test_actuator, test_a_refused_task_leaves_the_ones_already_held) {
  // A refusal is about the task offered, not about the actuator: what it already holds is untouched
  // and still runnable.
  int reported = 0;
  untangle::actuator<std::function<int(int)>> actuator;

  ASSERT_TRUE(actuator.add_task(untangle::bind_task(
      std::function<int(int)>([](int n) { return n * 2; }), 21,
      std::function<void(int)>([&reported](int result) { reported = result; }))));

  ASSERT_FALSE(actuator.add_task(untangle::bind_task(
      std::function<int(int)>([](int n) { return n; }), 1, std::function<void(int)>{})));

  ASSERT_EQ(actuator.tasks.size(), 1);
  auto& kept = actuator.tasks.front();
  kept.callback(kept());
  ASSERT_EQ(reported, 42);
}

// --- tasks, step 4 -------------------------------------------------------------------------------
// call_tasks() fires what add_task() holds: it runs each task, notifies its callback, records what
// anything threw, and consumes the list. Two rules it does NOT share with operator():
//
//   - a task's result goes to its callback and nowhere else. actuator::results is how an action
//     hands back what it returned; a task has a better way, and does not need both.
//   - it appends to actuator::errors rather than clearing it, so an actuator fired as
//     `one(); one.call_tasks();` reports both kinds together. operator() is the one that clears,
//     so the actions go first - which is the order a queue built on this uses anyway.
//
// See todo/FEATURE_PLAN.md step 4.

TEST(test_actuator, test_call_tasks_fires_every_task_and_notifies_each) {
  // Each task reports its own result, and they run in the order they were added.
  std::vector<int> reported;
  untangle::actuator<std::function<int(int)>> actuator;
  std::function<int(int)> action = [](int n) { return n * 10; };

  for (int i = 1; i <= 3; ++i) {
    ASSERT_TRUE(actuator.add_task(untangle::bind_task(
        action, i, std::function<void(int)>([&reported](int r) { reported.push_back(r); }))));
  }

  actuator.call_tasks();

  ASSERT_THAT(reported, testing::ElementsAre(10, 20, 30));
}

TEST(test_actuator, test_call_tasks_consumes_the_tasks) {
  // A task is a one-shot. That is what lets the list need no remove().
  int calls = 0;
  untangle::actuator<std::function<int(int)>> actuator;

  actuator.add_task(untangle::bind_task(std::function<int(int)>([&calls](int n) {
                                          ++calls;
                                          return n;
                                        }),
                                        1, std::function<void(int)>([](int) {})));

  actuator.call_tasks();
  ASSERT_EQ(calls, 1);
  ASSERT_TRUE(actuator.tasks.empty()) << "call_tasks left the tasks it had already fired";

  actuator.call_tasks();
  ASSERT_EQ(calls, 1) << "a task ran twice";
}

TEST(test_actuator, test_call_tasks_notifies_a_void_task_with_nothing) {
  // The half an action's callback convention has no answer for: finished is the whole message.
  int ran = 0;
  bool finished = false;
  untangle::actuator<std::function<void(int)>> actuator;

  actuator.add_task(untangle::bind_task(std::function<void(int)>([&ran](int by) { ran += by; }), 5,
                                        std::function<void()>([&finished] { finished = true; })));

  actuator.call_tasks();

  ASSERT_EQ(ran, 5);
  ASSERT_TRUE(finished) << "a void task finished without saying so";
}

TEST(test_actuator, test_a_task_that_throws_does_not_notify) {
  // Finished does not mean failed. There is no result to report and no completion to report, so
  // the callback is not invoked; what it threw goes to errors, and the tasks behind it still run.
  bool notified = false;
  bool later_ran = false;
  untangle::actuator<std::function<int(int)>> actuator;

  actuator.add_task(untangle::bind_task(
      std::function<int(int)>([](int) -> int { throw std::runtime_error("task failed"); }), 1,
      std::function<void(int)>([&notified](int) { notified = true; })));

  actuator.add_task(untangle::bind_task(std::function<int(int)>([&later_ran](int n) {
                                          later_ran = true;
                                          return n;
                                        }),
                                        2, std::function<void(int)>([](int) {})));

  actuator.call_tasks();

  ASSERT_FALSE(notified) << "a task that threw reported as though it had finished";
  ASSERT_TRUE(later_ran) << "one task throwing stopped the ones behind it";
  ASSERT_EQ(actuator.errors.size(), 1);
  EXPECT_THROW(std::rethrow_exception(actuator.errors.front()), std::runtime_error);
}

TEST(test_actuator, test_a_throwing_callback_is_recorded_like_any_other_failure) {
  // The callback runs inside the same try as the task, so what it throws travels the same path --
  // which means errors can hold a failure for a task whose action in fact succeeded. Surprising if
  // unsaid, so it is said, here and in the reference.
  int ran = 0;
  untangle::actuator<std::function<int(int)>> actuator;

  actuator.add_task(untangle::bind_task(
      std::function<int(int)>([&ran](int n) {
        ++ran;
        return n;
      }),
      1, std::function<void(int)>([](int) { throw std::runtime_error("callback failed"); })));

  actuator.call_tasks();

  ASSERT_EQ(ran, 1) << "the task's own action did not run";
  ASSERT_EQ(actuator.errors.size(), 1);
  EXPECT_THROW(std::rethrow_exception(actuator.errors.front()), std::runtime_error);
  ASSERT_TRUE(actuator.tasks.empty()) << "a task whose callback threw was left in the list";
}

TEST(test_actuator, test_a_task_result_is_delivered_only_to_its_callback) {
  // actuator::results is how an ACTION hands back what it returned. A task hands its result to the
  // callback it was built with, so it does not also grow the results list.
  int reported = 0;
  untangle::actuator<std::function<int(int)>> actuator;

  actuator.add_task(
      untangle::bind_task(std::function<int(int)>([](int n) { return n * 2; }), 21,
                          std::function<void(int)>([&reported](int r) { reported = r; })));

  actuator.call_tasks();

  ASSERT_EQ(reported, 42);
  ASSERT_TRUE(actuator.results.empty())
      << "a task's result was collected as though it were an action's";
}

TEST(test_actuator, test_call_tasks_leaves_the_actions_alone) {
  // The mirror of test_tasks_and_actions_live_side_by_side: operator() does not fire tasks, and
  // call_tasks() does not fire actions.
  int action_calls = 0;
  int reported = 0;

  std::function<int(int)> action = [&action_calls](int n) {
    ++action_calls;
    return n;
  };

  auto actuator = untangle::connect(action);
  actuator.add_task(
      untangle::bind_task(std::function<int(int)>([](int n) { return n; }), 7,
                          std::function<void(int)>([&reported](int r) { reported = r; })));

  actuator.call_tasks();

  ASSERT_EQ(reported, 7);
  ASSERT_EQ(action_calls, 0) << "call_tasks() fired an action; only operator() may";
  ASSERT_TRUE(actuator.is_connected()) << "call_tasks() dropped the actions";
}

TEST(test_actuator, test_call_tasks_appends_to_errors_rather_than_clearing_them) {
  // operator() clears errors and call_tasks() does not, so firing the actions first and the tasks
  // second reports both kinds together. That is the order a queue built on this uses.
  std::function<int(int)> throwing_action = [](int) -> int {
    throw std::runtime_error("action failed");
  };

  auto actuator = untangle::connect(throwing_action);
  actuator.add_task(untangle::bind_task(
      std::function<int(int)>([](int) -> int { throw std::runtime_error("task failed"); }), 1,
      std::function<void(int)>([](int) {})));

  actuator(5);
  ASSERT_EQ(actuator.errors.size(), 1) << "the action's failure was not recorded";

  actuator.call_tasks();
  ASSERT_EQ(actuator.errors.size(), 2) << "call_tasks() cleared what the actions had reported";
}

// --- tasks, step 6 -------------------------------------------------------------------------------
// has_tasks() answers for the tasks, is_connected() for the actions, and neither answers for the
// other. Decided 2026-09-25: is_connected() keeps its meaning exactly, because the attachment paths
// in async read it and none of them will ever hold a task. A queue built on this asks has_tasks()
// to decide whether its worker still has work. See todo/FEATURE_PLAN.md step 6.

TEST(test_actuator, test_has_tasks_answers_for_the_tasks_alone) {
  untangle::actuator<std::function<int(int)>> actuator;
  ASSERT_FALSE(actuator.has_tasks());

  ASSERT_TRUE(actuator.add_task(untangle::bind_task(
      std::function<int(int)>([](int n) { return n; }), 1, std::function<void(int)>([](int) {}))));
  ASSERT_TRUE(actuator.has_tasks());

  actuator.call_tasks();
  ASSERT_FALSE(actuator.has_tasks()) << "has_tasks() still reported work after call_tasks()";
}

TEST(test_actuator, test_has_tasks_and_is_connected_are_not_the_same_question) {
  // All four combinations, because a queue reading the wrong one either parks with work still
  // queued or spins on an actuator that has nothing to do.
  std::function<int(int)> action = [](int n) { return n; };
  auto task_of = [&action] {
    return untangle::bind_task(action, 1, std::function<void(int)>([](int) {}));
  };

  untangle::actuator<std::function<int(int)>> neither;
  EXPECT_FALSE(neither.has_tasks());
  EXPECT_FALSE(neither.is_connected());

  untangle::actuator<std::function<int(int)>> tasks_only;
  tasks_only.add_task(task_of());
  EXPECT_TRUE(tasks_only.has_tasks());
  EXPECT_FALSE(tasks_only.is_connected()) << "a task made the actuator look connected";

  auto actions_only = untangle::connect(action);
  EXPECT_FALSE(actions_only.has_tasks());
  EXPECT_TRUE(actions_only.is_connected());

  auto both = untangle::connect(action);
  both.add_task(task_of());
  EXPECT_TRUE(both.has_tasks());
  EXPECT_TRUE(both.is_connected());
}

TEST(test_actuator, test_is_connected_is_unmoved_by_tasks) {
  // The decision behind step 6, stated: adding, holding and firing tasks never changes what
  // is_connected() says, so async's attachment paths keep reading the answer they always read.
  std::function<int(int)> action = [](int n) { return n; };
  auto actuator = untangle::connect(action);

  ASSERT_TRUE(actuator.is_connected());

  actuator.add_task(untangle::bind_task(action, 1, std::function<void(int)>([](int) {})));
  ASSERT_TRUE(actuator.is_connected());

  actuator.call_tasks();
  ASSERT_TRUE(actuator.is_connected()) << "firing the tasks disconnected the actions";
}

TEST(test_actuator, test_a_refused_task_does_not_make_has_tasks_true) {
  // add_task() answering false means the task is gone, so nothing is waiting to be fired.
  untangle::actuator<std::function<int(int)>> actuator;

  ASSERT_FALSE(actuator.add_task(untangle::bind_task(
      std::function<int(int)>([](int n) { return n; }), 1, std::function<void(int)>{})));

  ASSERT_FALSE(actuator.has_tasks()) << "a refused task left the actuator claiming work";
}

}  // namespace untangle::test
